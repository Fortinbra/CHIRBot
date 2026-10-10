add_executable(test_controller_output test_controller_output.cpp)
target_include_directories(test_controller_output PRIVATE ../include
    ../../../chirbot-common/include)
if(MSVC)
    target_compile_options(test_controller_output PRIVATE /W4 /WX)
else()
    target_compile_options(test_controller_output PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME controller_output COMMAND test_controller_output
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/controller_output.pio
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/controller_playback.pio
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/controller_latch_filter.pio)
