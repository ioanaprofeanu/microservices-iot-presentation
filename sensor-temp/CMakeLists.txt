cmake_minimum_required(VERSION 3.16)
project(sensor-temp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PahoMqttCpp REQUIRED)

# Create executable
add_executable(sensor-temp main.cpp)

# Link libraries
target_link_libraries(sensor-temp 
    PRIVATE 
    PahoMqttCpp::paho-mqttpp3
)

# Compiler options
target_compile_options(sensor-temp PRIVATE -Wall -Wextra -O2)
