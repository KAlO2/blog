cmake_minimum_required(VERSION 3.1)

project(sudoku)

set(CMAKE_CXX_FLAGS "-Wall -O3")

set(SUDOKU_SRC Sudoku.cpp SudokuSolver.cpp)

add_executable(sudoku ${SUDOKU_SRC})
