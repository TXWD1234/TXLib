# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXUtility

cmake_minimum_required(VERSION 4.2)

# to defence being included multiple times
if(TARGET TXUtility)
	return()
endif()
# to defence the invalid add_subdirectories call that's not from add_txlib.cmake
if(NOT TXLib_SOURCE_DIR OR NOT TXLib_BINARY_DIR)
	message(FATAL_ERROR "TXLib: TXLib_SOURCE_DIR or TXLib_BINARY_DIR does not exist. Hint: Not added by `add_txlib.cmake`")
endif()

set(CMakeUtils "${TXLib_SOURCE_DIR}/TXCMakeUtils")

include("${CMakeUtils}/optimization.cmake")
include("${CMakeUtils}/impl/make_module_available.cmake")


set(TXLib_MODULE_NAME "TXUtility")
add_library(TXUtility INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/utility.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/type_traits.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/algorithm.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/random.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/bit_trick.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/tx/handle_system.hpp"
)
tx_add_release_ops_interface(TXUtility)
target_compile_features(TXUtility INTERFACE cxx_std_20)

tx_make_module_available(TXMath)
tx_make_module_available(TXData)
target_link_libraries(TXUtility INTERFACE
	TXMath
	TXData
)

target_include_directories(TXUtility INTERFACE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
)