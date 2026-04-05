find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)

    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(your_library_tests tests_greeter.cpp)
add_executable(your_library::tests ALIAS your_library_tests)

target_link_libraries(
    your_library_tests
    PRIVATE
        your_library::your_library
        GTest::gtest_main
)

target_compile_features(your_library_tests PRIVATE cxx_std_23)
your_library_apply_options(your_library_tests)

include(GoogleTest)
gtest_discover_tests(your_library_tests)
