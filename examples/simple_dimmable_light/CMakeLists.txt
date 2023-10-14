# ./examples/simple_dimmable_light/CMakeLists.txt

# Define the executable for the example
add_executable(simple_dimmable_light main.cpp)

# Link the necessary libraries
target_link_libraries(simple_dimmable_light PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(simple_dimmable_light PRIVATE ${CMAKE_SOURCE_DIR}/include)
