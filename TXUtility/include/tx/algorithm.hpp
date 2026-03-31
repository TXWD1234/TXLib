// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXUtility
// File: algorithm.hpp

#pragma once
#include "impl/basic_utils.hpp"
#include <type_traits>
#include <iterator>
#include <concepts>

namespace tx {

template <class Pred, std::contiguous_iterator it_t, std::contiguous_iterator... multi_it_t>
void multi_sort(it_t begin, it_t end, Pred&& pred, multi_it_t const&... byProducts) {
	std::vector<u32> indexArray(static_cast<u32>(end - begin));
	for (u32 i = 0; i < indexArray.size(); i++) {
		indexArray[i] = i;
	}

	auto* begin_ptr = std::to_address(begin);

	std::sort(indexArray.begin(), indexArray.end(), [&](u32 a, u32 b) {
		return pred(begin_ptr[a], begin_ptr[b]);
	});

	std::vector<u8> visited(indexArray.size(), 0);

	[&](auto*... byProducts_ptrs) {
		for (u32 i = 0; i < visited.size(); i++) {
			if (visited[i] || i == indexArray[i]) continue;

			u32 indexBegin = i;

			auto tempPrimary = std::move(begin_ptr[indexBegin]);
			auto tempByProduct = std::make_tuple(std::move(byProducts_ptrs[indexBegin])...);
			u32 cur = indexBegin;
			u32 next = indexArray[indexBegin];
			while (next != indexBegin) {
				begin_ptr[cur] = std::move(begin_ptr[next]);
				((byProducts_ptrs[cur] = std::move(byProducts_ptrs[next])), ...);

				visited[cur] = 1;
				cur = next;
				next = indexArray[next];
			}
			visited[cur] = 1;
			begin_ptr[cur] = std::move(tempPrimary);
			std::apply(
			    [&](auto&&... jtempByProducts) {
				    ((byProducts_ptrs[cur] = std::move(jtempByProducts)), ...);
			    },
			    tempByProduct);
		}
	}(std::to_address(byProducts)...);
}
}