# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXJson

tx_txlib_module(
	LIB_TYPE STATIC
	PUBLIC_HEADERS
		json.h
	IMPL_HEADERS
		json/parser.hpp
		json/type_traits.hpp
		json/types.hpp
		json/writer.hpp
	SOURCES
		parser.cpp
)