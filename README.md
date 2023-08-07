# MyRedis

Simple version of local server-client model Redis. <br />

# build your binaries:
mkdir build/ <br />
cd build/ <br />
cmake .. <br />
cmake --build . <br />

# run server instance in background
build $ ./server & <br />

# run client:

client ops: <br />
set <key> <val>: set key val to hashmap <br />
get <key>: get the key from hashmap if it exists. <br />
del <key>: delete the key from hashmap <br />
zadd <set_name> <score> <key>: add key to sorted set with name set_name, sorted by score <br />
zrem <set_name> <key>: remove key from sorted set with name set_name <br />
zscore <set_name> <key>: get the score for key from sorted set with name set_name <br />
zrank <set_name> <key>: get the rank for key from sorted set set_name (in increasing order) <br />
zrrank <set_name> <key>: reverse of zrank, get the reverse rank (in decreasing order) <br />
zrange <set_name> <left> <right>: get the number of element with score lower bounded by left, upper bound by right <br />
expire <key>: add a expiration in ms to key. Key will be deleted after expired. <br />
ttl <key>: check the time to live value of key <br />
