add_executable(floatingPointTests "floatingPointIndentityTest.cpp")
target_link_libraries(floatingPointTests math-compiler)
target_include_directories(floatingPointTests PRIVATE "../../src")