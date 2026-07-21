// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <span>
#include <algorithm>
#include <type_traits>

#include <vector>

namespace tx {

template<class T, bool keepOriginalData, bool keepOriginalIndex>
class BinarySetView {
public:
	BinarySetView(std::span<T> data) {
		if constexpr (keepOriginalData) {
			
		} else {
			m_data = data;
		}

	}
private:
	std::span<std::conditional_t<keepOriginalData, const T, T>> m_data;
	std::conditional_t<keepOriginalData, std::vector<T>, Nothing> m_dataCopied;
	std::conditional_t<keepOriginalIndex, std::vector<u32>, Nothing> m_originalIndex;

};
}