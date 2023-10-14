# ./examples/simple_on_off_light_multiple_functions/CMakeLists.txt

# Define the executable for the example
add_executable(simple_on_off_light_multiple_functions main.cpp)

# Link the necessary libraries
target_link_libraries(simple_on_off_light_multiple_functions PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(simple_on_off_light_multiple_functions PRIVATE ${CMAKE_SOURCE_DIR}/include)
