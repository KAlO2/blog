cmake_minimum_required(VERSION 3.5)

project(sudoku)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "-Wall -O3")

set(SUDOKU_SRC Sudoku.cpp SudokuSolver.cpp)

add_executable(sudoku ${SUDOKU_SRC})
