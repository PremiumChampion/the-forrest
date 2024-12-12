cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nb-iot)

target_include_directories(app PRIVATE "include")

target_sources(app PRIVATE 
    "include/at/prv.hpp"
    "include/gpio/adc.hpp"
    "include/at/https.hpp"
    "include/at/common.hpp"
    "include/uart/uart.hpp"

    "src/main.cpp"
    "src/at/prv.cpp"
    "src/at/https.cpp"
    "src/gpio/adc.cpp"
    "src/uart/uart.cpp")
