# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXData

tx_txlib_module(
	LIB_TYPE STATIC
	PUBLIC_HEADERS
		"data.h"
	IMPL_HEADERS
		"circular_arr.hpp"
		"parted_arr.hpp"
		"sorted_arr.hpp"
		"data_utils.hpp"
		"circular_queue.hpp"
		"binary_set_view.hpp"
	SOURCES
		"parted_arr.cpp"
)