cmake_minimum_required(VERSION 3.25)

project(
    lib_fmu_toolkit
    VERSION 1.0.0
    DESCRIPTION "C++ toolkit for loading, parsing, validating, and simulating FMI 2.0/3.0 FMUs"
    LANGUAGES CXX
)

include(GNUInstallDirs)

option(LIB_FMU_TOOLKIT_BUILD_SHARED "Build the shared library target" ON)
option(LIB_FMU_TOOLKIT_BUILD_STATIC "Build the static library target" ON)
option(LIB_FMU_TOOLKIT_BUILD_TESTS "Build unit and integration tests" ON)
option(LIB_FMU_TOOLKIT_BUILD_EXAMPLES "Build minimal example applications" ON)
option(LIB_FMU_TOOLKIT_ENABLE_WARNINGS "Enable strict warning flags" ON)
option(LIB_FMU_TOOLKIT_ENABLE_WERROR "Treat warnings as errors" OFF)
option(LIB_FMU_TOOLKIT_ENABLE_SANITIZERS "Enable sanitizers for supported debug builds" OFF)
option(LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME "Statically link MinGW C++ runtime libraries when using MinGW" ON)

# Name of the active CMake preset (injected automatically by each configurePreset).
# Used by SharedLibDeps to locate user dependencies in the shared library folder.
set(LIB_FMU_TOOLKIT_PRESET_NAME "" CACHE STRING "Active CMakePresets preset name (set automatically)")

if(NOT LIB_FMU_TOOLKIT_BUILD_SHARED AND NOT LIB_FMU_TOOLKIT_BUILD_STATIC)
    message(FATAL_ERROR "At least one of LIB_FMU_TOOLKIT_BUILD_SHARED or LIB_FMU_TOOLKIT_BUILD_STATIC must be ON")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

include(cmake/ProjectOptions.cmake)
include(cmake/SharedLibDeps.cmake)

function(lib_fmu_toolkit_apply_mingw_static_runtime target_name)
    if(MINGW AND LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME)
        # -static-libgcc / -static-libstdc++ ensure the GCC C++ runtime
        # (libstdc++-6.dll, libgcc_s_seh-1.dll) is baked into the binary so it
        # does not need to be distributed.  winpthread is linked statically where
        # possible; if libxml2.a or libzip.a pull in further pthread references
        # after the -Bdynamic switch, libwinpthread-1.dll may still appear as a
        # transitive dependency — distribute it alongside your DLL in that case.
        target_link_options(
            ${target_name}
            PRIVATE
                -static-libgcc
                -static-libstdc++
                -Wl,-Bstatic
                -lwinpthread
                -Wl,-Bdynamic
        )
    endif()
endfunction()

