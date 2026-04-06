// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXMap
// File: kvmap.h

#pragma once
#include "impl/basic_utils.hpp"
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <concepts>
#include <sstream>

namespace tx {

// key value pair *************************************************************************************************************
template <class KT, class VT>
class KVPair {
public:
	KVPair(const KT& in_k, const VT& in_v) : m_key(in_k), m_value(in_v) {}

	inline KT& k() { return m_key; }
	inline KT& key() { return m_key; }
	inline VT& v() { return m_value; }
	inline VT& value() { return m_value; }
	inline const KT& k() const { return m_key; }
	inline const VT& v() const { return m_value; }

private:
	KT m_key;
	VT m_value;
};
template <class KT, class VT, class CompareFunc = std::less<>>
    requires std::is_invocable_r_v<bool, CompareFunc, KT, KT>
class KVMap;
// Handle to access value without key
// Keep in mind that after any operation that involve key, all old handles will become invalid
template <class KT, class VT, class CompareFunc = std::less<>>
class KVMapHandle {
public:
	KVMapHandle(KVMap<KT, VT, CompareFunc>* in_map, int in_index) : map(in_map), index(in_index) {
	}
	VT& get() {
		return this->map->atIndex(index).v();
	}

private:
	KVMap<KT, VT, CompareFunc>* map;
	int index;
};
// built-in map conflict resolution methods
// used for duplicates when insert or merge
namespace Map {
template <class Func, class KT, class VT>
concept ConflictResolveFunc = std::is_invocable_r_v<KVPair<KT, VT>, Func, KVPair<KT, VT>&&, KVPair<KT, VT>&&>;

struct Replace {
	template <class KT, class VT>
	KVPair<KT, VT> operator()(KVPair<KT, VT>&& a, KVPair<KT, VT>&& b) const {
		return std::move(b);
	}
};
struct Ignore {
	template <class KT, class VT>
	KVPair<KT, VT> operator()(KVPair<KT, VT>&& a, KVPair<KT, VT>&& b) const {
		return a;
	}
};
struct Error {
	template <class KT, class VT>
	KVPair<KT, VT> operator()(KVPair<KT, VT>&& a, KVPair<KT, VT>&& b) const {
		if constexpr (std::is_convertible_v<KT, std::string>) {
			std::ostringstream oss;
			oss << "tx::KVMap: conflicting keys: \"" << a.k() << "\"";
			throw std::runtime_error{ oss.str() };
		} else {
			throw std::runtime_error{ "tx::KVMap: conflicting keys" };
		}
	}
};
} // namespace Map
// key value map
// Any insertion, removal, or validation invalidates all iterators.
template <class KT, class VT, class CompareFunc>
    requires std::is_invocable_r_v<bool, CompareFunc, KT, KT>
class KVMap {
	friend KVMapHandle<KT, VT, CompareFunc>;
	using Pair = KVPair<KT, VT>;
	using Handle = KVMapHandle<KT, VT, CompareFunc>;

public:
	using iterator = typename std::vector<Pair>::iterator;
	using const_iterator = typename std::vector<Pair>::const_iterator;

	using key_type = KT;
	using value_type = VT;
	using cmp_type = CompareFunc;

public:
	KVMap(CompareFunc in_cmp = std::less<>{}) : cmp(std::move(in_cmp)) {}
	KVMap(std::initializer_list<Pair> in_data, CompareFunc in_cmp = std::less<>{}) : pairs(in_data), cmp(std::move(in_cmp)) {
		validate();
	}

	// general

	inline bool valid() const { return this->m_valid; };
	inline int size() const { return this->pairs.size(); }
	inline bool empty() const { return this->pairs.empty(); }
	inline Pair& atIndex(int index) { return pairs[index]; }
	inline const Pair& atIndex(int index) const { return pairs[index]; }

