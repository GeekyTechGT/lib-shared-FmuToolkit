if(DEFINED GT_LIBRARY_OPTIONS_INCLUDED)
    return()
endif()
set(GT_LIBRARY_OPTIONS_INCLUDED TRUE)

function(gt_define_project_options)
    if(NOT DEFINED PROJECT_IDENTIFIER_UPPER)
        message(FATAL_ERROR "gt_define_project_options() requires PROJECT_IDENTIFIER_UPPER to be set.")
    endif()

    option(${PROJECT_IDENTIFIER_UPPER}_BUILD_TESTS "Build the test suite" ON)
    option(${PROJECT_IDENTIFIER_UPPER}_BUILD_EXAMPLES "Build the examples" ON)
    option(${PROJECT_IDENTIFIER_UPPER}_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)
    option(${PROJECT_IDENTIFIER_UPPER}_ENABLE_LTO "Enable link-time optimization (IPO)" OFF)

    message(STATUS "${PROJECT_NAME} options:")
    message(STATUS "  BUILD_TESTS       = ${${PROJECT_IDENTIFIER_UPPER}_BUILD_TESTS}")
    message(STATUS "  BUILD_EXAMPLES    = ${${PROJECT_IDENTIFIER_UPPER}_BUILD_EXAMPLES}")
    message(STATUS "  ENABLE_SANITIZERS = ${${PROJECT_IDENTIFIER_UPPER}_ENABLE_SANITIZERS}")
    message(STATUS "  ENABLE_LTO        = ${${PROJECT_IDENTIFIER_UPPER}_ENABLE_LTO}")
endfunction()

# Release presets set <PROJECT>_ENABLE_LTO=ON. Enabled globally through
# CMAKE_INTERPROCEDURAL_OPTIMIZATION (so every target defined afterwards gets
# -flto), and only after the toolchain confirms support - an unsupported
# toolchain degrades to a warning instead of a broken link.
function(gt_configure_link_time_optimization)
    if(NOT ${PROJECT_IDENTIFIER_UPPER}_ENABLE_LTO)
        return()
    endif()

    include(CheckIPOSupported)
    if(DEFINED PROJECT_LANGUAGE AND PROJECT_LANGUAGE STREQUAL "C")
        set(ipo_languages C)
    else()
        set(ipo_languages CXX)
    endif()
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output LANGUAGES ${ipo_languages})
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON PARENT_SCOPE)
        # GCC's -flto emits slim objects (LTO bytecode only) by default; a
        # deployed static archive must also carry real machine code so it
        # links with any compatible toolchain, with or without LTO.
        if(PROJECT_LIBRARY_TYPE STREQUAL "STATIC" AND CMAKE_${ipo_languages}_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-ffat-lto-objects)
        endif()
        message(STATUS "Link-time optimization: enabled")
    else()
        message(WARNING "Link-time optimization requested but not supported by this toolchain: ${ipo_output}")
    endif()
endfunction()

gt_define_project_options()
gt_configure_link_time_optimization()
