add_library(chirbot_link_protocol STATIC
    src/link_protocol.c
)

target_include_directories(chirbot_link_protocol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)