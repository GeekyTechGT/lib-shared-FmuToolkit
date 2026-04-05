function(your_library_enable_sanitizers target)
    if(NOT YOUR_LIBRARY_ENABLE_SANITIZERS)
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
