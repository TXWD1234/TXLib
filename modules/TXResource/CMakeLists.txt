# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXResource

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependency/stb_image")

tx_txlib_module(
	LIB_TYPE STATIC
	PUBLIC_HEADERS
		resource.h
	IMPL_HEADERS
		image.hpp
		io_file.hpp
	SOURCES
		io_file.cpp
)

target_link_libraries(TXResource PUBLIC
	stb_image
)