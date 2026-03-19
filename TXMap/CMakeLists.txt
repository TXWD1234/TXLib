# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXMap

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXMap)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

add_library(TXMap INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/map.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/kvmap.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/set_view.h"
)
tx_add_release_ops_interface(TXMap)

# dependency
target_link_libraries(TXMap INTERFACE 
	TXMath
)

target_include_directories(TXMap INTERFACE 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)