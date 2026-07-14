# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXGrid

tx_txlib_module(
	LIB_TYPE STATIC
	PUBLIC_HEADERS
		grid.h
	IMPL_HEADERS
		grid_system.hpp
		grid_utils.hpp
		coord_map.hpp
	SOURCES
		grid_system.cpp
		grid_utils.cpp
		coord_map.cpp
)