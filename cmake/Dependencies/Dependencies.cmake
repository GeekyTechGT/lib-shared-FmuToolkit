if(DEFINED GT_DEPENDENCIES_INCLUDED)
    return()
endif()
set(GT_DEPENDENCIES_INCLUDED TRUE)

# Library base name of a dependency: the deployed artifacts are
# lib<base>.dll / lib<base>.dll.a / lib<base>.a, and <base> is also the
# imported CMake target name (the `if(TARGET <base>)` guard consumers use).
# Derived from the dependency name by stripping the lib-shared-/lib-static-
# category prefix, removing separators and lower-casing:
#   lib-shared-QtCustomWidgets -> qtcustomwidgets
# An explicit `"library": "<base>"` in project.json overrides the derivation.
function(gt_dependency_library_name out_var dependency_name)
    string(REGEX REPLACE "^lib-(shared|static)-" "" base "${dependency_name}")
    string(REGEX REPLACE "[^A-Za-z0-9]" "" base "${base}")
    string(TOLOWER "${base}" base)
    set(${out_var} "${base}" PARENT_SCOPE)
endfunction()

# Resolves a shared-artifact dependency root directory.
#   1. <deploy_folder>/<name>/<version>/<variant>/     (shared drive deploy)
#   2. <project source>/external/<name>/               (offline fallback)
function(gt_find_shared_dependency)
    set(one_value_args NAME VERSION DEPLOY_FOLDER VARIANT OUT_ROOT OUT_FOUND)
    cmake_parse_arguments(ARG "" "${one_value_args}" "" ${ARGN})

    set(deployed_root "${ARG_DEPLOY_FOLDER}/${ARG_NAME}/${ARG_VERSION}/${ARG_VARIANT}")
    set(offline_root "${CMAKE_SOURCE_DIR}/external/${ARG_NAME}")

    if(EXISTS "${deployed_root}")
        set(${ARG_OUT_ROOT} "${deployed_root}" PARENT_SCOPE)
        set(${ARG_OUT_FOUND} TRUE PARENT_SCOPE)
    elseif(EXISTS "${offline_root}")
        set(${ARG_OUT_ROOT} "${offline_root}" PARENT_SCOPE)
        set(${ARG_OUT_FOUND} TRUE PARENT_SCOPE)
    else()
        set(${ARG_OUT_ROOT} "" PARENT_SCOPE)
        set(${ARG_OUT_FOUND} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(_gt_link_qt_modules target_name)
    set(qt_link_libs "")
    foreach(qt_module Core Gui Widgets Svg SvgWidgets Pdf PdfWidgets)
        if(TARGET Qt6::${qt_module})
            list(APPEND qt_link_libs "Qt6::${qt_module}")
        endif()
    endforeach()
    if(qt_link_libs)
        set_property(TARGET ${target_name} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${qt_link_libs}")
    endif()
endfunction()

# Creates the imported target <LIBRARY> for a dependency deployed under ROOT
# (bin/ lib/ include/ layout). Lookup order, first match wins:
#   1. lib/lib<base>.dll.a (+ bin/lib<base>.dll)  -> SHARED  (MinGW lib-shared)
#   2. lib/lib<base>.so                            -> SHARED  (Linux lib-shared)
#   3. lib/lib<base>.a                             -> STATIC  (lib-static)
#   4. lib/<base>.a                                -> STATIC  (pre-convention layout)
function(gt_create_imported_dependency_target NAME ROOT LIBRARY)
    set(target_name "${LIBRARY}")
    set(implib "${ROOT}/lib/lib${LIBRARY}.dll.a")
    set(dll "${ROOT}/bin/lib${LIBRARY}.dll")
    set(shared_object "${ROOT}/lib/lib${LIBRARY}.so")
    set(archive "${ROOT}/lib/lib${LIBRARY}.a")
    set(legacy_archive "${ROOT}/lib/${LIBRARY}.a")

    if(TARGET ${target_name})
        return()
    endif()

    if(EXISTS "${implib}")
        if(EXISTS "${dll}")
            add_library(${target_name} SHARED IMPORTED GLOBAL)
            set_target_properties(${target_name} PROPERTIES
                IMPORTED_LOCATION "${dll}"
                IMPORTED_IMPLIB "${implib}"
            )
        else()
            # Import library without its DLL: linkable, but the executable
            # will only run once the DLL is deployed next to it.
            add_library(${target_name} UNKNOWN IMPORTED GLOBAL)
            set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION "${implib}")
        endif()
        set(kind "shared")
    elseif(EXISTS "${shared_object}")
        add_library(${target_name} SHARED IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION "${shared_object}")
        set(kind "shared")
    elseif(EXISTS "${archive}")
        add_library(${target_name} STATIC IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION "${archive}")
        set(kind "static")
    elseif(EXISTS "${legacy_archive}")
        add_library(${target_name} STATIC IMPORTED GLOBAL)
        set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION "${legacy_archive}")
        set(kind "static, legacy naming")
    else()
        if(EXISTS "${ROOT}/bin")
            # Runtime-only dependency: nothing to link against, but
            # prj_exec.py's Deploy still copies it (project.json copy_to).
            message(STATUS "Dependency '${NAME}': runtime-only (no lib${LIBRARY}.dll.a/.a under ${ROOT}/lib).")
        else()
            message(STATUS "Dependency '${NAME}': nothing importable found under ${ROOT}.")
        endif()
        return()
    endif()

    set_target_properties(${target_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${ROOT}/include")
    _gt_link_qt_modules(${target_name})
    message(STATUS "Dependency '${NAME}': imported target '${target_name}' (${kind}) from ${ROOT}")

    # Makes build-tree executables runnable without a deploy step: the DLL
    # is copied next to them (CMAKE_RUNTIME_OUTPUT_DIRECTORY).
    if(WIN32 AND kind STREQUAL "shared" AND EXISTS "${dll}" AND DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY
       AND NOT TARGET copy_${target_name}_dll)
        add_custom_target(copy_${target_name}_dll ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/"
            COMMENT "Copying lib${LIBRARY}.dll next to the built executables"
        )
    endif()
endfunction()

# Reads project.json's deploy_folder + dependencies[] and creates an imported
# target for each dependency it can find. GT_DEPS_VARIANT (set by every
# preset to "${presetName}") is the only way at configure time to know which
# preset is active - it is used as the variant folder to look up.
function(gt_configure_shared_dependencies)
    set(one_value_args PROJECT_JSON)
    cmake_parse_arguments(ARG "" "${one_value_args}" "" ${ARGN})

    # Establish the dependency variant folder unconditionally (not just once a
    # dependency is actually found) so the "-DGT_DEPS_VARIANT=..." every
    # preset passes always counts as used, even for a project with none yet.
    if(NOT DEFINED GT_DEPS_VARIANT)
        set(GT_DEPS_VARIANT "${CMAKE_BUILD_TYPE}")
    endif()
    message(STATUS "gt_configure_shared_dependencies: variant '${GT_DEPS_VARIANT}'")

    if(NOT EXISTS "${ARG_PROJECT_JSON}")
        message(STATUS "gt_configure_shared_dependencies: no project.json at ${ARG_PROJECT_JSON}, skipping.")
        return()
    endif()

    file(READ "${ARG_PROJECT_JSON}" project_json_content)

    string(JSON deploy_folder ERROR_VARIABLE json_error GET "${project_json_content}" "deploy_folder")
    if(json_error)
        message(STATUS "gt_configure_shared_dependencies: no deploy_folder in project.json, skipping.")
        return()
    endif()

    string(JSON dependency_count ERROR_VARIABLE json_error LENGTH "${project_json_content}" "dependencies")
    if(json_error OR dependency_count EQUAL 0)
        return()
    endif()

    math(EXPR last_index "${dependency_count} - 1")
    foreach(index RANGE ${last_index})
        string(JSON dependency GET "${project_json_content}" "dependencies" ${index})
        string(JSON dep_name GET "${dependency}" "name")
        string(JSON dep_version GET "${dependency}" "version")
        string(JSON dep_library ERROR_VARIABLE json_error GET "${dependency}" "library")
        if(json_error OR dep_library STREQUAL "" OR dep_library STREQUAL "null" OR dep_library MATCHES "NOTFOUND$")
            gt_dependency_library_name(dep_library "${dep_name}")
        endif()

        gt_find_shared_dependency(
            NAME "${dep_name}"
            VERSION "${dep_version}"
            DEPLOY_FOLDER "${deploy_folder}"
            VARIANT "${GT_DEPS_VARIANT}"
            OUT_ROOT dep_root
            OUT_FOUND dep_found
        )

        if(dep_found)
            gt_create_imported_dependency_target("${dep_name}" "${dep_root}" "${dep_library}")
        else()
            message(STATUS "Dependency '${dep_name}' (${dep_version}) not found for variant '${GT_DEPS_VARIANT}'. "
                            "Consumers must guard usage with if(TARGET ${dep_library}).")
        endif()
    endforeach()
endfunction()
