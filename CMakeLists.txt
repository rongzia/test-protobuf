cmake_minimum_required(VERSION 2.8.4)
project(test_protobuf)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
add_executable(test_protobuf main.cpp)