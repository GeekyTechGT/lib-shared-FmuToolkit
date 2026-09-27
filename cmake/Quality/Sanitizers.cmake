if(DEFINED GT_SANITIZERS_INCLUDED)
    return()
endif()
set(GT_SANITIZERS_INCLUDED TRUE)

function(gt_enable_sanitizers target)
    if(NOT DEFINED PROJECT_IDENTIFIER_UPPER)
        return()
    endif()
    if(NOT ${PROJECT_IDENTIFIER_UPPER}_ENABLE_SANITIZERS)
        return()
    endif()

    if(MSVC)
        # Compile-only: MSVC's /fsanitize=address does not have a separate
        # link-time flag.
        target_compile_options(${target} PUBLIC /fsanitize=address)
    else()
        # PUBLIC (not PRIVATE): a static library archiving these flags needs
        # them to propagate to whoever links it, or the __asan_/__ubsan_
        # symbols fail to resolve.
        target_compile_options(${target} PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PUBLIC -fsanitize=address,undefined)
    endif()
endfunction()
