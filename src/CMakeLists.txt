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

if(PROJECT_LIBRARY_TYPE STREQUAL "SHARED")
    include(GenerateExportHeader)
    generate_export_header(fmutoolkit
        BASE_NAME FMUTOOLKIT
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/generated/include/fmutoolkit/fmutoolkit_export.hpp"
    )
    target_include_directories(fmutoolkit PUBLIC
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/generated/include/fmutoolkit/fmutoolkit_export.hpp"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/fmutoolkit"
    )
endif()
