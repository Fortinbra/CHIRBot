set(FATFS_SOURCE_DIR ${PICO_SDK_PATH}/lib/tinyusb/lib/fatfs/source)
set(FATFS_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/fatfs)
file(MAKE_DIRECTORY ${FATFS_BUILD_DIR})
configure_file(${FATFS_SOURCE_DIR}/ff.c ${FATFS_BUILD_DIR}/ff.c COPYONLY)
configure_file(${FATFS_SOURCE_DIR}/ff.h ${FATFS_BUILD_DIR}/ff.h COPYONLY)
configure_file(${FATFS_SOURCE_DIR}/diskio.h ${FATFS_BUILD_DIR}/diskio.h COPYONLY)
configure_file(${FATFS_SOURCE_DIR}/ffunicode.c ${FATFS_BUILD_DIR}/ffunicode.c COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ffconf.h ${FATFS_BUILD_DIR}/ffconf.h COPYONLY)

# Console is the physical UART (via a debug probe or similar adapter); no
# USB console/mass storage for now, to keep bring-up simple.
add_executable(chirbot_core
	src/main.cpp
	src/st7735_display.cpp
	src/splash_screen.cpp
	src/sd_probe.cpp
	src/sd_storage.cpp
	src/menu.cpp
	${FATFS_BUILD_DIR}/ff.c
	${FATFS_BUILD_DIR}/ffunicode.c
)

target_include_directories(chirbot_core PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${FATFS_BUILD_DIR}
	${FATFS_SOURCE_DIR}
)

target_link_libraries(chirbot_core PRIVATE
	pico_stdlib
	hardware_spi
	chirbot_link_protocol
	chirbot_controller_tasd
	tasd
)

pico_enable_stdio_uart(chirbot_core 1)
pico_enable_stdio_usb(chirbot_core 0)
pico_add_extra_outputs(chirbot_core)
