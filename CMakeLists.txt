cmake_minimum_required(VERSION 3.22)

# Project name and C++ standard
project(pogic LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_COMPILE_FLAGS} -O2")

include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

add_executable(pogic
    main.cpp
    khelper.cpp
)

# Include toml++ headers
target_link_libraries(pogic tomlplusplus::tomlplusplus)

# find_package(PkgConfig REQUIRED)
