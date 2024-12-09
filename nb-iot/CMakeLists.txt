cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nb-iot)

target_include_directories(app PRIVATE "include")

target_sources(app PRIVATE 
    "include/at/prv.hpp"
    "include/at/sim7000e/https.hpp"
    "include/at/sim7000e/power.hpp"
    "include/at/sim7000e/network_configuration.hpp"
    "include/at/sim7000e/time.hpp"
    "include/at/common.hpp"
    "include/uart/uart.hpp"
    "include/gpio/gpio.hpp"
    "include/iic/time.hpp"
    "include/iic/bus.hpp"

    "src/iic/time.cpp"
    "src/iic/bus.cpp"
    "src/at/prv.cpp"
    "src/at/sim7000e/https.cpp"
    "src/at/sim7000e/power.cpp"
    "src/at/sim7000e/network_configuration.cpp"
    "src/at/sim7000e/time.cpp"
    "src/uart/uart.cpp"
    "src/gpio/gpio.cpp"
    "src/main.cpp"
    )
