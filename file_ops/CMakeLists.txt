add_library(FileOps file_ops.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_link_libraries(FileOps PUBLIC Utils)
target_include_directories(FileOps INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
