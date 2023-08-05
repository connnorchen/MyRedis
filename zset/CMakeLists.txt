add_library(ZSet zset.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_link_libraries(ZSet PUBLIC HashTable AVLTREE Utils)
target_include_directories(ZSet INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
