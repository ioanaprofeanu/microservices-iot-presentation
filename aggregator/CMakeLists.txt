cmake_minimum_required(VERSION 3.16)
project(aggregator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PahoMqttCpp REQUIRED)
find_package(PostgreSQL REQUIRED)

# Find libpqxx
find_library(PQXX_LIB pqxx)
find_path(PQXX_INCLUDE pqxx/pqxx)

if(NOT PQXX_LIB OR NOT PQXX_INCLUDE)
    message(FATAL_ERROR "libpqxx not found")
endif()

# Create executable
add_executable(aggregator main.cpp)

# Include directories
target_include_directories(aggregator 
    PRIVATE 
    ${PQXX_INCLUDE}
    ${PostgreSQL_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(aggregator 
    PRIVATE 
    PahoMqttCpp::paho-mqttpp3
    ${PQXX_LIB}
    ${PostgreSQL_LIBRARIES}
)

# Compiler options
target_compile_options(aggregator PRIVATE -Wall -Wextra -O2)
