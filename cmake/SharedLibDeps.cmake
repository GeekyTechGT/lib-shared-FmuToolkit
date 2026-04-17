# cmake/SharedLibDeps.cmake
#
# Resolves user-defined shared library dependencies declared in project.json.
#
# Expected layout under shared_lib_base:
#   <shared_lib_base>/<dep_name>/<dep_version>/<preset>/include/   — public headers
#   <shared_lib_base>/<dep_name>/<dep_version>/<preset>/lib/       — static archives (.a)
#   <shared_lib_base>/<dep_name>/<dep_version>/<preset>/bin/       — shared libraries (.dll/.so)
#
# Static archives are preferred to avoid runtime distribution of dependency DLLs.
# If no .a is found, linking falls back to dynamic (.dll/.so) in bin/.
#
# Usage: called automatically from the top-level CMakeLists.txt.
#   lib_fmu_toolkit_resolve_shared_deps()
#
# Variables consumed:
#   LIB_FMU_TOOLKIT_PRESET_NAME  — cmake cache variable set by each CMakePreset
#
# Variables populated (appended to):
#   LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS
#   LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS

function(lib_fmu_toolkit_resolve_shared_deps)
    set(_json_path "${CMAKE_CURRENT_SOURCE_DIR}/project.json")

    if(NOT EXISTS "${_json_path}")
        message(STATUS "[SharedLibDeps] project.json not found — skipping dependency resolution")
        return()
    endif()

    file(READ "${_json_path}" _json_content)

    # ── shared_lib_base ────────────────────────────────────────────────────────
    string(JSON _shared_lib_base ERROR_VARIABLE _err GET "${_json_content}" "shared_lib_base")
    if(_err OR NOT _shared_lib_base)
        message(STATUS "[SharedLibDeps] 'shared_lib_base' not defined in project.json — skipping")
        return()
    endif()

    # ── preset name ───────────────────────────────────────────────────────────
    if(NOT LIB_FMU_TOOLKIT_PRESET_NAME)
        message(STATUS "[SharedLibDeps] LIB_FMU_TOOLKIT_PRESET_NAME not set — skipping")
        return()
    endif()

    # ── dependencies array ────────────────────────────────────────────────────
    string(JSON _deps_count ERROR_VARIABLE _err LENGTH "${_json_content}" "dependencies")
    if(_err OR _deps_count EQUAL 0)
        message(STATUS "[SharedLibDeps] No dependencies declared in project.json")
        return()
    endif()

    message(STATUS "[SharedLibDeps] Resolving ${_deps_count} dependency(-ies) for preset '${LIB_FMU_TOOLKIT_PRESET_NAME}'")

    math(EXPR _last_idx "${_deps_count} - 1")
    set(_extra_includes "")
    set(_extra_libs "")

    foreach(_idx RANGE 0 ${_last_idx})
        string(JSON _dep_name    GET "${_json_content}" "dependencies" ${_idx} "name")
        string(JSON _dep_version GET "${_json_content}" "dependencies" ${_idx} "version")

        set(_dep_root "${_shared_lib_base}/${_dep_name}/${_dep_version}/${LIB_FMU_TOOLKIT_PRESET_NAME}")
        set(_dep_include "${_dep_root}/include")
        set(_dep_lib     "${_dep_root}/lib")
        set(_dep_bin     "${_dep_root}/bin")

        # ── validate existence ─────────────────────────────────────────────────
        if(NOT EXISTS "${_dep_root}")
            message(FATAL_ERROR
                "[SharedLibDeps] Dependency '${_dep_name}' v${_dep_version} not found.\n"
                "  Expected path: ${_dep_root}\n"
                "  Deploy the dependency first using its project_manager.py → Deploy option."
            )
        endif()

        # ── include directory ─────────────────────────────────────────────────
        if(EXISTS "${_dep_include}")
            list(APPEND _extra_includes "${_dep_include}")
            message(STATUS "  [${_dep_name}] include: ${_dep_include}")
        else()
            message(WARNING "[SharedLibDeps] '${_dep_name}' include dir not found: ${_dep_include}")
        endif()

        # ── static archives (preferred — avoids runtime DLL distribution) ──────
        set(_found_static FALSE)
        if(EXISTS "${_dep_lib}")
            file(GLOB _archives "${_dep_lib}/*.a")
            if(_archives)
                foreach(_archive IN LISTS _archives)
                    list(APPEND _extra_libs "${_archive}")
                    message(STATUS "  [${_dep_name}] static: ${_archive}")
                endforeach()
                set(_found_static TRUE)
            endif()
        endif()

        # ── fallback: import lib or shared lib in bin/ ────────────────────────
        if(NOT _found_static)
            if(EXISTS "${_dep_bin}")
                if(WIN32)
                    file(GLOB _import_libs "${_dep_bin}/*.dll.a" "${_dep_bin}/*.lib")
                    if(_import_libs)
                        foreach(_ilib IN LISTS _import_libs)
                            list(APPEND _extra_libs "${_ilib}")
                            message(STATUS "  [${_dep_name}] import: ${_ilib}")
                        endforeach()
                    else()
                        file(GLOB _dlls "${_dep_bin}/*.dll")
                        foreach(_dll IN LISTS _dlls)
                            list(APPEND _extra_libs "${_dll}")
                            message(WARNING "  [${_dep_name}] fallback DLL (requires distribution): ${_dll}")
                        endforeach()
                    endif()
                else()
                    file(GLOB _shared_libs "${_dep_bin}/*.so*")
                    foreach(_so IN LISTS _shared_libs)
                        list(APPEND _extra_libs "${_so}")
                        message(WARNING "  [${_dep_name}] fallback .so (requires distribution): ${_so}")
                    endforeach()
                endif()
            endif()
        endif()
    endforeach()

    # ── propagate into parent scope variables ─────────────────────────────────
    list(APPEND LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS ${_extra_includes})
    list(APPEND LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS     ${_extra_libs})

    set(LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS "${LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS}" PARENT_SCOPE)
    set(LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS    "${LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS}"    PARENT_SCOPE)
endfunction()
