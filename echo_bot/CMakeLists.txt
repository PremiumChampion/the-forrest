# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(echo_bot)

target_include_directories(app PRIVATE "include")

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


target_sources(app PRIVATE
    "src/memory_retention/retained.c"
    "include/memory_retention/retained.h"

    "src/main.cpp"
)