# Copy libwinpthread-1.dll next to an EXECUTABLE target at build time.
# Must only be called for executable targets (not libraries) to avoid nested
# cmd.exe /C quoting issues in the generated ninja build rules on Windows.
function(lib_fmu_toolkit_copy_winpthread target_name)
    if(MINGW AND LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME AND WIN32)
        get_filename_component(_mingw_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(_winpthread_src "${_mingw_bin}/libwinpthread-1.dll")
        if(EXISTS "${_winpthread_src}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_winpthread_src}"
                    "$<TARGET_FILE_DIR:${target_name}>"
                COMMENT "Copying libwinpthread-1.dll next to ${target_name}"
            )
        endif()
    endif()
endfunction()

find_package(LibXml2 REQUIRED)
find_package(PkgConfig REQUIRED)

set(LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS "")
set(LIB_FMU_TOOLKIT_EXTRA_LINK_DIRS "")
set(LIB_FMU_TOOLKIT_EXTRA_COMPILE_DEFINITIONS "")
set(LIB_FMU_TOOLKIT_EXTRA_LINK_OPTIONS "")
set(LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS "")
set(LIB_FMU_TOOLKIT_EXTRA_SYSTEM_LINK_LIBS "")
set(LIB_FMU_TOOLKIT_XML_LINK_LIBS LibXml2::LibXml2)

if(MINGW AND LIB_FMU_TOOLKIT_MINGW_STATIC_RUNTIME)
    pkg_check_modules(LIBXML2_PKG REQUIRED IMPORTED_TARGET libxml-2.0)
    pkg_check_modules(LIBZIP_PKG REQUIRED IMPORTED_TARGET libzip)

    set(LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS
        ${LIBXML2_PKG_INCLUDE_DIRS}
        ${LIBZIP_PKG_INCLUDE_DIRS}
    )
    set(LIB_FMU_TOOLKIT_EXTRA_COMPILE_DEFINITIONS LIBXML_STATIC ZIP_STATIC)
    set(LIB_FMU_TOOLKIT_XML_LINK_LIBS "")
    set(LIB_FMU_TOOLKIT_LIBZIP_TARGET "")

    set(_lib_fmu_toolkit_static_lib_names
        ${LIBXML2_PKG_STATIC_LIBRARIES}
        ${LIBZIP_PKG_STATIC_LIBRARIES}
    )
    list(REMOVE_DUPLICATES _lib_fmu_toolkit_static_lib_names)

    set(_lib_fmu_toolkit_windows_system_libs
        advapi32
        bcrypt
        crypt32
        gdi32
        iphlpapi
        kernel32
        ncrypt
        netapi32
        ole32
        oleaut32
        rpcrt4
        secur32
        shell32
        user32
        userenv
        uuid
        version
        winhttp
        wininet
        winmm
        ws2_32
    )

    foreach(_lib_name IN LISTS _lib_fmu_toolkit_static_lib_names)
        if(_lib_name IN_LIST _lib_fmu_toolkit_windows_system_libs)
            list(APPEND LIB_FMU_TOOLKIT_EXTRA_SYSTEM_LINK_LIBS ${_lib_name})
            continue()
        endif()
        set(_lib_fmu_toolkit_static_archive "")
        foreach(_lib_dir IN LISTS LIBXML2_PKG_STATIC_LIBRARY_DIRS LIBZIP_PKG_STATIC_LIBRARY_DIRS)
            if(EXISTS "${_lib_dir}/lib${_lib_name}.a")
                set(_lib_fmu_toolkit_static_archive "${_lib_dir}/lib${_lib_name}.a")
                break()
            endif()
            if(EXISTS "${_lib_dir}/${_lib_name}.a")
                set(_lib_fmu_toolkit_static_archive "${_lib_dir}/${_lib_name}.a")
                break()
            endif()
        endforeach()
        if(_lib_fmu_toolkit_static_archive)
            list(APPEND LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS ${_lib_fmu_toolkit_static_archive})
        else()
            list(APPEND LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS ${_lib_name})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES LIB_FMU_TOOLKIT_EXTRA_SYSTEM_LINK_LIBS)
else()
    find_package(unofficial-libzip CONFIG QUIET)
    if(unofficial-libzip_FOUND)
        set(LIB_FMU_TOOLKIT_LIBZIP_TARGET unofficial::libzip::libzip)
    else()
        pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)
        set(LIB_FMU_TOOLKIT_LIBZIP_TARGET PkgConfig::LIBZIP)
    endif()
endif()

# Resolve user-defined dependencies from the shared library folder (_shared_lib).
# This appends to LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS and LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS.
lib_fmu_toolkit_resolve_shared_deps()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/libfmu_toolkit")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/libfmu_toolkit/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/libfmu_toolkit/version.hpp"
    @ONLY
)

