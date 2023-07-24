add_library(Utils file_ops.cxx util.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_include_directories(Utils
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
                           )
