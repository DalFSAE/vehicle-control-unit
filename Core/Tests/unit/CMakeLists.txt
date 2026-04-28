cmake_minimum_required(VERSION 3.22)
project(vcu_unit_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test_runner
    runner.c
    stubs.c
    ../test_fms.c
    ../../User/fsm.c
    ../unity/unity.c
)

target_include_directories(test_runner PRIVATE
    ../../User
    ..
)

target_compile_options(test_runner PRIVATE -Wall -Wextra)

enable_testing()
add_test(NAME unit COMMAND test_runner)
