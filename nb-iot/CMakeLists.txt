cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nb-iot)

target_include_directories(app PRIVATE "include")

target_sources(app PRIVATE 
    "include/at/prv.hpp"
    "include/at/https.hpp"
    "include/at/common.hpp"
    "include/at/power.hpp"
    "include/uart/uart.hpp"
    "include/gpio/gpio.hpp"

    "src/main.cpp"
    "src/at/prv.cpp"
    "src/at/https.cpp"
    "src/at/power.cpp"
    "src/uart/uart.cpp"
    "src/gpio/gpio.cpp")