	void reserve(int count) { this->pairs.reserve(count); }
#ifdef TX_JERRY_IMPL
	template <class K>
	inline VT& operator[](const K& key) { return this->at(key); }
	template <class K>
	inline const VT& operator[](const K& key) const { return this->at(key); }
#endif



	//inline Handle insertAssign(const KT& key, const VT& value = VT{}) {
	//	if (this->exist(key)) {
	//		return this->set(key, value);
	//	} else {
	//		return _insert(key, value);
	//	}
	//}


	// Disorder
	// note that all const functions are disorder because they cannot sort the array

	template <class K>
	inline bool exist(const K& key) const {
		return validIt_impl(findItDisorder_impl(key), key);
	}

	template <class K>
	inline const VT& at(const K& key) const {
		auto it = findItDisorder_impl(key);
		if (validIt_impl(it, key))
			return it->v();
		else
			throw_impl();
	}

	template <Map::ConflictResolveFunc<KT, VT> ResolveFunc = Map::Replace>
	inline Handle insertMulti(const KT& key, const VT& value = VT{}, ResolveFunc&& f = Map::Replace{}) {
		return insertDisorder_impl(key, value, std::forward<ResolveFunc>(f));
	}

	template <class K>
	inline const_iterator find(const K& key) const {
		auto it = findItDisorder_impl(key);
		if (validIt_impl(it, key))
			return it;
		else
			return pairs.end();
	}





	// Order
	// the first line of any order function must be validate()

	template <class K>
	inline bool exist(const K& key) {
		validate();
		return existOrder_impl(key);
	}

	template <class K>
	inline VT& at(const K& key) {
		validate();
		auto it = findItOrder_impl(key);
		if (!validIt_impl(it, key)) throw_impl();
		return it->v();
	}

	template <class K>
	inline Handle set(const K& key, const VT& value) {
		validate();
		auto it = findItOrder_impl(key);
		if (!validIt_impl(it, key)) throw_impl();
		it->v() = value;
		return Handle(this, static_cast<int>(it - this->pairs.begin()));
	}

	template <class K>
	inline void remove(const K& key) {
		validate();
		auto it = findItOrder_impl(key);
		if (!validIt_impl(it, key)) throw_impl();
		if (pairs.size() < 100) {
			std::swap(*it, pairs.back());
			pairs.pop_back();
			this->m_valid = 0;
			disorderIndex = tx::min(disorderIndex, static_cast<int>(it - pairs.begin()));
		} else {
			pairs.erase(it);
		}
	}

	template <Map::ConflictResolveFunc<KT, VT> ResolveFunc = Map::Replace>
	inline Handle insertSingle(const KT& key, const VT& value = VT{}, ResolveFunc&& f = Map::Replace{}) {
		validate();
		return insertOrder_impl(key, value, std::forward<ResolveFunc>(f));
	}

	template <class K>
	inline iterator find(const K& key) {
		validate();
		auto it = findItOrder_impl(key);
		if (validIt_impl(it, key))
			return it;
		else
			return pairs.end();
	}

	// base function

	inline void validate() {
		if (!this->m_valid) validate_impl();
	}



	// Resolves duplicate keys by applying a user-defined lambda
	// The lambda should take two `Pair`s and return the `Pair` to keep.

	template <Map::ConflictResolveFunc<KT, VT> ResolveFunc = Map::Replace>
	inline void merge(const KVMap& other, ResolveFunc resolve = Map::Replace{}) {
		this->merge_impl(other);
		this->unique_impl(resolve);
	}
	template <Map::ConflictResolveFunc<KT, VT> ResolveFunc = Map::Replace>
	inline void merge(KVMap&& other, ResolveFunc resolve = Map::Replace{}) {
		bool reversed = this->merge_impl(std::move(other));
		this->unique_impl(resolve, reversed);
	}

	template <std::invocable<VT&> Func>
	void foreach (Func&& f) {
		for (Pair& i : pairs) {
			f(i.v());
		}
	}
	template <std::invocable<const VT&> Func>
	void foreach (Func&& f) const {
		for (const Pair& i : pairs) {
			f(i.v());
		}
	}

