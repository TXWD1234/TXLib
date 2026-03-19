# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXResource

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXResource)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

# dependencies

add_library(TXResource STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/resource.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/detail.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/detail.cpp"
)
tx_add_release_ops(TXResource)

target_include_directories(TXResource PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)