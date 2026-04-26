// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "impl/basic_utils.hpp"
#include <type_traits>
#include <iterator>
#include <concepts>

namespace tx {

template <class Pred, std::contiguous_iterator it_t, std::contiguous_iterator... multi_it_t>
    requires std::invocable<Pred, std::iter_value_t<it_t>, std::iter_value_t<it_t>>
void sort_multi(it_t begin, it_t end, Pred&& pred, multi_it_t const&... byProducts) {
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
template <std::contiguous_iterator it_t, std::contiguous_iterator... multi_it_t>
void sort_multi(it_t begin, it_t end, multi_it_t const&... byProducts) {
	sort_multi(begin, end, std::less<>{}, byProducts...);
}

/**
 * @brief remove a set of values in a range. the set of value to be removed and the range are both sorted
 * @return the first element of the removed range
 */
template <std::forward_iterator it_t, std::forward_iterator rem_it_t, class Cmp = std::less<>>
    requires std::invocable<Cmp, std::iter_value_t<it_t>, std::iter_value_t<rem_it_t>>
it_t remove_multi_sorted(it_t begin, it_t end, rem_it_t remove_begin, rem_it_t remove_end, Cmp cmp = {}) {
	it_t data = begin;
	it_t dest = begin;
	rem_it_t rem = remove_begin;
	rem_it_t remEnd = remove_end;

	while (data != end && rem != remEnd) {
		if (cmp(*data, *rem)) { // data less then rem
			*(dest++) = std::move(*(data++)); // move this element to dest -> save it
		} else if (cmp(*rem, *data)) { // data bigger then rem -> no more things need to be consider for the current rem value
			++rem; // next rem val
		} else { // data equal to rem -> remove
			++data; // skip this element, it will be over-written by incoming values, or result in garbage value at the back
		}
	}

	// Copy any remaining elements that are larger than the last remove target
	while (data != end) { *(dest++) = std::move(*(data++)); }

	return dest;
}
} // namespace tx