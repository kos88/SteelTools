cmake_minimum_required(VERSION 3.5)

project(SteelFaceTools VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../releases)  # global folder for installation
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)  # System variable that contains a path which will be used for "include()" and "find_package()" commands
set(PLUGIN_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_MAYA "Build maya integration" OFF)
option(BUILD_UNREAL "Build ureal integration" OFF)
message(STATUS "DCC targets Maya:${BUILD_MAYA} Unreal: ${BUILD_UNREAL}")

add_subdirectory(core)

if(BUILD_MAYA)
    add_subdirectory(maya)
endif()

if(BUILD_UNREAL)
    add_subdirectory(unreal)
endif()