// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: Utility
// File: random.hpp

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
	    : m_dist(min, max) {}
	RandDistBlacklist(T min, T max, SortedArr<T>& blacklist)
	    : m_blacklist(blacklist), m_dist(min, max - m_blacklist.size()) {}
	RandDistBlacklist(T min, T max, SortedArr<T>&& blacklist)
	    : m_blacklist(std::move(blacklist)), m_dist(min, max - m_blacklist.size()) {}
	template <tx::input_iterator_value_type<T> It>
	RandDistBlacklist(T min, T max, It begin, It end)
	    : m_blacklist(begin, end), m_dist(min, max - m_blacklist.size()) {}
	RandDistBlacklist(T min, T max, std::span<const T> blacklist)
	    : m_blacklist(blacklist), m_dist(min, max - m_blacklist.size()) {}
	RandDistBlacklist(T min, T max, std::initializer_list<T> blacklist)
	    : m_blacklist(blacklist), m_dist(min, max - m_blacklist.size()) {}
	RandDistBlacklist() = default;

	void insertBlacklistEntry(T val) {
		if (m_blacklist.exist(val)) return;
		m_blacklist.insert(val);
		rebuildDist_impl();
	}
	void insertBlacklistEntry(std::span<const T> vals) {
		m_blacklist.merge(vals);
		m_blacklist.unique();
		rebuildDist_impl();
	}
	void insertBlacklistEntry(std::initializer_list<T> vals) {
		m_blacklist.merge(vals);
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
	T operator()(GeneratorT& gen) const {
		T result = m_dist(gen);
		u32 begin = 0;
		while (marchBlacklist_impl(result, begin)) {}
		return result;
	}

private:
	SortedArr<T> m_blacklist;
	std::uniform_int_distribution<T> m_dist;

	// @return 1 for continue, 0 for end
	bool marchBlacklist_impl(T& result, u32& begin) {
		u32 index = m_blacklist.index(m_blacklist.lower_bound(begin, result));
		if (index <= begin) return 0; // if `it` is before the valid range
		// index become the increment for the result value

		// dealing with the continuous stride of blacklist value
		u32 count = 0;
		while (index + count < m_blacklist.size() &&
		       m_blacklist[index + count] == result + count) {
			count++;
		}
		index += count;

		result += index;
		return 1;
	}

	void rebuildDist_impl(T min, T max) {
		m_dist = std::uniform_int_distribution<T>(
		    min, max - m_blacklist.size());
	}
	void rebuildDist_impl() { rebuldDist_impl(m_dist.min(), m_dist.max()); }
};

template <class T>
using uniform_int_distribution_blacklist = RandDistBlacklist<T>;
} // namespace tx