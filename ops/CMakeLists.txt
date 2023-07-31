add_library(Ops ops.cxx)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_link_libraries(Ops PUBLIC FileOps Serialize)
target_include_directories(Ops INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