	// iterator

	inline iterator begin() { return pairs.begin(); }
	inline const_iterator begin() const { return pairs.begin(); }
	inline iterator end() { return pairs.end(); }
	inline const_iterator end() const { return pairs.end(); }

private:
	std::vector<Pair> pairs;
	mutable bool m_valid = 0; // is sorted
	mutable int disorderIndex = 0; // the index of where the pairs start to become disorder
	CompareFunc cmp;

	// utilities

	inline void merge_impl(const KVMap& other) {
		validate();
		size_t originalSize = pairs.size();
		pairs.insert(pairs.end(), other.pairs.begin(), other.pairs.end());
		iterator otherBegin = pairs.begin() + originalSize;

		if (!other.m_valid) {
			sort_impl(otherBegin + other.disorderIndex, pairs.end());
			std::inplace_merge(otherBegin, otherBegin + other.disorderIndex, pairs.end(), PairCompare{ this->cmp });
		}
		std::inplace_merge(pairs.begin(), otherBegin, pairs.end(), PairCompare{ this->cmp });

		this->m_valid = 1;
		this->disorderIndex = pairs.size();
	}
	inline bool merge_impl(KVMap&& other) {
		validate();
		other.validate();

		bool reversed = false;
		// Choose the vector with the larger capacity to minimize memory allocations
		if (other.pairs.capacity() > pairs.capacity()) { // use other's vector
			size_t otherOriginalSize = other.pairs.size();
			other.pairs.insert(other.pairs.end(), std::make_move_iterator(pairs.begin()), std::make_move_iterator(pairs.end()));
			std::inplace_merge(other.pairs.begin(), other.pairs.begin() + otherOriginalSize, other.pairs.end(), PairCompare{ this->cmp });
			pairs = std::move(other.pairs);
			reversed = true;
		} else { // use own vector
			size_t originalSize = pairs.size();
			pairs.insert(pairs.end(), std::make_move_iterator(other.pairs.begin()), std::make_move_iterator(other.pairs.end()));
			std::inplace_merge(pairs.begin(), pairs.begin() + originalSize, pairs.end(), PairCompare{ this->cmp });
		}

		other.pairs.clear();
		other.m_valid = 1;
		other.disorderIndex = 0;

		this->m_valid = 1;
		this->disorderIndex = pairs.size();
		return reversed;
	}
	template <Map::ConflictResolveFunc<KT, VT> ResolveFunc>
	inline void unique_impl(ResolveFunc&& resolve, bool reversed = false) {
		if (pairs.empty()) return;
		auto dest = pairs.begin();
		for (auto it = pairs.begin() + 1; it != pairs.end(); ++it) {
			if (isSame_impl(dest->k(), it->k())) {
				if (reversed) {
					*dest = resolve(std::move(*it), std::move(*dest));
				} else {
					*dest = resolve(std::move(*dest), std::move(*it));
				}
			} else {
				++dest;
				if (dest != it) {
					*dest = std::move(*it);
				}
			}
		}
		pairs.erase(dest + 1, pairs.end());
		this->disorderIndex = pairs.size();
	}

	// base functions

	//inline bool isSame_impl(const T& a, const T& b) const { return cmp(a, b) == cmp(b, a); }
	template <class K1, class K2>
	inline bool isSame_impl(const K1& a, const K2& b) const { return !cmp(a, b) && !cmp(b, a); }
	template <class It, class K>
	inline bool validIt_impl(const It& it, const K& key) const { return (it != pairs.end() && isSame_impl(it->k(), key)); }

	struct PairCompare {
		const CompareFunc& cmp;
		inline bool operator()(const Pair& a, const Pair& b) const { return cmp(a.k(), b.k()); }
		template <class K>
		inline bool operator()(const Pair& element, const K& key) const { return cmp(element.k(), key); }
		template <class K>
		inline bool operator()(const K& key, const Pair& element) const { return cmp(key, element.k()); }
	};

