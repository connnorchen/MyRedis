add_library(AVLTREE avl_trees.cxx)
add_executable(test_avl_tree test_avl_tree.cxx)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_link_libraries(AVLTREE PUBLIC Utils)
target_link_libraries(test_avl_tree PUBLIC AVLTREE)
target_include_directories(AVLTREE INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
