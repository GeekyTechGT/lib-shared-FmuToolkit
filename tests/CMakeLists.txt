include(CTest)
include(GoogleTest)
include(FetchContent)

# GoogleTest + GoogleMock. An installed package (find_package(GTest)) is
# preferred so offline builds work; otherwise the pinned release below is
# downloaded once per build tree.
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    FIND_PACKAGE_ARGS NAMES GTest
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Ported from the pre-scaffold lib-fmu-toolkit: unit/integration tests reach
# into internal classes (FmuValidator, SimulationController, the adapters, ...)
# that carry no LIB_FMU_TOOLKIT_API export annotation, so they cannot be
# resolved through the shared fmutoolkit.dll (CXX_VISIBILITY_PRESET hidden).
# The old project solved this by also building a static library variant and
# linking tests against that; this scaffold's lib-shared category only
# builds one variant (project.json's library_type). Compiling the same
# hexagonal sources directly into each test binary reaches the same result
# without a second library target.
file(GLOB_RECURSE FMUTOOLKIT_LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/domain/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/application/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/adapter/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/config/*.cpp"
)
file(GLOB FMUTOOLKIT_LIB_ROOT_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(APPEND FMUTOOLKIT_LIB_SOURCES ${FMUTOOLKIT_LIB_ROOT_SOURCES})

# gt_add_fmutoolkit_gtest(<name> SOURCES <test files...>)
#
# Like the generic gt_add_gtest() other categories use, plus: the library's
# own sources compiled directly in (see comment above) and third-party deps
# (nlohmann_json, LibXml2, libzip) resolved the same way the fmutoolkit
# target itself resolves them.
function(gt_add_fmutoolkit_gtest name)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    add_executable(${name} ${ARG_SOURCES} ${FMUTOOLKIT_LIB_SOURCES})
    target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/src")
    target_include_directories(${name} PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
        # Where src/CMakeLists.txt's configure_file() writes the generated
        # version.hpp (CMAKE_CURRENT_BINARY_DIR there is <build>/src, not the
        # top-level build dir).
        "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src/generated>"
    )
    target_link_libraries(${name} PRIVATE GTest::gmock GTest::gtest_main)
    gt_target_defaults(${name})
    if(COMMAND gt_enable_sanitizers)
        gt_enable_sanitizers(${name})
    endif()
    fmutoolkit_configure_third_party(${name})

    target_compile_definitions(${name} PRIVATE
        LIB_FMU_TOOLKIT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
        LIB_FMU_TOOLKIT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
    fmutoolkit_copy_winpthread(${name})

    if(WIN32)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:${name}>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:${name}>;$<TARGET_FILE_DIR:${name}>>"
            COMMAND_EXPAND_LISTS
        )
    endif()

    gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()

file(GLOB FMUTOOLKIT_UNIT_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp")
gt_add_fmutoolkit_gtest(fmutoolkit_unit_tests SOURCES ${FMUTOOLKIT_UNIT_TEST_SOURCES})

file(GLOB FMUTOOLKIT_INTEGRATION_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp")
gt_add_fmutoolkit_gtest(fmutoolkit_integration_tests SOURCES ${FMUTOOLKIT_INTEGRATION_TEST_SOURCES})
