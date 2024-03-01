add_executable(tests "tests.cpp" "testingIrCompilerMessageReporter.cpp" "testingParserMessageReporter.cpp" "testingScannerMessageReporter.cpp")
target_link_libraries(tests math-compiler)
target_include_directories(tests PRIVATE "../../src")