add_library(ThreadPools thread_pools.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_include_directories(ThreadPools INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
