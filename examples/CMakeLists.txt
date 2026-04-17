cmake_minimum_required(VERSION 3.25)

# ---------------------------------------------------------------------------
# Target selection: prefer the shared library so that examples exercise the
# public DLL API (symbols exported via LIB_FMU_TOOLKIT_API).
# If only the static target was built, fall back gracefully.
# ---------------------------------------------------------------------------
if(TARGET lib_fmu_toolkit::shared)
    set(LIB_FMU_TOOLKIT_EXAMPLE_TARGET lib_fmu_toolkit::shared)
else()
    set(LIB_FMU_TOOLKIT_EXAMPLE_TARGET lib_fmu_toolkit::lib_fmu_toolkit)
endif()

function(lib_fmu_toolkit_configure_example target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE ${LIB_FMU_TOOLKIT_EXAMPLE_TARGET})
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    lib_fmu_toolkit_apply_options(${target_name})
    lib_fmu_toolkit_apply_mingw_static_runtime(${target_name})
    lib_fmu_toolkit_copy_winpthread(${target_name})

    # On Windows, copy the shared library DLL next to the example executable so
    # that it can be run without modifying PATH.
    if(WIN32 AND TARGET lib_fmu_toolkit_shared)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:lib_fmu_toolkit_shared>
                $<TARGET_FILE_DIR:${target_name}>
            COMMENT "Copying lib-fmu-toolkit DLL next to ${target_name}"
        )
    endif()
endfunction()

add_subdirectory(01_basic_simulate)
add_subdirectory(02_step_by_step)
add_subdirectory(03_validate_fmu)
add_subdirectory(04_export_results)
add_subdirectory(05_generatore_sommatore)
add_subdirectory(06_batch_validate)