	inline void sort_impl(iterator begin, iterator end) {
		std::sort(begin, end, PairCompare{ this->cmp });
	}

	// findIt in range (from start)
	// before calling this must make sure that the provided range is sorted
	template <class K>
	inline iterator findIt__impl(const K& key, int end) {
		return std::lower_bound(pairs.begin(), pairs.begin() + end, key, PairCompare{ this->cmp });
	}
	template <class K>
	inline const_iterator findIt__impl(const K& key, int end) const {
		return std::lower_bound(pairs.begin(), pairs.begin() + end, key, PairCompare{ this->cmp });
	}

	inline void validate_impl() {
		sort_impl(pairs.begin() + disorderIndex, pairs.end());
		std::inplace_merge(pairs.begin(), pairs.begin() + disorderIndex, pairs.end(), PairCompare{ this->cmp });
		this->m_valid = 1;
		this->disorderIndex = this->pairs.size();
	}

	// Order
	// all private function don't account for validate()
	// before public funcitons call private functions, make sure validate() was called
	template <Map::ConflictResolveFunc<KT, VT> ResolveFunc>
	inline Handle insertOrder_impl(const KT& key, const VT& value, ResolveFunc&& resolve) {
		auto it = findItOrder_impl(key);
		if (validIt_impl(it, key)) { // if conflict
			*it = resolve(std::move(*it), Pair{ key, value });
			return Handle{ this, static_cast<int>(it - this->pairs.begin()) };
		}
		if (it == this->pairs.end()) {
			this->pairs.emplace_back(key, value);
			return Handle{ this, static_cast<int>(this->pairs.size() - 1) };
		} else {
			auto new_it = this->pairs.insert(it, Pair{ key, value });
			return Handle{ this, static_cast<int>(new_it - this->pairs.begin()) };
		}
	}

	template <class K>
	inline auto findItOrder_impl(const K& key) {
		return findIt__impl(key, pairs.size());
	}

	template <class K>
	inline bool existOrder_impl(const K& key) {
		return validIt_impl(findItOrder_impl(key), key);
	}

	// Disorder
	template <Map::ConflictResolveFunc<KT, VT> ResolveFunc>
	inline Handle insertDisorder_impl(const KT& key, const VT& value, ResolveFunc&& resolve) {
		auto it = findItDisorder_impl(key);
		if (validIt_impl(it, key)) { // if conflict
			*it = resolve(std::move(*it), Pair{ key, value });
			return Handle{ this, static_cast<int>(it - this->pairs.begin()) };
		}
		this->pairs.emplace_back(key, value);
		if (this->m_valid) {
			this->m_valid = 0;
			this->disorderIndex = this->pairs.size() - 1;
		}
		return Handle(this, this->pairs.size() - 1);
	}
	template <class K>
	inline const_iterator findItDisorder_impl(const K& key) const {
		auto it = findIt__impl(key, this->disorderIndex);
		if (validIt_impl(it, key)) return it;
		for (size_t i = this->disorderIndex; i < pairs.size(); ++i) {
			if (isSame_impl(pairs[i].k(), key)) { return pairs.begin() + i; }
		}
		return pairs.end();
	}
	template <class K>
	inline iterator findItDisorder_impl(const K& key) {
		auto it = findIt__impl(key, this->disorderIndex);
		if (validIt_impl(it, key)) return it;
		for (size_t i = this->disorderIndex; i < pairs.size(); ++i) {
			if (isSame_impl(pairs[i].k(), key)) { return pairs.begin() + i; }
		}
		return pairs.end();
	}

	template <class K>
	inline bool existDisorder_impl(const K& key) const {
		return validIt_impl(findItDisorder_impl(key), key);
	}



	// throw
	inline void throw_impl() const {
		throw std::runtime_error{ "tx::KVMap:: error" };
	}
};
} // namespace tx