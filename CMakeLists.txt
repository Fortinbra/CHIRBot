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

project(chirbot)

pico_sdk_init()

# Add firmware
add_subdirectory(firmware)
