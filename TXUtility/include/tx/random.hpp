// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/sorted_arr.hpp"
#include <concepts>
#include <random>

namespace tx {
template <std::integral T>
class RandDistBlacklist {
public:
	using value_type = T;

	RandDistBlacklist(T min, T max)
	    : m_dist(min, max), m_range{ min, max } {}
	RandDistBlacklist(T min, T max, SortedArr<T>& blacklist)
	    : m_blacklist(blacklist), m_dist(min, max - m_blacklist.size()), m_range{ min, max } {}
	RandDistBlacklist(T min, T max, SortedArr<T>&& blacklist)
	    : m_blacklist(std::move(blacklist)), m_dist(min, max - m_blacklist.size()), m_range{ min, max } {}
	template <tx::input_iterator_value_type<T> It>
	RandDistBlacklist(T min, T max, It begin, It end)
	    : m_range{ min, max } {
		insertBlacklistEntry(begin, end);
	}
	RandDistBlacklist(T min, T max, std::span<const T> blacklist)
	    : m_range{ min, max } {
		insertBlacklistEntry(blacklist);
	}
	RandDistBlacklist(T min, T max, std::initializer_list<T> blacklist)
	    : m_range{ min, max } {
		insertBlacklistEntry(blacklist);
	}
	RandDistBlacklist() = default;

	void insertBlacklistEntry(T val) {
		if (m_blacklist.exist(val) || !inRange(val, m_range.min, m_range.max)) return;
		m_blacklist.insert(val);
		rebuildDist_impl();
	}
	void insertBlacklistEntry(std::span<const T> vals) {
		m_blacklist.merge_if(vals, [&](const T& val) { return inRange(val, m_range.min, m_range.max); });
		m_blacklist.unique();
		rebuildDist_impl();
	}
	template <tx::input_iterator_value_type<T> It>
	void insertBlacklistEntry(It begin, It end) {
		m_blacklist.insertMulti([&](auto& inserter) {
			std::for_each(begin, end, [&](const T& val) {
				if (inRange(val, m_range.min, m_range.max)) { inserter.insert(val); }
			});
		});
		m_blacklist.unique();
		rebuildDist_impl();
	}
	void insertBlacklistEntry(std::initializer_list<T> vals) {
		m_blacklist.merge_if(vals, [&](const T& val) { return inRange(val, m_range.min, m_range.max); });
		m_blacklist.unique();
		rebuildDist_impl();
	}
	void ban(T val) { insertBlacklistEntry(val); }
	void ban(std::span<const T> vals) { insertBlacklistEntry(vals); }
	void ban(std::initializer_list<T> vals) { insertBlacklistEntry(vals); }

	void removeBlacklistEntry(T val) {
		auto it = m_blacklist.find(val);
		if (it == m_blacklist.end()) return;
		m_blacklist.erase(it);
		rebuildDist_impl();
	}
	void removeBlacklistEntry(std::span<const T> vals) {
		m_blacklist.erase(vals);
		rebuildDist_impl();
	}
	void removeBlacklistEntry(std::initializer_list<T> vals) {
		m_blacklist.erase(vals);
		rebuildDist_impl();
	}

	void unban(T val) { removeBlacklistEntry(val); }
	void unban(std::span<const T> vals) { removeBlacklistEntry(vals); }
	void unban(std::initializer_list<T> vals) { removeBlacklistEntry(vals); }

	template <std::uniform_random_bit_generator GeneratorT>
	T operator()(GeneratorT& gen) { // not count because std::uniform_int_distribution::operator() is not const
		T result = m_dist(gen);
		u32 begin = 0;
		while (marchBlacklist_impl(result, begin)) {}
		return result + static_cast<T>(begin);
	}

private:
	SortedArr<T> m_blacklist;
	std::uniform_int_distribution<T> m_dist;
	struct Range_impl {
		T min = 0, max = 0;
		std::uniform_int_distribution<T> makeDist_impl(u32 blacklistCount) {
			return std::uniform_int_distribution<T>(min, max - blacklistCount);
		}
	} m_range;

	// @return true for continue, false for end
	bool marchBlacklist_impl(T result, u32& begin) const {
		result += static_cast<T>(begin); // just to get the new result
		u32 index = m_blacklist.index(m_blacklist.lower_bound(begin, result));
		if (index == m_blacklist.size()) {
			begin = index;
			return false;
		}
		// if there's no more blacklist entry that the `result` can reach
		if (index == begin && m_blacklist[index] != result) return false;
		// index become the increment for the result value, which is the total accounted "holes" / blacklist entries that the result can reach

		// dealing with the continuous stride of blacklist value
		u32 count = 0;
		while (index + count < m_blacklist.size() &&
		       m_blacklist[index + count] == result + static_cast<T>(count)) {
			count++;
		}

		begin = index + count; // the total accounted "holes" / blacklist entries
		return true;
	}

	void rebuildDist_impl() {
		m_dist = m_range.makeDist_impl(m_blacklist.size());
	}
};

template <class T>
using uniform_int_distribution_blacklist = RandDistBlacklist<T>;
} // namespace tx