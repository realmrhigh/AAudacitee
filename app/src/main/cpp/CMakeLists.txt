cmake_minimum_required(VERSION 3.22.1)

project("audioapp")

# Find Oboe
find_package(oboe REQUIRED CONFIG)

add_library( # Sets the name of the library.
        audioapp

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        native-lib.cpp)

target_link_libraries( # Specifies the target library.
        audioapp

        # Links the target library to the log library
        # and allows it to call functions from that library.
        oboe::oboe
        log)

# Add the parametric EQ plugin
add_library(parametric_eq SHARED
        plugins/parametric_eq/ParametricEQ.cpp
        dsp/Biquad.cpp
        dsp/FFT.cpp
        dsp/FrequencyResponse.cpp)

target_link_libraries(parametric_eq log)
