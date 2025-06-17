# ACTHUNG !!!
# For development-only. Main distribution is from tapki.h
cmake_minimum_required(VERSION 3.16)
project(tapki C)

set(CMAKE_C_STANDARD 99)

add_executable(test test.c)

target_compile_definitions(test PRIVATE TAPKI_IMPLEMENTATION)

if (MSVC)
    target_compile_options(test PRIVATE /W3)
else()
    target_compile_options(test PRIVATE -Wall -Wextra -Wno-missing-field-initializers)
endif()

if(CMAKE_C_COMPILER_ID MATCHES GNU|Clang)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()
