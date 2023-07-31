add_library(Utils util.cxx )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_link_libraries(Utils PUBLIC HashTable)
target_include_directories(Utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
