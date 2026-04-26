# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXResource

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXResource)
	return()
endif()

set(TXLib "..")
set(CMakeUtils "${TXLib}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")

# dependencies'
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependency/stb_image")

add_library(TXResource STATIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/resource.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/impl/io_file.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/io_file.cpp"
)
tx_add_release_ops(TXResource)
target_compile_features(TXResource PUBLIC cxx_std_20)
target_link_libraries(TXResource PUBLIC
	stb_image
	TXMath
)

target_include_directories(TXResource PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)