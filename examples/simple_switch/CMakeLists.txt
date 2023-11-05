# ./examples/simple_switch/CMakeLists.txt

# Define the executable for the example
add_executable(simple_switch main.cpp)

# Link the necessary libraries
target_link_libraries(simple_switch PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(simple_switch PRIVATE ${CMAKE_SOURCE_DIR}/include)
