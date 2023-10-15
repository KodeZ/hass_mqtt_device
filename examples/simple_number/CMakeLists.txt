# ./examples/simple_number/CMakeLists.txt

# Define the executable for the example
add_executable(simple_number main.cpp)

# Link the necessary libraries
target_link_libraries(simple_number PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(simple_number PRIVATE ${CMAKE_SOURCE_DIR}/include)
