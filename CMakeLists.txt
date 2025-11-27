cmake_minimum_required(VERSION 4.0)
project(chip8_emulator C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(libs/lv_port_pc_vscode)
add_subdirectory(chip8-core)

add_executable(chip8_emulator main_pc.c)

target_link_libraries(
        chip8_emulator
        PRIVATE
        lvgl
        chip8_core
        stdc++
)
