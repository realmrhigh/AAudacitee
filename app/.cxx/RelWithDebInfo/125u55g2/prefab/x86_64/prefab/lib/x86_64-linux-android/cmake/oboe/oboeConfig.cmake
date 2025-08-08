if(NOT TARGET oboe::oboe)
add_library(oboe::oboe SHARED IMPORTED)
set_target_properties(oboe::oboe PROPERTIES
    IMPORTED_LOCATION "C:/Users/stant/.gradle/caches/transforms-3/750cd355f74f293a14287285b6361954/transformed/oboe-1.5.0/prefab/modules/oboe/libs/android.x86_64/liboboe.so"
    INTERFACE_INCLUDE_DIRECTORIES "C:/Users/stant/.gradle/caches/transforms-3/750cd355f74f293a14287285b6361954/transformed/oboe-1.5.0/prefab/modules/oboe/include"
    INTERFACE_LINK_LIBRARIES ""
)
endif()

