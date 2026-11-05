cmake_minimum_required(VERSION 3.25)

# Examples exercise the public DLL API only (symbols exported via
# LIB_FMU_TOOLKIT_API) - never the internal domain/application/adapter
# sources directly, unlike tests/CMakeLists.txt.
function(fmutoolkit_configure_example target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE fmutoolkit)
    target_compile_definitions(${target_name} PRIVATE LIB_FMU_TOOLKIT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    gt_target_defaults(${target_name})
    fmutoolkit_apply_mingw_static_runtime(${target_name})
    fmutoolkit_copy_winpthread(${target_name})

    # fmutoolkit's RUNTIME_OUTPUT_DIRECTORY is build/<preset>/lib/ (see
    # src/CMakeLists.txt), not this executable's own output directory - copy
    # the DLL next to it so the example runs without touching PATH.
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:fmutoolkit>
                $<TARGET_FILE_DIR:${target_name}>
            COMMENT "Copying libfmutoolkit DLL next to ${target_name}"
        )
    endif()
endfunction()

add_subdirectory(01_basic_simulate)
add_subdirectory(02_step_by_step)
add_subdirectory(03_validate_fmu)
add_subdirectory(04_export_results)
add_subdirectory(05_generatore_sommatore)
add_subdirectory(06_batch_validate)
add_subdirectory(07_generate_from_json)
add_subdirectory(08_generate_selective)
