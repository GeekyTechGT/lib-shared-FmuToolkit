cmake_minimum_required(VERSION 3.25)

# --- Read project.json's version before project(), with a generation-time
#     fallback if the file is missing, unparsable, or not in a form project()
#     accepts (MAJOR.MINOR.PATCH). ---
set(gt_project_json "${CMAKE_CURRENT_SOURCE_DIR}/project.json")
set(gt_project_version "0.1.0")

if(EXISTS "${gt_project_json}")
    file(READ "${gt_project_json}" gt_project_json_content)
    string(JSON gt_parsed_version ERROR_VARIABLE gt_json_error GET "${gt_project_json_content}" "version")
    if(NOT gt_json_error AND gt_parsed_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
        set(gt_project_version "${gt_parsed_version}")
    endif()
endif()

project(fmutoolkit
    VERSION ${gt_project_version}
    LANGUAGES CXX
)

set(PROJECT_IDENTIFIER "fmutoolkit")
string(TOUPPER "${PROJECT_IDENTIFIER}" PROJECT_IDENTIFIER_UPPER)
set(PROJECT_CATEGORY "lib-shared")
set(PROJECT_LANGUAGE "CXX")
set(GT_CMAKE_ROOT "${CMAKE_SOURCE_DIR}/cmake")

list(APPEND CMAKE_MODULE_PATH
    "${GT_CMAKE_ROOT}/Utilities"
    "${GT_CMAKE_ROOT}/Project"
    "${GT_CMAKE_ROOT}/Dependencies"
    "${GT_CMAKE_ROOT}/Quality"
    "${GT_CMAKE_ROOT}/Hexagonal"
    "${GT_CMAKE_ROOT}/Install"
)

include(PreventInSourceBuilds)
include(ProjectConfiguration)
include(TargetDefaults)
include(PresetConfiguration)
include(LibraryOptions)
include(Sanitizers)
include(Coverage)
include(Dependencies)
include(ThirdParty)
include(FmuToolkitThirdParty)
include(HexagonalTarget)
include(InstallTargets)
include(VersionHeader)

gt_configure_shared_dependencies(PROJECT_JSON "${gt_project_json}")

add_subdirectory(src)

if(${PROJECT_IDENTIFIER_UPPER}_BUILD_EXAMPLES AND EXISTS "${CMAKE_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(${PROJECT_IDENTIFIER_UPPER}_BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

if(PROJECT_CATEGORY MATCHES "^lib-(shared|static)$")
    # No INCLUDE_DIRS: gt_install_target()'s default install(DIRECTORY ...)
    # would preserve the include/libfmu_toolkit/ subfolder. Public headers
    # are ported from the pre-scaffold project, which installs them FLAT
    # (no subfolder) - tool-FmuRunner's already-working
    # `#include <lib_fmu_toolkit.hpp>` depends on that exact layout, so it
    # is kept here rather than switched to this scaffold's default
    # include/fmutoolkit/ convention.
    gt_install_target(${PROJECT_IDENTIFIER}
        CATEGORY ${PROJECT_CATEGORY}
    )
    install(
        FILES
            include/libfmu_toolkit/export.hpp
            include/libfmu_toolkit/exceptions.hpp
            include/libfmu_toolkit/fmu_generation.hpp
            include/libfmu_toolkit/fmu_engine.hpp
            include/libfmu_toolkit/lib_fmu_toolkit.hpp
            include/libfmu_toolkit/types.hpp
            "${CMAKE_BINARY_DIR}/src/generated/version.hpp"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )
endif()
