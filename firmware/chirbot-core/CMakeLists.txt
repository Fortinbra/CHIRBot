add_executable(chirbot_core
	src/main.cpp
)

target_include_directories(chirbot_core PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chirbot_core PRIVATE
	pico_stdlib
	hardware_spi
	chirbot_link_protocol
	tasd
)

pico_enable_stdio_uart(chirbot_core 1)
pico_enable_stdio_usb(chirbot_core 0)
pico_add_extra_outputs(chirbot_core)
