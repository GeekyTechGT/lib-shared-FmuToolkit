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

# gt_add_gtest(<name> SOURCES <files...> LIBRARIES <targets...>)
#
# One test executable linked against GoogleTest/GoogleMock (gtest_main
# provides main()). The name must end in _unit_tests / _integration_tests:
# prj_exec.py's deploy filters those out of the deploy folder.
function(gt_add_gtest name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE ${ARG_LIBRARIES} GTest::gmock GTest::gtest_main)
    gt_target_defaults(${name})
    if(COMMAND gt_enable_sanitizers)
        gt_enable_sanitizers(${name})
    endif()

    # Windows has no rpath: copy the DLLs the test links (a lib-shared
    # library, Qt, ...) next to the executable. Guarded because
    # `copy_if_different` with no source files is an error.
    if(WIN32)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:${name}>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:${name}>;$<TARGET_FILE_DIR:${name}>>"
            COMMAND_EXPAND_LISTS
        )
    endif()

    # PRE_TEST: tests are listed when ctest runs (where scripts/prj_run_tests.py
    # sets up PATH), not at build time - no DLL lookup needed while building.
    gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()

foreach(suite unit integration)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${suite}/CMakeLists.txt")
        add_subdirectory(${suite})
    endif()
endforeach()
