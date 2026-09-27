if(DEFINED GT_VERSION_HEADER_INCLUDED)
    return()
endif()
set(GT_VERSION_HEADER_INCLUDED TRUE)

# Reads project.json's "version" field at configure time and generates
# <build>/generated/include/<snake>/version.hpp. Bumping project.json.version
# + reconfiguring (no regeneration needed) is enough for kVersion to reflect
# the new value everywhere it's read from (window title, About dialog, ...).
function(gt_generate_version_header target)
    if(NOT DEFINED GT_CMAKE_ROOT)
        message(FATAL_ERROR "gt_generate_version_header: GT_CMAKE_ROOT is not set (should point to <project>/cmake).")
    endif()

    set(project_json "${CMAKE_SOURCE_DIR}/project.json")
    set(GT_VERSION_HEADER_VALUE "0.0.0")

    if(EXISTS "${project_json}")
        file(READ "${project_json}" project_json_content)
        string(JSON parsed_version ERROR_VARIABLE json_error GET "${project_json_content}" "version")
        if(NOT json_error AND parsed_version)
            set(GT_VERSION_HEADER_VALUE "${parsed_version}")
        endif()
    endif()

    set(GT_VERSION_HEADER_NAMESPACE "${PROJECT_IDENTIFIER}")
    set(generated_dir "${CMAKE_BINARY_DIR}/generated/include/${PROJECT_IDENTIFIER}")

    configure_file(
        "${GT_CMAKE_ROOT}/Utilities/version.hpp.in"
        "${generated_dir}/version.hpp"
        @ONLY
    )

    target_include_directories(${target} PUBLIC "${CMAKE_BINARY_DIR}/generated/include")
endfunction()
