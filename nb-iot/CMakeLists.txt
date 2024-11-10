cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nb-iot)

target_include_directories(app PRIVATE "include")

target_sources(app PRIVATE 
    "include/at/commands.hpp"
    "include/uart/uart.hpp"
    "include/uart/uart_callback.hpp"

    "src/main.cpp"
    "src/at/commands.cpp"
    "src/uart/uart.cpp"
    "src/uart/uart_callback.cpp")
