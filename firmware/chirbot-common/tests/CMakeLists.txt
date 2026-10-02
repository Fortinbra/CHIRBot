add_executable(test_controller_pipeline test_controller_pipeline.c)
chirbot_enable_strict_warnings(test_controller_pipeline)
target_link_libraries(test_controller_pipeline PRIVATE
    chirbot_controller_tasd
    chirbot_link_protocol
)

add_test(NAME controller_pipeline COMMAND test_controller_pipeline)