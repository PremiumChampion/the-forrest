cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(iic)

target_include_directories(app PRIVATE "includes")

target_sources(app PRIVATE 

"includes/iic/time.hpp"
"includes/iic/bus.hpp"

"src/iic/time.cpp"
"src/iic/bus.cpp"

"src/main.cpp")
