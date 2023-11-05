# ./examples/simple_hvac_full_feature/CMakeLists.txt

# Define the executable for the example
add_executable(simple_hvac_full_feature main.cpp)

# Link the necessary libraries
target_link_libraries(simple_hvac_full_feature PRIVATE hass_mqtt_device)

# Include the necessary directories
target_include_directories(simple_hvac_full_feature PRIVATE ${CMAKE_SOURCE_DIR}/include)
