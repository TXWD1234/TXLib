# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
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

add_library(TXGraphics STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/graphics.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/basic_gl_utils.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/buffer_manager.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/vertex_attribute_manager.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/shader.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/shader_manager.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/draw_call_manager.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/texture_manager.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/fence.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/buffer.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/render_engine.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/log.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/rgb.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/render_engine.cpp"
)
tx_add_release_ops(TXGraphics)
target_link_libraries(TXGraphics PUBLIC 
	glfw
	glad
	OpenGL::GL
	TXMath
	TXUtility
)

target_compile_features(TXGraphics PUBLIC cxx_std_20)

target_include_directories(TXGraphics PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)