// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// File: TXLib_KVMap

#pragma once
#include "txlib.hpp"

namespace tx {

	// key value pair *************************************************************************************************************
	template<class KT, class VT>
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
	template<class KT, class VT, class CompareFunc = std::less<KT>>
	class KVMap;
	// Handle to access value without key
	// Keep in mind that after any operation that involve key, all old handles will become invalid
	template<class KT, class VT>
	class KVMapHandle {
	public:
		KVMapHandle(KVMap<KT, VT>* in_map, int in_index) :
			map(in_map), index(in_index) {
		}
		VT& get() {
			return this->map->atIndex(index).v();
		}
	private:
		KVMap<KT, VT>* map;
		int index;
	};
	// key value map
	// Any insertion, removal, or validation invalidates all iterators.
	template<class KT, class VT, class CompareFunc>
	class KVMap {
		friend KVMapHandle<KT, VT>;
		using Pair = KVPair<KT, VT>;
		using Handle = KVMapHandle<KT, VT>;
	public:
		using iterator = std::vector<Pair>::iterator;
		using const_iterator = std::vector<Pair>::const_iterator;
	public:
		KVMap(CompareFunc in_cmp = std::less<KT>{}) : 
			cmp(in_cmp)
		{
			/*static_assert(std::is_convertible_v<decltype(std::declval<KT>() < std::declval<KT>()), bool>,
				"tx::KVMap: the type of key need to be comparable with operator<. The provided type don;t match the requirements.");*/
			static_assert(std::is_invocable_r_v<bool, CompareFunc, KT, KT>,
				"tx::KVMap: the type of key need to be comparable with a compare function. The provided type or CompareFunction don't match the requirements.");
		}
		KVMap(std::initializer_list<Pair> in_data,  CompareFunc in_cmp = std::less<KT>{}) : 
			pairs(in_data), cmp(in_cmp)
		{
			static_assert(std::is_invocable_r_v<bool, CompareFunc, KT, KT>,
				"tx::KVMap: the type of key need to be comparable with a compare function. The provided type or CompareFunction don't match the requirements.");
			validate();
		}
		

		// general

		inline bool valid() const { return this->m_valid; };
		inline int  size()  const { return this->pairs.size(); }
		inline bool empty() const { return this->pairs.empty(); }
		inline       Pair& atIndex(int index)       { return pairs[index]; }
		inline const Pair& atIndex(int index) const { return pairs[index]; }

		void reserve(int count) { this->pairs.reserve(count); }
#ifdef TX_JERRY_IMPL
		inline       VT& operator[](const KT& key)       { return this->at(key); }
		inline const VT& operator[](const KT& key) const { return this->at(key); }
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

		inline bool exist(const KT& key) const {
			return validIt_impl(findItDisorder_impl(key), key);
		}

		inline const VT& at(const KT& key) const {
			auto it = findItDisorder_impl(key);
			if (validIt_impl(it, key)) return it->v();
			else throw_impl(key);
		}

		inline Handle insertMulti(const KT& key, const VT& value = VT{}) {
			return insertDisorder_impl(key, value);
		}

		inline const_iterator find(const KT& key) const {
			auto it = findItDisorder_impl(key);
			if (validIt_impl(it, key)) return it;
			else                       return pairs.end();
		}





		// Order
		// the first line of any order function must be validate()

		inline bool exist(const KT& key) {
			validate();
			return existOrder_impl(key);
		}

		inline VT& at(const KT& key) {
			validate();
			auto it = findItOrder_impl(key);
			if (validIt_impl(it, key)) return it->v();
			else throw_impl(key);
		}

		inline Handle set(const KT& key, const VT& value) {
			validate();
			auto it = findItOrder_impl(key);
			if (!validIt_impl(it, key)) throw_impl(key);
			it->v() = value;
			return Handle(this, static_cast<int>(it - this->pairs.begin()));
		}

		inline void remove(const KT& key) {
			validate();
			auto it = findItOrder_impl(key);
			if (!validIt_impl(it, key)) throw_impl(key);
			if (pairs.size() < 100) {
				std::swap(*it, pairs.back());
				pairs.pop_back();
				this->m_valid = 0;
				disorderIndex = tx::min(disorderIndex, static_cast<int>(it - pairs.begin()));
			}
			else {
				pairs.erase(it);
			}
		}

		inline Handle insertSingle(const KT& key, const VT& value = VT{}) {
			validate();
			return insertOrder_impl(key, value);
		}

		inline iterator find(const KT& key) {
			validate();
			auto it = findItOrder_impl(key);
			if (validIt_impl(it, key)) return it;
			else                       return pairs.end();
		}

		// base function

		inline void validate() { if (!this->m_valid) validate_impl(); }

		// iterator

		inline iterator       begin()       { return pairs.begin(); }
		inline const_iterator begin() const { return pairs.begin(); }
		inline iterator       end()         { return pairs.end(); }
		inline const_iterator end()   const { return pairs.end(); }
	private:
		vector<Pair> pairs;
		mutable bool m_valid = 0; // is sorted
		mutable int disorderIndex = 0;
		CompareFunc cmp;


		// base functions

