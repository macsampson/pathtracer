cmake_minimum_required(VERSION 3.10)
project(raytracing)

set(CMAKE_CXX_STANDARD 17)

find_package(TBB REQUIRED)

add_executable(raytracing main.cpp)
target_compile_options(raytracing PRIVATE -O3)
target_link_libraries(raytracing TBB::tbb)
