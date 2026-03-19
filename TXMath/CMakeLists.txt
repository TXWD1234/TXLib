# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXMath

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXMath)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

# no dependencies

add_library(TXMath INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/math.h" # math.h is the umbrella header: the header the user should include
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/geometry.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/math_line.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/geom_utils.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/basic_utils.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/geom_util_classes.h"
)
tx_add_release_ops_interface(TXMath)

target_include_directories(TXMath INTERFACE 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)