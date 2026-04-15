function(lib_fmu_toolkit_enable_sanitizers target)
    if(NOT LIB_FMU_TOOLKIT_ENABLE_SANITIZERS)
        return()
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        return()
    endif()

    if(MSVC)
        return()
    endif()

    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()
