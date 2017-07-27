cmake_minimum_required(VERSION 3.8)
project(Benchmark)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES IO/main.cpp)
add_executable(IO ${SOURCE_FILES})