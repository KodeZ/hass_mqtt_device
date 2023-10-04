cmake_minimum_required(VERSION 3.10)

project(hass_mqtt_device VERSION 0.0.1 LANGUAGES CXX)

# Specify the required C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the library
add_library(hass_mqtt_device STATIC ${SOURCES})

# Find required library
find_package(PahoMqttCpp REQUIRED)
find_package(spdlog REQUIRED)

# Link libraries
target_link_libraries(hass_mqtt_device PahoMqttCpp::paho-mqttpp3 spdlog::spdlog)

# Specify where the library should be installed
install(TARGETS hass_mqtt_device DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

# For tests, you can use CTest
enable_testing()
add_subdirectory(tests)

# For examples
add_subdirectory(examples)
