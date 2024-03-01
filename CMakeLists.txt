cmake_minimum_required(VERSION 3.15)

project(math-compiler)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(testMain)
add_subdirectory(testing)