# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXGraphics

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXGraphics)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

# dependencies
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependency/glad")

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Automatically collect all nested headers and source files
file(GLOB_RECURSE TX_GRAPHICS_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE TX_GRAPHICS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(TXGraphics STATIC ${TX_GRAPHICS_HEADERS} ${TX_GRAPHICS_SOURCES})

tx_add_release_ops(TXGraphics)
target_compile_features(TXGraphics PUBLIC cxx_std_20)
target_link_libraries(TXGraphics PUBLIC 
	glfw
	glad
	OpenGL::GL
	TXMath
	TXUtility
	TXData
)


target_include_directories(TXGraphics PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)