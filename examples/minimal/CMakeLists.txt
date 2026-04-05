cmake_minimum_required(VERSION 3.25)
project(your_library_example LANGUAGES CXX)

find_package(your_library REQUIRED)

add_executable(minimal_example main.cpp)
target_link_libraries(minimal_example PRIVATE your_library::your_library)
target_compile_features(minimal_example PRIVATE cxx_std_23)
