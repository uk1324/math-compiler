add_executable(testMain "main.cpp")

target_link_libraries(testMain math-compiler)

target_include_directories(testMain PRIVATE "../src")