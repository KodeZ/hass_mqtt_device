# ./examples/advanced_rpi_1w_twin_thermostat/CMakeLists.txt

# Define the executable for the example
add_executable(advanced_rpi_1w_twin_thermostat main.cpp)

# Link the necessary libraries
target_link_libraries(advanced_rpi_1w_twin_thermostat PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(advanced_rpi_1w_twin_thermostat PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Conditionally link wiringPi for ARM architectures
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
    if(WIRINGPI_LIBRARIES)
        target_link_libraries(advanced_rpi_1w_twin_thermostat PRIVATE ${WIRINGPI_LIBRARIES})
    else()
        message(FATAL_ERROR "wiringPi library not found")
    endif()
endif()
