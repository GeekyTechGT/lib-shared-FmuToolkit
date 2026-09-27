if(DEFINED GT_TARGET_DEFAULTS_INCLUDED)
    return()
endif()
set(GT_TARGET_DEFAULTS_INCLUDED TRUE)

function(gt_target_defaults target)
    if(DEFINED PROJECT_LANGUAGE AND PROJECT_LANGUAGE STREQUAL "C")
        target_compile_features(${target} PUBLIC c_std_17)
    else()
        target_compile_features(${target} PUBLIC cxx_std_23)
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /Zc:__cplusplus /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -fPIC)
    endif()

    if(COMMAND gt_enable_coverage)
        gt_enable_coverage(${target})
    endif()

    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "SHARED_LIBRARY")
        set_target_properties(${target} PROPERTIES
            CXX_VISIBILITY_PRESET hidden
            C_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
        )
    endif()
endfunction()

function(gt_add_namespace_alias target namespace)
    if(NOT TARGET ${namespace}::${target})
        add_library(${namespace}::${target} ALIAS ${target})
    endif()
endfunction()
