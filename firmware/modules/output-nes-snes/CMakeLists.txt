cmake_minimum_required(VERSION 3.13)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(controller_output_tests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    enable_testing()
    add_subdirectory(tests)
    return()
endif()

set(OUTPUT_NES_SNES_TARGET output_nes_snes)

add_executable(${OUTPUT_NES_SNES_TARGET}
    src/main.cpp
)
chirbot_enable_strict_warnings(${OUTPUT_NES_SNES_TARGET})
target_compile_definitions(${OUTPUT_NES_SNES_TARGET} PRIVATE
    PICO_STDIO_USB_STDOUT_TIMEOUT_US=0
    PICO_DEFAULT_UART=1
    PICO_DEFAULT_UART_TX_PIN=6
    PICO_DEFAULT_UART_RX_PIN=5
    PICO_DEFAULT_UART_BAUD_RATE=115200
)

pico_generate_pio_header(${OUTPUT_NES_SNES_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controller_output.pio
)
pico_generate_pio_header(${OUTPUT_NES_SNES_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controller_playback.pio
)
pico_generate_pio_header(${OUTPUT_NES_SNES_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controller_latch_filter.pio
)

target_include_directories(${OUTPUT_NES_SNES_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${OUTPUT_NES_SNES_TARGET} PRIVATE
    pico_stdlib
    hardware_pio
    hardware_spi
    chirbot_controller_tasd
    chirbot_link_protocol
    chirbot_spi_subnode
    tasd
)

pico_enable_stdio_usb(${OUTPUT_NES_SNES_TARGET} 1)
pico_enable_stdio_uart(${OUTPUT_NES_SNES_TARGET} 1)
pico_add_extra_outputs(${OUTPUT_NES_SNES_TARGET})