if(DEFINED GT_PREVENT_IN_SOURCE_BUILDS_INCLUDED)
    return()
endif()
set(GT_PREVENT_IN_SOURCE_BUILDS_INCLUDED TRUE)

function(gt_prevent_in_source_builds)
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
        message(FATAL_ERROR
            "In-source builds are not supported. Use a preset or a dedicated "
            "build directory, e.g.: cmake --preset linux-gcc-debug"
        )
    endif()
endfunction()

gt_prevent_in_source_builds()
