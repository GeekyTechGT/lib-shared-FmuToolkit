cmake_minimum_required(VERSION 3.25)

# ---------------------------------------------------------------------------
# Select the library target that examples link against.
# The static target is preferred because it makes the example binaries fully
# self-contained (no need to copy shared library DLLs alongside the exe).
# ---------------------------------------------------------------------------
if(TARGET lib_fmu_toolkit::static)
    set(LIB_FMU_TOOLKIT_EXAMPLE_TARGET lib_fmu_toolkit::static)
else()
    set(LIB_FMU_TOOLKIT_EXAMPLE_TARGET lib_fmu_toolkit::lib_fmu_toolkit)
endif()

function(lib_fmu_toolkit_configure_example target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE ${LIB_FMU_TOOLKIT_EXAMPLE_TARGET})
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    lib_fmu_toolkit_apply_options(${target_name})
    lib_fmu_toolkit_apply_mingw_static_runtime(${target_name})
endfunction()

add_subdirectory(01_basic_simulate)
add_subdirectory(02_step_by_step)
add_subdirectory(03_validate_fmu)
add_subdirectory(04_export_results)
add_subdirectory(05_generatore_sommatore)
add_subdirectory(06_batch_validate)
