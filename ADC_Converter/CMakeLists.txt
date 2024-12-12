cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(SensorTest)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE 

"include/gpio/adc.hpp"
"src/gpio/adc.cpp"

"src/main.cpp"

)
