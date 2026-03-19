# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXGrid

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXGrid)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

add_library(TXGrid STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/grid.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/grid_system.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/grid_system.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/grid_utils.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/grid_utils.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/coord_map.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/coord_map.cpp"
)
tx_add_release_ops(TXGrid)
target_link_libraries(TXGrid PUBLIC TXMath)

target_include_directories(TXGrid PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)