add_executable(allocatorTests "allocatorTests.cpp")
target_link_libraries(allocatorTests math-compiler)
target_include_directories(allocatorTests PRIVATE "../../src")