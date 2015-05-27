cmake_minimum_required(VERSION 2.6)

project(popcount)

set(CMAKE_CXX_FLAGS "-Wall -O3")

set(POPCOUNT_SRC popcount.cpp)

add_executable(popcount ${POPCOUNT_SRC})
