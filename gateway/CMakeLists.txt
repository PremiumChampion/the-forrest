cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(echo_bot)

target_include_directories(app PRIVATE "include")

# main

target_sources(app PRIVATE 
    "src/main.cpp")

# uart sources

if (CONFIG_UART_0_ENABLE OR CONFIG_UART_1_ENABLE)
    target_sources(app PRIVATE 
        "include/uart/uart.hpp"
        "src/uart/uart.cpp")
endif()

if(CONFIG_UART_0_ENABLE)
    target_sources(app PRIVATE 
        "src/uart/uart0.cpp" 
        "include/uart/uart0.hpp")
endif()

if(CONFIG_UART_1_ENABLE)
    target_sources(app PRIVATE 
        "src/uart/uart1.cpp" 
        "include/uart/uart1.hpp")
endif()

# iic sources

target_sources(app PRIVATE 
    "include/iic/time.hpp"
    "src/iic/time.cpp"

    "include/iic/bus.hpp"
    "src/iic/bus.cpp")


# gpio sources

target_sources(app PRIVATE 
    "include/gpio/adc.hpp"
    "src/gpio/adc.cpp"

    "include/gpio/gpio.hpp"
    "src/gpio/gpio.cpp")

# common at sources

target_sources(app PRIVATE 
    "include/at/common.hpp"

    "include/at/prv.hpp"
    "src/at/prv.cpp")

# sim7000e sources

target_sources(app PRIVATE 
    "include/at/sim7000e/common.hpp"
    "src/at/sim7000e/common.cpp"

    "include/at/sim7000e/https.hpp"
    "src/at/sim7000e/https.cpp"
    
    "include/at/sim7000e/network_configuration.hpp"
    "src/at/sim7000e/network_configuration.cpp"
    
    "include/at/sim7000e/power.hpp"
    "src/at/sim7000e/power.cpp"
    
    "include/at/sim7000e/time.hpp"
    "src/at/sim7000e/time.cpp")

# data handling sources

target_sources(app PRIVATE 
    "include/data/collection.hpp"
    "src/data/collection.cpp"

    "include/data/reception.hpp"
    "src/data/reception.cpp"

    "include/data/transmission.hpp"
    "src/data/transmission.cpp"

    "include/data/storage.hpp"
    "src/data/storage.cpp")