cmake_minimum_required(VERSION 3.10)

project(hass_mqtt_device VERSION 0.0.1 LANGUAGES CXX)

# Export compile flags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Specify the required C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the library
add_library(hass_mqtt_device SHARED ${SOURCES})

# Compiler flags
target_compile_options(hass_mqtt_device PRIVATE 
  $<$<CONFIG:Debug>:-g3 -O0>
  $<$<CONFIG:Release>:-O3>
)

# Include directories
target_include_directories(hass_mqtt_device PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(hass_mqtt_device PUBLIC ${MOSQUITTO_INCLUDE_DIRS})

# Find required libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
find_package(spdlog REQUIRED)

# Link libraries
target_link_libraries(hass_mqtt_device ${MOSQUITTO_LIBRARIES} spdlog::spdlog)

# Specify where the library should be installed
install(TARGETS hass_mqtt_device DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

# For tests, you can use CTest
#enable_testing()
#add_subdirectory(tests EXCLUDE_FROM_ALL)

# Create a custom target for building tests
#add_custom_target(tests
#    COMMAND ${CMAKE_COMMAND} --build . --target all
#    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests
#)

# For examples
add_subdirectory(examples)

# Create a custom target for building examples
add_custom_target(examples
    COMMAND ${CMAKE_COMMAND} --build . --target all
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples
)
