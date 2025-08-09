# CMakeLists.txt for libebur128

# Add the library as a static library
add_library(ebur128 STATIC ebur128.c)

# Expose the include directory
target_include_directories(ebur128 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
