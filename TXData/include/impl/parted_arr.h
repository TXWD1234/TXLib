// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXResource
// File: detail.h

#pragma once
#include "impl/basic_utils.h"

namespace tx {

// partitioned array, an alternative for std::vector<std::vector<T>> that's more cache friendly
// order is not garenteed. any pointer is not garenteed to be valid after push_back
template <class T>
class PartedArr {
	// terminology:
	// part = partition
public:
	class iterator {
	public:
		// Required traits for STL compatibility
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		iterator(T* base, u32 capacity, u32 virtualIndex)
		    : m_base(base), m_capacity(capacity), m_vIndex(virtualIndex) {}

		reference operator*() const {
			return m_base[m_vIndex % m_capacity];
		}

		iterator& operator++() {
			++m_vIndex;
			return *this;
		}
		iterator operator+(u32 n) const { return { m_base, m_capacity, m_vIndex + n }; }

		// Logic: Equality is based on the "virtual" position
		bool operator==(const iterator& other) const { return m_vIndex == other.m_vIndex; }
		bool operator!=(const iterator& other) const { return !(*this == other); }

		// Distance (needed for std::distance and some loops)
		difference_type operator-(const iterator& other) const {
			return (difference_type)m_vIndex - (difference_type)other.m_vIndex;
		}

	private:
		T* m_base; // Pointer to the start of data()
		u32 m_capacity; // Total size of data()
		u32 m_vIndex; // The "absolute" position (can be > capacity)
	};

	using It_t = typename std::vector<T>::iterator;
	using ConstIt_t = typename std::vector<T>::const_iterator;

public:
	PartedArr(u32 partCount, u32 partLen)
	    : data(std::vector<T>(partCount * partLen)),
	      partAttribs(std::vector<u32>(partCount)),
	      PartLen(partLen) {
		for (int i = 0; i < partCount; ++i) {
			partAttribs[i].offset = i * partLen;
			partAttribs[i].len = 0;
		}
	}


	class PartAttrib_impl {
	public:
		u32 offset;
		u32 len;
	};
	class Partition_impl {
	public:
		Partition_impl(PartedArr* in_parent, u32 in_index)
		    : parent(in_parent), attrib(in_parent->partAttribs[in_index]), partIndex(in_index) {}

		T& operator[](u32 index) { return parent->data[parent->clampIndex(attrib.offset + index)]; }

		u32 size() const { return attrib.len; }

		void push_back(const T& val) {
			parent->push_back_impl(partIndex, val);
		}

		It_t begin() { return parent->begin() + attrib.offset; }
		It_t end() { return parent->begin() + attrib.offset + attrib.len; }

	private:
		PartedArr<T>* parent;
		PartAttrib_impl attrib;
		u32 partIndex;
	};
	class ConstPartition_impl {
	public:
		ConstPartition_impl(const PartedArr* in_parent, u32 in_index)
		    : parent(in_parent), attrib(in_parent->partAttribs[in_index]) {}

		const T& operator[](u32 index) const { return parent->data[parent->clampIndex(attrib.offset + index)]; }

		u32 size() const { return attrib.len; }
		ConstIt_t begin() const { return parent->begin() + attrib.offset; }
		ConstIt_t end() const { return parent->begin() + attrib.offset + attrib.len; }

	private:
		const PartedArr<T>* parent;
		PartAttrib_impl attrib;
	};

	Partition_impl operator[](u32 index) { return Partition_impl{ this, index }; }
	ConstPartition_impl operator[](u32 index) const { return ConstPartition_impl{ this, index }; }

	void refresh(Partition_impl& in) const { in.attrib = partAttribs[in.partIndex]; }
	void refresh(ConstPartition_impl& in) const { in.attrib = partAttribs[in.partIndex]; }


	It_t begin() { return data.begin(); }
	It_t end() { return data.end(); }
	ConstIt_t begin() const { return data.begin(); }
	ConstIt_t end() const { return data.end(); }

private:
	std::vector<T> data;
	std::vector<PartAttrib_impl> partAttribs;
	const u32 PartLen;

	u32 nextPart(u32 index) {
		return index >= partAttribs.size() - 1 ? 0 : index + 1;
	}
	u32 checkIndexBound(u32 index) {
		return index >= data.size() ? 0 : index;
	}
	u32 clampIndex(u32 index) const {
		return index % data.size();
	}
	// circular shift
	void push_back_impl(u32 partIndex, const T& val) {
		PartAttrib_impl& attrib = partAttribs[partIndex];
		if (partAttribs[partIndex].len >= PartLen) {
			u32 next = nextPart(partIndex);
			if (partAttribs[next].len) {
				push_back_impl(next, std::move(data[partAttribs[next].offset++]));
				partAttribs[next].offset = checkIndexBound(partAttribs[next].offset);
			} else
				++partAttribs[next].offset;
			--partAttribs[next].len;
		}
		data[checkIndexBound(attrib.offset + attrib.len++)] = val;
	}
};

} // namespace tx