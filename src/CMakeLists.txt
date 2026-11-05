gt_discover_hexagonal_sources(GT_SOURCES)

gt_add_hexagonal_target(fmutoolkit
    CATEGORY lib-shared
    ALIAS fmutoolkit
    SOURCES ${GT_SOURCES}
)

target_include_directories(fmutoolkit PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(fmutoolkit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# Generated version.hpp (ported from the pre-scaffold project: its own
# lib_fmu_toolkit.hpp already #includes "version.hpp", kept as-is rather
# than switched to this scaffold's {snake}::kVersion, which nothing here or
# in tool-FmuRunner references). Generated flat (no libfmu_toolkit/
# subfolder) so it installs alongside the other public headers with no
# special-casing - see the single install(FILES ...) block in the root
# CMakeLists.txt.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/libfmu_toolkit/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version.hpp"
    @ONLY
)
target_include_directories(fmutoolkit PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
)

fmutoolkit_configure_third_party(fmutoolkit)

# Artifact naming convention: libfmutoolkit.dll (+ import library
# libfmutoolkit.dll.a) for lib-shared, libfmutoolkit.a for lib-static. PREFIX /
# IMPORT_PREFIX are pinned so the convention also holds on toolchains whose
# default prefix is empty (MSVC); the deploy folder already separates Debug
# from Release, so no DEBUG_POSTFIX is applied either.
set_target_properties(fmutoolkit PROPERTIES
    OUTPUT_NAME "fmutoolkit"
    PREFIX "lib"
    IMPORT_PREFIX "lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Export visibility uses the pre-scaffold project's own hand-written
# LIB_FMU_TOOLKIT_API macro (include/libfmu_toolkit/export.hpp), not this
# scaffold's generate_export_header()/FMUTOOLKIT_EXPORT default: every public
# header already annotates its API with LIB_FMU_TOOLKIT_API, and switching
# macros would mean editing all of them for no behavioural change.
if(PROJECT_LIBRARY_TYPE STREQUAL "SHARED")
    target_compile_definitions(fmutoolkit
        PUBLIC LIB_FMU_TOOLKIT_SHARED
        PRIVATE LIB_FMU_TOOLKIT_BUILDING_SHARED
    )
endif()
