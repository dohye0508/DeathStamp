cmake_minimum_required(VERSION 3.21)
set(PROJECT_NAME DeathStamp)
project(${PROJECT_NAME} VERSION 1.0.0)

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK environment variable is not defined. Install the Geode CLI and run `geode config setup` first.")
else()
    message(STATUS "Found GEODE_SDK: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
