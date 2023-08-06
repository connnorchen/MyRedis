add_library(LinkedList linked_list.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_include_directories(LinkedList INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
