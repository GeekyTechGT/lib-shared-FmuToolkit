include("${CMAKE_CURRENT_LIST_DIR}/CompilerWarnings.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Sanitizers.cmake")

function(lib_fmu_toolkit_apply_options target)
    if(LIB_FMU_TOOLKIT_ENABLE_WARNINGS)
        lib_fmu_toolkit_enable_warnings(${target})
    endif()

    lib_fmu_toolkit_enable_sanitizers(${target})
endfunction()
