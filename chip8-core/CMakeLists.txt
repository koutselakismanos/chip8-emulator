cmake_minimum_required(VERSION 4.0)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(chip8_core STATIC
        chip8.c
        chip8.h
)

target_include_directories(chip8_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
