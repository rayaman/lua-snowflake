package = "snowflake"
version = "1.0-0"

source = {
	url = "git://github.com/rayaman/lua-snowflake.git",
	tag = "v1.0",
}

description = {
	summary = "An implementation of a distributed ID generator, similar to Snowflake by Twitter",
	homepage = "http://github.com/rayaman/lua-snowflake",
	license = "MIT",
	maintainer = "Stuart Carnie",
}

dependencies = {
	"lua >= 5.1",
}

build = {
	type = "builtin",

    modules = {
        snowflake = {
            sources = { "src/main.c" }
        }
    },
}
