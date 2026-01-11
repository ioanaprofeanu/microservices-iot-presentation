cmake_minimum_required(VERSION 3.16)
project(sensor-vib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PahoMqttCpp REQUIRED)

# Create executable
add_executable(sensor-vib main.cpp)

# Link libraries
target_link_libraries(sensor-vib 
    PRIVATE 
    PahoMqttCpp::paho-mqttpp3
)

# Compiler options
target_compile_options(sensor-vib PRIVATE -Wall -Wextra -O2)
