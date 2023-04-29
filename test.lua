local sf = require "snowflake"

sf.init(0x1f, 0x1f)
id = sf.next_id()
print(id, type(id))
