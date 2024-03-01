add_executable(simdFunctionsTests "simdFunctionsTest.cpp")
target_link_libraries(simdFunctionsTests math-compiler)
target_include_directories(simdFunctionsTests PRIVATE "../../src")