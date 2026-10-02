cmake_minimum_required(VERSION 3.13)

if(NOT COMMAND chirbot_enable_strict_warnings)
    function(chirbot_enable_strict_warnings target)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /WX)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
        endif()
    endfunction()
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(chirbot_common LANGUAGES C)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    add_subdirectory(../TASD ${CMAKE_CURRENT_BINARY_DIR}/tasd)
    enable_testing()
    set(CHIRBOT_COMMON_BUILD_TESTS ON)
endif()

add_library(chirbot_link_protocol STATIC
    src/link_protocol.c
)
chirbot_enable_strict_warnings(chirbot_link_protocol)

target_include_directories(chirbot_link_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(chirbot_controller_tasd STATIC
    src/controller_tasd.c
)
chirbot_enable_strict_warnings(chirbot_controller_tasd)

target_include_directories(chirbot_controller_tasd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chirbot_controller_tasd PUBLIC tasd)

# Pico-only: depends on the SDK's SPI/GPIO HAL, so it is skipped for host builds.
if(TARGET hardware_spi)
    add_library(chirbot_spi_subnode STATIC
        src/spi_subnode.c
    )
    chirbot_enable_strict_warnings(chirbot_spi_subnode)

    target_include_directories(chirbot_spi_subnode PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(chirbot_spi_subnode PUBLIC
        pico_stdlib
        hardware_spi
    )
endif()

if(CHIRBOT_COMMON_BUILD_TESTS)
    add_subdirectory(tests)
endif()