file(GLOB_RECURSE LIB_FMU_TOOLKIT_SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(NOT LIB_FMU_TOOLKIT_SOURCE_FILES)
    message(FATAL_ERROR "No library source files were found under src/.")
endif()

set(LIB_FMU_TOOLKIT_PUBLIC_HEADERS
    include/libfmu_toolkit/export.hpp
    include/libfmu_toolkit/exceptions.hpp
    include/libfmu_toolkit/fmu_engine.hpp
    include/libfmu_toolkit/lib_fmu_toolkit.hpp
    include/libfmu_toolkit/types.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/generated/libfmu_toolkit/version.hpp"
)

function(lib_fmu_toolkit_configure_library target_name library_type)
    add_library(${target_name} ${library_type})

    target_sources(
        ${target_name}
        PRIVATE
            ${LIB_FMU_TOOLKIT_SOURCE_FILES}
        PUBLIC
            FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS
                "${CMAKE_CURRENT_SOURCE_DIR}/include"
                "${CMAKE_CURRENT_BINARY_DIR}/generated"
            FILES
                ${LIB_FMU_TOOLKIT_PUBLIC_HEADERS}
    )

    target_include_directories(
        ${target_name}
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
            ${LIB_FMU_TOOLKIT_EXTRA_INCLUDE_DIRS}
    )

    target_compile_features(${target_name} PUBLIC cxx_std_20)

    # Pin the libstdc++ C++11 ABI (non-COW string/list ABI introduced in GCC 5).
    # Without this, code compiled with _GLIBCXX_USE_CXX11_ABI=0 (e.g. legacy
    # GCC4 environments or old distro packages) would silently use a different
    # std::string layout, causing heap corruption across the library boundary.
    # This propagates to consumers via target_link_libraries() so they remain
    # ABI-consistent with the library.
    # On Windows/MinGW the same libstdc++ ABI switch applies; on MSVC this
    # define is unknown and harmless (the MSVC block is never reached anyway
    # due to ADR-0003).
    if(NOT MSVC)
        target_compile_definitions(${target_name} PUBLIC _GLIBCXX_USE_CXX11_ABI=1)
    endif()

    if(library_type STREQUAL "STATIC")
        set(_lib_fmu_toolkit_dependency_scope PUBLIC)
    else()
        set(_lib_fmu_toolkit_dependency_scope PRIVATE)
    endif()

    set_target_properties(
        ${target_name}
        PROPERTIES
            OUTPUT_NAME "lib-fmu-toolkit"
            PREFIX ""
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${PROJECT_VERSION_MAJOR}"
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN YES
    )

    if(library_type STREQUAL "SHARED")
        target_compile_definitions(
            ${target_name}
            PUBLIC LIB_FMU_TOOLKIT_SHARED
            PRIVATE LIB_FMU_TOOLKIT_BUILDING_SHARED
        )
    endif()

    target_compile_definitions(
        ${target_name}
        PRIVATE
            LIB_FMU_TOOLKIT_SOURCE_EXTERNAL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/external"
            ${LIB_FMU_TOOLKIT_EXTRA_COMPILE_DEFINITIONS}
    )

    target_link_directories(
        ${target_name}
        ${_lib_fmu_toolkit_dependency_scope}
            ${LIB_FMU_TOOLKIT_EXTRA_LINK_DIRS}
    )

    target_link_options(
        ${target_name}
        ${_lib_fmu_toolkit_dependency_scope}
            ${LIB_FMU_TOOLKIT_EXTRA_LINK_OPTIONS}
    )

    target_link_libraries(
        ${target_name}
        ${_lib_fmu_toolkit_dependency_scope}
            ${LIB_FMU_TOOLKIT_XML_LINK_LIBS}
            ${LIB_FMU_TOOLKIT_LIBZIP_TARGET}
            ${LIB_FMU_TOOLKIT_EXTRA_LINK_LIBS}
            ${LIB_FMU_TOOLKIT_EXTRA_SYSTEM_LINK_LIBS}
            ${CMAKE_DL_LIBS}
    )

    lib_fmu_toolkit_apply_options(${target_name})
    lib_fmu_toolkit_apply_mingw_static_runtime(${target_name})
endfunction()

set(_lib_fmu_toolkit_primary_target "")
set(_lib_fmu_toolkit_install_targets "")

if(LIB_FMU_TOOLKIT_BUILD_SHARED)
    lib_fmu_toolkit_configure_library(lib_fmu_toolkit_shared SHARED)
    add_library(lib_fmu_toolkit::shared ALIAS lib_fmu_toolkit_shared)
    list(APPEND _lib_fmu_toolkit_install_targets lib_fmu_toolkit_shared)
    if(NOT _lib_fmu_toolkit_primary_target)
        set(_lib_fmu_toolkit_primary_target lib_fmu_toolkit_shared)
    endif()
endif()

if(LIB_FMU_TOOLKIT_BUILD_STATIC)
    lib_fmu_toolkit_configure_library(lib_fmu_toolkit_static STATIC)
    add_library(lib_fmu_toolkit::static ALIAS lib_fmu_toolkit_static)
    list(APPEND _lib_fmu_toolkit_install_targets lib_fmu_toolkit_static)
    if(NOT _lib_fmu_toolkit_primary_target)
        set(_lib_fmu_toolkit_primary_target lib_fmu_toolkit_static)
    endif()
endif()

add_library(lib_fmu_toolkit::lib_fmu_toolkit ALIAS ${_lib_fmu_toolkit_primary_target})

# Install library binaries and import libs.
# Do NOT install FILE_SET here — headers are installed flat below.
install(
    TARGETS ${_lib_fmu_toolkit_install_targets}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# Install public headers flat into include/ (no libfmu_toolkit/ subdirectory).
# Consumers add <prefix>/include to their include path and use e.g.
#   #include "fmu_engine.hpp"
install(
    FILES ${LIB_FMU_TOOLKIT_PUBLIC_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

if(LIB_FMU_TOOLKIT_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

if(LIB_FMU_TOOLKIT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
