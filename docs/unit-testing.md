# Unit Testing

## Running Tests

```
cd Core/Tests/unit
cmake -B build # first time only
cmake --build build
./build/test_runner
```

# Adding New Tests

  1. Write `test_foo()` in a .c file in Core/Tests/unit/ (or ../test_fms.c for FSM tests)
  2. Add the source to CMakeLists.txt
  3. Add RUN_TEST(test_foo) in runner.c
