// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXData
// File: data_utils.hpp

#pragma once
#include "impl/basic_utils.hpp"

namespace tx {
template <std::input_iterator It>
inline u32 index(It begin, It it) {
	if constexpr (std::random_access_iterator<It>) {
		return static_cast<u32>(it - begin); // O(1)
	} else {
		return static_cast<u32>(std::distance(begin, it)); // O(n)
	}
}
} // namespace tx