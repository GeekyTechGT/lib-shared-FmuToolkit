cmake_minimum_required(VERSION 3.25)
project(your_library_example LANGUAGES CXX)

add_executable(minimal_example main.cpp)

if(TARGET your_library::your_library)
    target_link_libraries(minimal_example PRIVATE your_library::your_library)
else()
    find_package(your_library REQUIRED)
    target_link_libraries(minimal_example PRIVATE your_library::your_library)
endif()

target_compile_features(minimal_example PRIVATE cxx_std_23)
