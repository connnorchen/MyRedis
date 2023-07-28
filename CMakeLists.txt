cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Redis)

add_executable(server server.cxx)
add_executable(client client.cxx)

add_subdirectory(utils)
add_subdirectory(hashtable)

target_link_libraries(server PUBLIC Utils HashTable)
target_link_libraries(client PUBLIC Utils)

target_include_directories(server PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )

target_include_directories(client PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
