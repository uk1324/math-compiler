add_executable(fuzzTests "fuzzTests.cpp" "randomInputGenerator.cpp")
target_link_libraries(fuzzTests math-compiler)
target_include_directories(fuzzTests PRIVATE "../../src")