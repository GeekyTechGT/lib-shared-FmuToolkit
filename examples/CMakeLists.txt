add_executable(fmutoolkit_example_basic basic_usage.cpp)
target_link_libraries(fmutoolkit_example_basic PRIVATE fmutoolkit)
gt_target_defaults(fmutoolkit_example_basic)

set_target_properties(fmutoolkit_example_basic PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples"
)
