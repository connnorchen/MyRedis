add_library(Serialize serialize.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_include_directories(Serialize INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
