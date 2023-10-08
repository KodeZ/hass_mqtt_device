# ./examples/CMakeLists.txt

# Iterate over each subdirectory in the examples folder
file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(child ${children})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
        # Add the subdirectory to the build
        add_subdirectory(${child})
    endif()
endforeach()
