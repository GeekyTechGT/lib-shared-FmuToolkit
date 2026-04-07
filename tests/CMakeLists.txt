find_package(GTest CONFIG QUIET)

set(_your_library_gtest_target GTest::gtest_main)
set(_your_library_need_fetch_gtest OFF)
set(_your_library_gtest_reason "")
set(_your_library_gtest_location "")

if(NOT GTest_FOUND)
    set(_your_library_need_fetch_gtest ON)
    set(_your_library_gtest_reason "GoogleTest was not found in the active CMake environment")
else()
    foreach(_prop
        IMPORTED_LOCATION
        IMPORTED_IMPLIB
        IMPORTED_LOCATION_DEBUG
        IMPORTED_IMPLIB_DEBUG
        IMPORTED_LOCATION_RELEASE
        IMPORTED_IMPLIB_RELEASE)
        get_target_property(_value GTest::gtest_main ${_prop})
        if(_value AND NOT _value MATCHES "-NOTFOUND")
            set(_your_library_gtest_location "${_value}")
            break()
        endif()
    endforeach()

    if(MINGW AND _your_library_gtest_location MATCHES [[\.lib$]])
        set(_your_library_need_fetch_gtest ON)
        set(_your_library_gtest_reason
            "Detected an incompatible non-MinGW GoogleTest binary at '${_your_library_gtest_location}'")
    endif()
endif()

if(_your_library_need_fetch_gtest)
    include(FetchContent)

    message(STATUS
        "${_your_library_gtest_reason}. Fetching googletest so tests build with the active toolchain.")

    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    if(MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    )
    FetchContent_MakeAvailable(googletest)
    set(_your_library_gtest_target gtest_main)
endif()

file(GLOB_RECURSE YOUR_LIBRARY_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER YOUR_LIBRARY_TEST_SOURCES EXCLUDE REGEX "/CMakeLists\.txt$")

if(NOT YOUR_LIBRARY_TEST_SOURCES)
    message(FATAL_ERROR "No test source files were found under tests/.")
endif()

add_executable(your_library_tests ${YOUR_LIBRARY_TEST_SOURCES})
add_executable(your_library::tests ALIAS your_library_tests)

target_link_libraries(
    your_library_tests
    PRIVATE
        your_library::your_library
        ${_your_library_gtest_target}
)

target_include_directories(your_library_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_compile_features(your_library_tests PRIVATE cxx_std_23)
your_library_apply_options(your_library_tests)

include(GoogleTest)
gtest_discover_tests(your_library_tests)
