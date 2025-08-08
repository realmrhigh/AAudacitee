# CMakeLists.txt for LAME

# Add the library as a static library
add_library(lame STATIC libmp3lame.c)

# Expose the include directory
target_include_directories(lame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
