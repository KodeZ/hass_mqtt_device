cmake_minimum_required(VERSION 3.10)

project(hass_mqtt_device VERSION 0.0.1 LANGUAGES CXX)

option(HASS_MQTT_DEVICE_EXAMPLES "Build Examples" OFF)
option(HASS_MQTT_DEVICE_STATIC "Build as a static lib" OFF)
option(HASS_MQTT_DEVICE_SPDLOG "Use SPDLOG lib" ON)

# Assume that we are on rPi if it is an arm variant
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    add_definitions(-DARM_ARCH)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    add_definitions(-DARM64_ARCH)
endif()

# Export compile flags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the library
if(HASS_MQTT_DEVICE_STATIC)
    add_library(hass_mqtt_device STATIC ${SOURCES})
else()
    add_library(hass_mqtt_device SHARED ${SOURCES})
endif()
set_target_properties(hass_mqtt_device PROPERTIES
    # Specify the required C++ standard
    CXX_STANDARD 17
)

# Compiler flags
target_compile_options(hass_mqtt_device PRIVATE
  $<$<CONFIG:Debug>:-g3 -O0>
  $<$<CONFIG:Release>:-O3>
)

# Use ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Include directories
target_include_directories(hass_mqtt_device PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(hass_mqtt_device PUBLIC ${MOSQUITTO_INCLUDE_DIRS})

# Find required libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
find_package(nlohmann_json REQUIRED)

# Link libraries
target_link_libraries(hass_mqtt_device ${MOSQUITTO_LIBRARIES} nlohmann_json::nlohmann_json)

if(HASS_MQTT_DEVICE_SPDLOG)
    find_package(spdlog)
    add_definitions(-DHASS_MQTT_DEVICE_SPDLOG)
    if(spdlog_FOUND)
        target_link_libraries(hass_mqtt_device spdlog::spdlog)
    endif()
endif()


# Specify where the library should be installed
install(TARGETS hass_mqtt_device DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)

# For tests, you can use CTest
#enable_testing()
#add_subdirectory(tests EXCLUDE_FROM_ALL)

# Create a custom target for building tests
#add_custom_target(tests
#    COMMAND ${CMAKE_COMMAND} --build . --target all
#    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests
#)

if(HASS_MQTT_DEVICE_EXAMPLES)
    # For examples
    add_subdirectory(examples)

    # Create a custom target for building examples
    add_custom_target(examples
        COMMAND ${CMAKE_COMMAND} --build . --target all
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples
    )
endif()