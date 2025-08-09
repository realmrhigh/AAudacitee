cmake_minimum_required(VERSION 3.22.1)

project("audioapp")

# Find required packages
find_package(oboe REQUIRED CONFIG)

# Add DSP source files
file(GLOB DSP_SOURCES 
    "dsp/*.cpp"
    "dsp/*.h"
)

add_library( # Sets the name of the library.
        audioapp

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        native-lib.cpp
        ${DSP_SOURCES}
        AudioBuffer.h
        CircularBuffer.h
        LockFreeQueue.h
        ObjectPool.h)

# Include directories for headers
target_include_directories(audioapp PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dsp
    ${CMAKE_SOURCE_DIR}/avst
)

target_link_libraries( # Specifies the target library.
        audioapp

        # Links the target library to the log library
        # and allows it to call functions from that library.
        oboe::oboe
        log)