		inline bool isSame_impl(const KT& a, const KT& b) const { return cmp(a, b) == cmp(b, a); }
		template<class It>
		inline bool validIt_impl(const It& it, const KT& key) const { return (it != pairs.end() && isSame_impl(it->k(), key)); }


		// findIt in range (from start)
		// before calling this must make sure that the provided range is sorted
		inline iterator findIt__impl(const KT& key, int end) {
			return std::lower_bound(
				pairs.begin(), pairs.begin() + end, key,
				[this](const Pair& element, const KT& key) {
					return this->cmp(element.k(), key);
				}
			);
		}
		inline const_iterator findIt__impl(const KT& key, int end) const {
			return std::lower_bound(
				pairs.begin(), pairs.begin() + end, key,
				[this](const Pair& element, const KT& key) {
					return this->cmp(element.k(), key);
				}
			);
		}

		inline void validate_impl() {
			std::sort(pairs.begin(), pairs.end(), [this](const Pair& a, const Pair& b) {
				return this->cmp(a.k(), b.k());
				});
			this->m_valid = 1;
			this->disorderIndex = this->pairs.size();
		}

		// Order
		// all private function don't account for validate()
		// before public funcitons call private functions, make sure validate() was called

		inline Handle insertOrder_impl(const KT& key, const VT& value) {
			auto it = findItOrder_impl(key);
			if (validIt_impl(it, key)) throw_impl(key);
			if(it == this->pairs.end()){
				this->pairs.emplace_back(key, value);
				return Handle{ this, this->pairs.size() - 1 };
			} else {
				this->pairs.insert(it, Pair{ key, value });
				return Handle{ this, static_cast<int>(it - this->pairs.begin()) };
			}
		}

		inline auto findItOrder_impl(const KT& key) {
			return findIt__impl(key, pairs.size());
		}

		inline bool existOrder_impl(const KT& key) {
			return validIt_impl(findItOrder_impl(key), key);
		}

		// Disorder

		inline Handle insertDisorder_impl(const KT& key, const VT& value) {
			// check if exist
			if (existDisorder_impl(key)) throw_impl(key);

			this->pairs.emplace_back(key, value);
			if (this->m_valid) {
				this->m_valid = 0;
				this->disorderIndex = this->pairs.size() - 1;
			} return Handle(this, this->pairs.size() - 1);
		}

		inline auto findItDisorder_impl(const KT& key) const {
			auto it = findIt__impl(key, this->disorderIndex);
			if (validIt_impl(it, key)) return it;
			for (int i = this->disorderIndex; i < pairs.size(); ++i) {
				if (pairs[i].k() == key) { return pairs.begin() + i; }
			} return pairs.end();
		}

		inline bool existDisorder_impl(const KT& key) const {
			if (validIt_impl(findIt__impl(key, this->disorderIndex), key)) return 1;
			for (int i = this->disorderIndex; i < pairs.size(); ++i) {
				if (pairs[i].k() == key) { return 1; }
			} return 0;
		}



		// throw
		inline void throw_Impl(const string& key) const {
			string message = string{"tx::KVMap::at():"} + key + " key not found.";
			throw std::out_of_range{ message };
		}
	};

	// any manipulation of the original vector will invalidate the SetView object
	// you can use the validate function to validate the object, or construct a new object
	template<class T, class CmpFunc = std::less<T>>
	class SetView {
		using It_t      = vector<T>::iterator;
		//using ConstIt_t = vector<T>::const_iterator;
	public:
		SetView(It_t in_beginIt, It_t in_endIt, CmpFunc in_cmp = std::less<T>{}) :
			m_beginIt(in_beginIt), m_endIt(in_endIt), cmp(in_cmp)
		{
			sort_impl();
		}


		inline bool exist(const T& in) const { return validIt_impl(findIt_impl(in), in); }
		inline int count(const T& in) const {
			auto it = findIt_impl(in);
			int counter = 0;
			while (it != m_endIt && isSame_impl(*it, in)) {
				++counter;
				++it;
			} return counter;
		}
		inline int find(const T& key) const {
			It_t it = findIt_impl(key);
			if(!validIt_impl(it, key)) return -1;
			return static_cast<int>(it - m_beginIt);
		}

		
		inline void validate() { sort_impl(); }

		inline void push_back(int amount = 1) {
			m_endIt += amount;
			sort_impl();
		}




	private:
		It_t m_beginIt, m_endIt;
		CmpFunc cmp;

		inline It_t findIt_impl(const T& key) const {
			return std::lower_bound(
				m_beginIt, m_endIt, key,
				[this](const T& element, const T& key) {
					return this->cmp(element, key);
				}
			);
		}

		inline void sort_impl() {
			std::sort(m_beginIt, m_endIt, cmp);
		}

		inline bool isSame_impl(const T& a, const T& b) const { return cmp(a, b) == cmp(b, a); }
		template<class It>
		inline bool validIt_impl(const It& it, const T& key) const { return (it != m_endIt && isSame_impl(*it, key)); }




	};

	template<class T>
	SetView(typename vector<T>::iterator, typename vector<T>::iterator) -> SetView<T>;
	template<class T, class CmpFunc>
	SetView(typename vector<T>::iterator, typename vector<T>::iterator, CmpFunc) -> SetView<T, CmpFunc>;
















}