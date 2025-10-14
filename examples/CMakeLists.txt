add_executable(exp0 exp0.cpp)
target_link_libraries(exp0 PRIVATE cmdline)

add_executable(exp1 exp1.cpp)
target_link_libraries(exp1 PRIVATE cmdline)

add_executable(exp2 exp2.cpp)
target_link_libraries(exp2 PRIVATE cmdline)

add_executable(exp3 exp3.cpp)
target_link_libraries(exp3 PRIVATE cmdline)