cmake_minimum_required(VERSION 3.22.1)

project("audioapp")

# Find Oboe
find_package(oboe REQUIRED CONFIG)

# Add libebur128
add_subdirectory(libebur128)

# Add LAME
add_subdirectory(lame)

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
        ebur128
        lame
        log)

# Add the parametric EQ plugin
add_library(parametric_eq SHARED
        plugins/parametric_eq/ParametricEQ.cpp
        dsp/Biquad.cpp
        dsp/FFT.cpp
        dsp/FrequencyResponse.cpp)

target_link_libraries(parametric_eq log)

# Add the compressor plugin
add_library(compressor SHARED
        plugins/compressor/CompressorPlugin.cpp
        dsp/Compressor.cpp)

target_link_libraries(compressor log)

# Add the leveler plugin
add_library(leveler SHARED
        plugins/leveler/LevelerPlugin.cpp
        dsp/Leveler.cpp)

target_link_libraries(leveler log)

# Add the transient shaper plugin
add_library(transient_shaper SHARED
        plugins/transient_shaper/TransientShaperPlugin.cpp
        dsp/TransientShaper.cpp)

target_link_libraries(transient_shaper log)
