set(OUTPUT_NES_SNES_TARGET output_nes_snes)

add_executable(${OUTPUT_NES_SNES_TARGET}
    src/main.cpp
)

pico_generate_pio_header(${OUTPUT_NES_SNES_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controller_output.pio
)

target_include_directories(${OUTPUT_NES_SNES_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${OUTPUT_NES_SNES_TARGET} PRIVATE
    pico_stdlib
    hardware_pio
    hardware_spi
    chirbot_link_protocol
    tasd
)

pico_enable_stdio_usb(${OUTPUT_NES_SNES_TARGET} 1)
pico_enable_stdio_uart(${OUTPUT_NES_SNES_TARGET} 0)
pico_add_extra_outputs(${OUTPUT_NES_SNES_TARGET})