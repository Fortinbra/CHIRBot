# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pimoroni_pga2350 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/CMakeLists.txt)

project(input_nes_snes)

# Initialize the SDK
pico_sdk_init()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/nes_input_module.cpp
    src/nes_protocol.cpp
    src/spi_subnode.cpp
    src/tasd_wrapper.cpp
)

# Pull in our includes
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enable pico_stdlib
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    pico_multicore
)

# Enable USB
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Add TASD library
add_subdirectory(../../TASD TASD)
target_link_libraries(${PROJECT_NAME} PRIVATE tasd)

# Add chirbot-common when available
# add_subdirectory(../../chirbot-common chirbot-common)
# target_link_libraries(${PROJECT_NAME} PRIVATE chirbot-common)

# Create map file
pico_add_extra_outputs(${PROJECT_NAME})
