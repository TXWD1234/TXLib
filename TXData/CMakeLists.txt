# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXData

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXData)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

add_library(TXData STATIC
"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/data.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/parted_arr.h"
"${CMAKE_CURRENT_SOURCE_DIR}/src/parted_arr.cpp"
)
tx_add_release_ops(TXData)
# dependencies
target_link_libraries(TXData PUBLIC
	TXMath
)

target_include_directories(TXData PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)