# Copyright (c) 2025 TXLib. Licensed under the MIT License.
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
"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/json_parser.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/src/json_parser.cpp"
)
tx_add_release_ops(TXJson)
target_compile_features(TXJson PUBLIC cxx_std_20)

# dependencies
target_link_libraries(TXJson PUBLIC
	TXMap
	TXMath	
)

target_include_directories(TXJson PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)