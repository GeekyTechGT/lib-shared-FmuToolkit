find_package(GTest CONFIG QUIET)

set(_lib_fmu_toolkit_gtest_target GTest::gtest_main)
set(_lib_fmu_toolkit_need_fetch_gtest OFF)
set(_lib_fmu_toolkit_gtest_reason "")
set(_lib_fmu_toolkit_gtest_location "")

if(NOT GTest_FOUND)
    set(_lib_fmu_toolkit_need_fetch_gtest ON)
    set(_lib_fmu_toolkit_gtest_reason "GoogleTest was not found in the active CMake environment")
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
            set(_lib_fmu_toolkit_gtest_location "${_value}")
            break()
        endif()
    endforeach()

    if(MINGW AND _lib_fmu_toolkit_gtest_location MATCHES [[\.lib$]])
        set(_lib_fmu_toolkit_need_fetch_gtest ON)
        set(
            _lib_fmu_toolkit_gtest_reason
            "Detected an incompatible non-MinGW GoogleTest binary at '${_lib_fmu_toolkit_gtest_location}'"
        )
    endif()
endif()

if(_lib_fmu_toolkit_need_fetch_gtest)
    include(FetchContent)

    message(
        STATUS
        "${_lib_fmu_toolkit_gtest_reason}. Fetching googletest so tests build with the active toolchain."
    )

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
    set(_lib_fmu_toolkit_gtest_target gtest_main)
endif()

file(GLOB_RECURSE LIB_FMU_TOOLKIT_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
)
list(FILTER LIB_FMU_TOOLKIT_TEST_SOURCES EXCLUDE REGEX "/CMakeLists\.txt$")

if(NOT LIB_FMU_TOOLKIT_TEST_SOURCES)
    message(FATAL_ERROR "No test source files were found under tests/.")
endif()

add_executable(lib_fmu_toolkit_tests ${LIB_FMU_TOOLKIT_TEST_SOURCES})
add_executable(lib_fmu_toolkit::tests ALIAS lib_fmu_toolkit_tests)

# Unit tests include internal headers directly (src/) — they must link against
# the static target so that internal symbols (FmuValidator, SimulationController,
# etc.) are visible without needing DLL export annotations.
# Integration tests that only use the public API could use the shared target,
# but keeping everything on the static target keeps the CMakeLists simple.
if(LIB_FMU_TOOLKIT_BUILD_STATIC)
    set(_lib_fmu_toolkit_test_link_target lib_fmu_toolkit::static)
else()
    set(_lib_fmu_toolkit_test_link_target lib_fmu_toolkit::lib_fmu_toolkit)
endif()

target_link_libraries(
    lib_fmu_toolkit_tests
    PRIVATE
        ${_lib_fmu_toolkit_test_link_target}
        ${_lib_fmu_toolkit_gtest_target}
)

target_include_directories(lib_fmu_toolkit_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_compile_features(lib_fmu_toolkit_tests PRIVATE cxx_std_20)
lib_fmu_toolkit_apply_options(lib_fmu_toolkit_tests)
lib_fmu_toolkit_apply_mingw_static_runtime(lib_fmu_toolkit_tests)

include(GoogleTest)
gtest_discover_tests(lib_fmu_toolkit_tests)

target_compile_definitions(lib_fmu_toolkit_tests PRIVATE LIB_FMU_TOOLKIT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
