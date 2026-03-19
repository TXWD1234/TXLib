# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXJson

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXJson)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")


add_library(TXJson STATIC
"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/json.h"
"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/json_parser.h"
"${CMAKE_CURRENT_SOURCE_DIR}/src/json_parser.cpp"
)
tx_add_release_ops(TXJson)

# dependencies
target_link_libraries(TXJson PUBLIC TXMap)

target_include_directories(TXJson PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)