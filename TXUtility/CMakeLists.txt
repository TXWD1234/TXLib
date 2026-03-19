# Copyright@TXLib All rights reserved.
# Author: TX Studio: TX_Jerry
# Module: TXUtility

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXUtility)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

add_library(TXUtility INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/utility.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/type_traits.hpp"
)
tx_add_release_ops_interface(TXUtility)

target_link_libraries(TXUtility INTERFACE TXMath)

target_include_directories(TXUtility INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)