# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXPixels

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXPixels)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

# dependencies
add_subdirectory("${TXLib}/TXGraphics")

add_library(TXPixels STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/pixels.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/pixels.cpp"
)
tx_add_release_ops(TXPixels)
target_link_libraries(TXPixels PUBLIC TXGraphics)

target_include_directories(TXPixels PUBLIC 
	"${TXLib}/TXStdHeader"
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)