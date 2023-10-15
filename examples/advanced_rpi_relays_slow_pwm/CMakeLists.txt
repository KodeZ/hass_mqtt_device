# ./examples/advanced_rpi_relays_slow_pwm/CMakeLists.txt

# Define the executable for the example
add_executable(advanced_rpi_relays_slow_pwm main.cpp)

# Link the necessary libraries
target_link_libraries(advanced_rpi_relays_slow_pwm PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(advanced_rpi_relays_slow_pwm PRIVATE ${CMAKE_SOURCE_DIR}/include)
