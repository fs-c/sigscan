cmake_minimum_required(VERSION 3.23)
project(sigscan)

set(CMAKE_CXX_STANDARD 23)

add_executable(sample sample/main.cpp)

add_executable(sigscan sigscan/main.cpp sigscan/process.cpp sigscan/module.cpp)
