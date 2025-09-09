cmake_minimum_required(VERSION 3.31)
project(compi)

set(CMAKE_CXX_STANDARD 20)

add_executable(compi main.cpp)
add_executable(compi_parser main_parser.c parser.c symbol_table.c semantic.c)
