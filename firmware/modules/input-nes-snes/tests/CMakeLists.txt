cmake_minimum_required(VERSION 3.13)
project(chirbot_nes_input_tests LANGUAGES CXX)
enable_testing()
add_executable(test_nes_input
    test_nes_input.cpp
    ../src/nes_input_module.cpp
    ../src/nes_protocol.cpp)
target_compile_features(test_nes_input PRIVATE cxx_std_17)
target_include_directories(test_nes_input PRIVATE
    stubs ../include ../../../chirbot-common/include)
if(MSVC)
    target_compile_options(test_nes_input PRIVATE /W4 /WX)
else()
    target_compile_options(test_nes_input PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME nes_physical_input COMMAND test_nes_input)
