cmake_minimum_required(VERSION 3.20)
project(SlideShow)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(SlideShow ${SOURCES})

# find libzip from vcpkg
find_package(libzip CONFIG REQUIRED)
target_link_libraries(SlideShow PRIVATE libzip::zip)
