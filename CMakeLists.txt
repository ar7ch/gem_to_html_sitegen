cmake_minimum_required(VERSION 3.12)
project(site_generator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_executable(site_generator main.cpp)
