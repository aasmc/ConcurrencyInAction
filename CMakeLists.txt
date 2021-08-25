cmake_minimum_required(VERSION 3.20)
project(ConcurrencyInAction)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_executable(ConcurrencyInAction chapter02/examples.cpp)
