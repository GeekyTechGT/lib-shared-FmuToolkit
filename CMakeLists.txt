cmake_minimum_required(VERSION 3.25)

project(
    your_library
    VERSION 0.1.0
    DESCRIPTION "Reusable C++23 library scaffold with installable CMake package, tests, examples, and vcpkg support"
    LANGUAGES CXX
)

include(GNUInstallDirs)

option(YOUR_LIBRARY_BUILD_SHARED "Build the shared library target" ON)
option(YOUR_LIBRARY_BUILD_STATIC "Build the static library target" ON)
option(YOUR_LIBRARY_BUILD_TESTS "Build unit tests" ON)
option(YOUR_LIBRARY_ENABLE_WARNINGS "Enable strict warning flags" ON)
option(YOUR_LIBRARY_ENABLE_WERROR "Treat warnings as errors" OFF)
option(YOUR_LIBRARY_ENABLE_SANITIZERS "Enable sanitizers for supported debug builds" OFF)
option(YOUR_LIBRARY_MINGW_STATIC_RUNTIME "Statically link MinGW C++ runtime libraries when using MinGW" ON)

if(NOT YOUR_LIBRARY_BUILD_SHARED AND NOT YOUR_LIBRARY_BUILD_STATIC)
    message(FATAL_ERROR "At least one of YOUR_LIBRARY_BUILD_SHARED or YOUR_LIBRARY_BUILD_STATIC must be ON")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

include(cmake/ProjectOptions.cmake)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/yourlib")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/yourlib/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/yourlib/version.hpp"
    @ONLY
)

file(GLOB_RECURSE YOUR_LIBRARY_SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

if(NOT YOUR_LIBRARY_SOURCE_FILES)
    message(FATAL_ERROR "No library source files were found under src/.")
endif()

set(YOUR_LIBRARY_PUBLIC_HEADERS
    include/yourlib/yourlib.hpp
    include/yourlib/export.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/generated/yourlib/version.hpp"
)

function(your_library_configure_library target_name library_type)
    add_library(${target_name} ${library_type})

    target_sources(
        ${target_name}
        PRIVATE
            ${YOUR_LIBRARY_SOURCE_FILES}
        PUBLIC
            FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS
                "${CMAKE_CURRENT_SOURCE_DIR}/include"
                "${CMAKE_CURRENT_BINARY_DIR}/generated"
            FILES
                ${YOUR_LIBRARY_PUBLIC_HEADERS}
    )

    target_include_directories(
        ${target_name}
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
    )

    target_compile_features(${target_name} PUBLIC cxx_std_23)

    set_target_properties(
        ${target_name}
        PROPERTIES
            VERSION "${PROJECT_VERSION}"
            SOVERSION "${PROJECT_VERSION_MAJOR}"
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN YES
    )

    if(library_type STREQUAL "SHARED")
        set_target_properties(
            ${target_name}
            PROPERTIES
                OUTPUT_NAME "your_library"
                EXPORT_NAME your_library
        )
        if(WIN32)
            target_compile_definitions(
                ${target_name}
                PUBLIC YOUR_LIBRARY_SHARED
                PRIVATE YOUR_LIBRARY_BUILDING_SHARED
            )
        endif()
    else()
        set_target_properties(
            ${target_name}
            PROPERTIES
                OUTPUT_NAME "your_library_static"
                EXPORT_NAME your_library_static
        )
    endif()

    your_library_apply_options(${target_name})
endfunction()

set(_your_library_primary_target "")
set(_your_library_install_targets "")

if(YOUR_LIBRARY_BUILD_SHARED)
    your_library_configure_library(your_library_shared SHARED)
    add_library(your_library::your_library_shared ALIAS your_library_shared)
    list(APPEND _your_library_install_targets your_library_shared)
    if(NOT _your_library_primary_target)
        set(_your_library_primary_target your_library_shared)
    endif()
endif()

if(YOUR_LIBRARY_BUILD_STATIC)
    your_library_configure_library(your_library_static STATIC)
    add_library(your_library::your_library_static ALIAS your_library_static)
    list(APPEND _your_library_install_targets your_library_static)
    if(NOT _your_library_primary_target)
        set(_your_library_primary_target your_library_static)
    endif()
endif()

add_library(your_library::your_library ALIAS ${_your_library_primary_target})

install(
    TARGETS ${_your_library_install_targets}
    EXPORT your_libraryTargets
    FILE_SET public_headers
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/your_libraryConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/your_libraryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/your_libraryConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/your_library"
)

install(
    EXPORT your_libraryTargets
    FILE your_libraryTargets.cmake
    NAMESPACE your_library::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/your_library"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/your_libraryConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/your_libraryConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/your_library"
)

if(YOUR_LIBRARY_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

add_subdirectory(examples/minimal)
