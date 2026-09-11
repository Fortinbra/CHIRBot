set(INPUT_NES_SNES_TARGET input_nes_snes)

add_executable(${INPUT_NES_SNES_TARGET}
    src/main.cpp
    src/nes_input_module.cpp
    src/nes_protocol.cpp
    src/spi_subnode.cpp
    src/tasd_wrapper.cpp
)

# Pull in our includes
target_include_directories(${INPUT_NES_SNES_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enable pico_stdlib
target_link_libraries(${INPUT_NES_SNES_TARGET} PRIVATE
    pico_stdlib
    hardware_spi
    chirbot_link_protocol
    tasd
)

# Enable USB
pico_enable_stdio_usb(${INPUT_NES_SNES_TARGET} 1)
pico_enable_stdio_uart(${INPUT_NES_SNES_TARGET} 0)

# Add chirbot-common when available
# add_subdirectory(../../chirbot-common chirbot-common)
# target_link_libraries(${PROJECT_NAME} PRIVATE chirbot-common)

# Create map file
pico_add_extra_outputs(${INPUT_NES_SNES_TARGET})
