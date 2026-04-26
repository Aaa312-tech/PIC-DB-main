cmake_minimum_required(VERSION 3.16)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(pic_pr LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(DEFINED ENV{CONDA_PREFIX})
    include_directories($ENV{CONDA_PREFIX}/include)
    link_directories($ENV{CONDA_PREFIX}/lib)
endif()

find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(fmt REQUIRED)


set(CMAKE_BUILD_TYPE Debug)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Boost REQUIRED)

option(BUILD_PR "Build pr tool" ON)
option(BUILD_PR_DREAMPLACE "Build pr_dreamplace tool" ON)
option(BUILD_PR_LIDAR "Build pr_lidar tool" ON)
option(BUILD_PR_HANAN "Build pr_hanan tool" ON)

add_subdirectory(src)
add_subdirectory(third-party)
