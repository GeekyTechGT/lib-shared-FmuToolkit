if(DEFINED GT_INSTALL_TARGETS_INCLUDED)
    return()
endif()
set(GT_INSTALL_TARGETS_INCLUDED TRUE)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install component holding the find_package() package config
# (lib/cmake/<name>/*.cmake). It is EXCLUDE_FROM_ALL: a plain
# `cmake --install` - which is what scripts/prj_exec.py's Deploy runs - never
# writes a cmake/ folder into the deployed lib/ directory. Consumers of this
# scaffold resolve dependencies from the bin/ lib/ include/ layout instead
# (cmake/Dependencies/Dependencies.cmake). Opt in explicitly when a classic
# find_package() consumer needs it:
#   cmake --install build/<preset> --prefix <dir> --component cmake_package
set(GT_CMAKE_PACKAGE_COMPONENT "cmake_package")

function(gt_install_library_config target)
    set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
    )

    if(NOT DEFINED GT_CMAKE_ROOT)
        message(FATAL_ERROR "gt_install_library_config: GT_CMAKE_ROOT is not set (should point to <project>/cmake).")
    endif()

    configure_package_config_file(
        "${GT_CMAKE_ROOT}/Templates/PackageConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION "${config_install_dir}"
    )

    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "${config_install_dir}"
        COMPONENT ${GT_CMAKE_PACKAGE_COMPONENT}
        EXCLUDE_FROM_ALL
    )

    install(
        EXPORT ${PROJECT_NAME}Targets
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${config_install_dir}"
        COMPONENT ${GT_CMAKE_PACKAGE_COMPONENT}
        EXCLUDE_FROM_ALL
    )
endfunction()

function(gt_install_target name)
    set(one_value_args CATEGORY)
    set(multi_value_args INCLUDE_DIRS)
    cmake_parse_arguments(ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(ARG_CATEGORY MATCHES "^(app|tool|catalog|container|svc-cpp|app-qml)$")
        install(TARGETS ${name} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    elseif(ARG_CATEGORY MATCHES "^lib-(shared|static)$")
        # Deployed layout: bin/ (DLL), lib/ (import/static library),
        # include/ (public headers).
        install(
            TARGETS ${name}
            EXPORT ${PROJECT_NAME}Targets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )

        foreach(include_dir IN LISTS ARG_INCLUDE_DIRS)
            install(
                DIRECTORY "${include_dir}/"
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
                FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
            )
        endforeach()

        gt_install_library_config(${name})
    else()
        message(FATAL_ERROR "gt_install_target: unknown CATEGORY '${ARG_CATEGORY}'.")
    endif()
endfunction()
