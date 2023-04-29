#include <lua.h>
#include <lauxlib.h>
#include <stdbool.h>
#include <windows.h>


#if LUA_VERSION_NUM < 502
#define luaL_newlib(L, l) ( lua_newtable( L ), luaL_register( L, NULL, l ) )
#endif

static bool initialized = false;

static int g_datacenter_id = 0;
static int g_node_id = 0;
static long g_last_timestamp = -1;
static int g_sequence = 0;

// 2014-10-20T15:00:00Z
#define SNOWFLAKE_EPOC 1413817200000L

#define NODE_ID_BITS 5
#define DATACENTER_ID_BITS 5
#define SEQUENCE_BITS 12
#define NODE_ID_SHIFT SEQUENCE_BITS
#define DATACENTER_ID_SHIFT (NODE_ID_SHIFT + NODE_ID_BITS)
#define TIMESTAMP_SHIFT (DATACENTER_ID_SHIFT + DATACENTER_ID_BITS)

#define SEQUENCE_MASK (0xffffffff ^ (0xffffffff << SEQUENCE_BITS))

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64 

// MSVC defines this in winsock2.h!?
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

static long get_timestamp() {
    timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static long get_til_next_millis(long last_timestamp) {
    long ts = get_timestamp();
    while (ts < last_timestamp) {
        ts = get_timestamp();
    }

    return ts;
}

static int luasnowflake_init(lua_State *L) {
    g_datacenter_id = luaL_checkint(L, 1);
    if (g_datacenter_id < 0x00 || g_datacenter_id > 0x1f) {
        return luaL_error(L, "datacenter_id must be an integer n, where 0 ≤ n ≤ 0x1f");
    }

    g_node_id = luaL_checkint(L, 2);
    if (g_node_id < 0x00 || g_node_id > 0x1f) {
        return luaL_error(L, "node_id must be an integer n where 0 ≤ n ≤ 0x1f");
    }

    initialized = true;

    return 1;
}

static int luasnowflake_next_id(lua_State *L) {
    if (!initialized) {
        return luaL_error(L, "snowflake.init must be called first");
    }

    long ts = get_timestamp();

    if (g_last_timestamp == ts) {
        g_sequence = (g_sequence + 1) & SEQUENCE_MASK;
        if (g_sequence == 0) {
            ts = get_til_next_millis(g_last_timestamp);
        }
    } else {
        g_sequence = 0;
    }

    g_last_timestamp = ts;
    ts = ((ts - SNOWFLAKE_EPOC) << TIMESTAMP_SHIFT)
            | (g_datacenter_id << DATACENTER_ID_SHIFT)
            | (g_node_id << NODE_ID_SHIFT)
            | g_sequence;

    char buffer[32];
    snprintf(buffer, 32, "%ld", ts);
    lua_pushstring(L, buffer);

    return 1;
}

static const struct luaL_Reg luasnowflake_lib[] = {
        {"init", luasnowflake_init},
        {"next_id", luasnowflake_next_id},
        {NULL, NULL},
};

LUALIB_API int luaopen_snowflake(lua_State *const L) {
    luaL_newlib(L, luasnowflake_lib);

    return 1;
}