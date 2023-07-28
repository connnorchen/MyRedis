add_library(HashTable hashtable.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_include_directories(HashTable INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
