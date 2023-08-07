add_library(Heap heap.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_include_directories(Heap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
