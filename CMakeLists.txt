cmake_minimum_required(VERSION 3.10)
project(raytracing)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

find_package(TBB REQUIRED)
find_package(OpenImageDenoise REQUIRED)

add_executable(raytracing src/main.cpp)

target_include_directories(raytracing PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

target_compile_options(raytracing PRIVATE -O3)
target_link_libraries(raytracing TBB::tbb OpenImageDenoise)
