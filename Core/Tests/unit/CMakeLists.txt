cmake_minimum_required(VERSION 3.22)
project(vcu_unit_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

# add_test_suite(<target> sources...)
# Registers a Unity test executable and a CTest entry.
# Always include: runner_<name>.c, stubs.c, ../unity/unity.c, and the module under test.
function(add_test_suite name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ../../User .. ../unity)
    target_compile_options(${name} PRIVATE -Wall -Wextra)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# --- Suites ---

add_test_suite(test_fsm
    runner_fsm.c
    stubs.c
    test_fsm.c
    ../../User/fsm.c
    ../unity/unity.c
)
