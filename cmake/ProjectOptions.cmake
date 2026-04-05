include("${CMAKE_CURRENT_LIST_DIR}/CompilerWarnings.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Sanitizers.cmake")

function(your_library_apply_options target)
    if(YOUR_LIBRARY_ENABLE_WARNINGS)
        your_library_enable_warnings(${target})
    endif()

    your_library_enable_sanitizers(${target})
endfunction()
