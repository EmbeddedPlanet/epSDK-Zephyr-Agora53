cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(epSDK_Agora53)

# Define the nested Bosch BSEC static library as an imported target
add_library(bosch_bsec STATIC IMPORTED GLOBAL)

# Tell CMake exactly where to find the libbsec.a file
# (Update this path to point to the Cortex-M4 or M33 version of libbsec.a inside your directory)
set_target_properties(bosch_bsec PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/epSDK/library/libalgobsec.a"
)

# Define epSDK library as an imported global target
add_library(epSDK_lib STATIC IMPORTED GLOBAL)

# Tell CMake where the actual static .a file resides
set_target_properties(epSDK_lib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/epSDK/library/libepSDKZephyr.a"
)

target_include_directories(epSDK_lib INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/epSDK/include"
)

# Bind library to the Zephyr interface and cryptographic system
# This gives static library visibility into nrf's source hooks and headers.
target_link_libraries(epSDK_lib INTERFACE zephyr_interface)

# Link the BSEC library to your custom library target
target_link_libraries(epSDK_lib INTERFACE bosch_bsec)

# Force the ZEPHYROS macro definition to pass down to anything reading epSDK headers
target_compile_definitions(app PUBLIC ZEPHYROS)

# Add source locations
target_sources(app PRIVATE
    src/main.c
    src/sensor_task.c
    src/cellularTask.c
)

# Add header locations
target_include_directories(app PRIVATE
    src
    epSDK/include
)

# Link static libraries
target_link_libraries(app PUBLIC epSDK_lib)