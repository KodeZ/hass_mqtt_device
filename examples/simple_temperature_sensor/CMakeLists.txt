# ./examples/simple_temperature_sensor/CMakeLists.txt

# Define the executable for the example
add_executable(simple_temperature_sensor main.cpp)

# Link the necessary libraries
target_link_libraries(simple_temperature_sensor PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(simple_temperature_sensor PRIVATE ${CMAKE_SOURCE_DIR}/include)
