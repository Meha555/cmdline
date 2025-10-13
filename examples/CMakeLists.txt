add_executable(exp1 exp1.cpp)
target_link_libraries(exp1 PRIVATE cmdline)

add_executable(exp2 exp2.cpp)
target_link_libraries(exp2 PRIVATE cmdline)