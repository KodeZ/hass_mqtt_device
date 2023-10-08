# ./examples/simple_on_off_light/CMakeLists.txt

# Define the executable for the example
add_executable(simple_on_off_light main.cpp)

# Link the necessary libraries
target_link_libraries(simple_on_off_light PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(simple_on_off_light PRIVATE ${CMAKE_SOURCE_DIR}/include)
