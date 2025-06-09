# ACTHUNG !!!
# For development-only. Main distribution is from tapki.h
cmake_minimum_required(VERSION 3.16)
project(tapki C)

set(CMAKE_C_STANDARD 99)

add_executable(test test.c)
target_compile_definitions(test PRIVATE TAPKI_IMPLEMENTATION)
