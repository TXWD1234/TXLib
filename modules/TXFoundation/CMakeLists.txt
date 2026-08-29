# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXFoundation

tx_txlib_module(
	LIB_TYPE INTERFACE
	PUBLIC_HEADERS
		"foundation.h"

		"type_traits.hpp"
		"algorithm.hpp"
		"basic_types.hpp"
		"build_config.hpp"
		"exception.hpp"
)

# target_link_libraries(${TXLib_INSTALLATION_MODULE_NAME} INTERFACE 
# 	stdc++_libbacktrace
# )