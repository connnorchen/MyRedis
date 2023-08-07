# MyRedis

Simple version of local server-client model Redis. 

# build your binaries:
mkdir build/
cd build/
cmake ..
cmake --build .

# run server instance in background
build $ ./server &

# run client:

client ops:
set <key> <val>: set key val to hashmap
get <key>: get the key from hashmap if it exists. 
del <key>: delete the key from hashmap
zadd <set_name> <score> <key>: add key to sorted set with name set_name, sorted by score
zrem <set_name> <key>: remove key from sorted set with name set_name
zscore <set_name> <key>: get the score for key from sorted set with name set_name
zrank <set_name> <key>: get the rank for key from sorted set set_name (in increasing order)
zrrank <set_name> <key>: reverse of zrank, get the reverse rank (in decreasing order)
zrange <set_name> <left> <right>: get the number of element with score lower bounded by left, upper bound by right
expire <key>: add a expiration in ms to key. Key will be deleted after expired.
ttl <key>: check the time to live value of key
