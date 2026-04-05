cmake_minimum_required(VERSION 3.25)

project(
    your_library
    VERSION 0.1.0
    DESCRIPTION "Professional C++23 library scaffold with PIMPL, tests, install, and package support"
    LANGUAGES CXX
)

include(GNUInstallDirs)

option(YOUR_LIBRARY_BUILD_SHARED "Build the library as shared" OFF)
option(YOUR_LIBRARY_BUILD_TESTS "Build unit tests" ON)
option(YOUR_LIBRARY_ENABLE_WARNINGS "Enable strict warning flags" ON)
option(YOUR_LIBRARY_ENABLE_WERROR "Treat warnings as errors" OFF)
option(YOUR_LIBRARY_ENABLE_SANITIZERS "Enable sanitizers for supported debug builds" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(YOUR_LIBRARY_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()

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

add_library(your_library)
add_library(your_library::your_library ALIAS your_library)

target_sources(
    your_library
    PRIVATE
        src/yourlib.cpp
        src/yourlib_impl.hpp
)

target_include_directories(
    your_library
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(your_library PUBLIC cxx_std_23)

set_target_properties(
    your_library
    PROPERTIES
        OUTPUT_NAME "your_library"
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME your_library
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
)

if(WIN32 AND BUILD_SHARED_LIBS)
    target_compile_definitions(
        your_library
        PUBLIC YOUR_LIBRARY_SHARED
        PRIVATE YOUR_LIBRARY_BUILDING_SHARED
    )
endif()

target_sources(
    your_library
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_BINARY_DIR}/generated"
        FILES
            include/yourlib/export.hpp
            include/yourlib/yourlib.hpp
            "${CMAKE_CURRENT_BINARY_DIR}/generated/yourlib/version.hpp"
)

your_library_apply_options(your_library)

install(
    TARGETS your_library
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
