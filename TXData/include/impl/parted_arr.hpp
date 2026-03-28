// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXResource
// File: detail.h

#pragma once
#include "impl/basic_utils.hpp"
#include <cstring>
#include <algorithm>

namespace tx {

// partitioned array, an alternative for std::vector<std::vector<T>> that's more cache friendly and faster
// note: slightly memory wasting comparing to CircularArr but faster
// order is not garenteed. any pointer is not garenteed to be valid after push_back
template <class T>
class PartedArr {
	// terminology:
	// part = partition
public:
	using It_t = typename std::vector<T>::iterator;
	using ConstIt_t = typename std::vector<T>::const_iterator;

public:
	PartedArr(u32 partLen = 64) : PartLen(partLen) {}
	PartedArr(u32 partCount, u32 partLen)
	    : m_data(std::vector<T>(partCount * partLen)),
	      partAttribs(std::vector<PartAttrib_impl>(partCount)),
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
		u32 memoryIndex;
		u32 partCount = 1;
	};
	class Partition_impl {
	public:
		Partition_impl(PartedArr* in_parent, u32 in_index)
		    : parent(in_parent), partIndex(in_index) {}

		T& operator[](u32 index) { return parent->m_data[attrib().offset + index]; }

		u32 size() const { return attrib().len; }
		u32 offset() const { return attrib().offset; }

		void push_back(const T& val) {
			parent->push_back_impl(partIndex, val);
		}

		void clear() {
			attrib().len = 0;
		}

		void resize(u32 newsize) {
			parent->reservePart_impl(partIndex, newsize);
			attrib().len = newsize;
		}
		void reserve(u32 newsize) {
			parent->reservePart_impl(partIndex, newsize);
		}

		It_t begin() { return parent->begin() + attrib().offset; }
		It_t end() { return parent->begin() + attrib().offset + attrib().len; }

	private:
		PartedArr<T>* parent;
		u32 partIndex;

		PartAttrib_impl& attrib() const { return parent->partAttribs[partIndex]; }
	};
	class ConstPartition_impl {
	public:
		ConstPartition_impl(const PartedArr* in_parent, u32 in_index)
		    : parent(in_parent), partIndex(in_index) {}

		const T& operator[](u32 index) const { return parent->m_data[attrib().offset + index]; }

		u32 size() const { return attrib().len; }
		u32 offset() const { return attrib().offset; }
		ConstIt_t begin() const { return parent->begin() + attrib().offset; }
		ConstIt_t end() const { return parent->begin() + attrib().offset + attrib().len; }

	private:
		const PartedArr<T>* parent;
		u32 partIndex;

		const PartAttrib_impl& attrib() const { return parent->partAttribs[partIndex]; }
	};

	Partition_impl operator[](u32 index) { return Partition_impl{ this, index }; }
	ConstPartition_impl operator[](u32 index) const { return ConstPartition_impl{ this, index }; }

	void addPartition() {
		partOrder.push_back(partAttribs.size());
		partAttribs.push_back(PartAttrib_impl{ static_cast<u32>(m_data.size()), 0, static_cast<u32>(partAttribs.size()), 1 });
		pushPart_impl(1);
	}

	void clear() {
		m_data.clear();
		partAttribs.clear();
	}
	// keep partitions
	void clearData() {
		for (PartAttrib_impl& i : partAttribs) {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (u32 j = 0; j < i.len; ++j) {
					m_data[i.offset + j] = T{};
				}
			}
			i.len = 0;
		}
	}

	T* data() { return m_data.data(); }
	u32 size() { return partAttribs.size(); }
	u32 dataSize() { return m_data.size(); }

	It_t begin() { return m_data.begin(); }
	It_t end() { return m_data.end(); }
	ConstIt_t begin() const { return m_data.begin(); }
	ConstIt_t end() const { return m_data.end(); }

private:
	std::vector<T> m_data;
	std::vector<PartAttrib_impl> partAttribs;
	std::vector<u32> partOrder;
	const u32 PartLen;

	// memory operations

	void pushPart_impl(u32 n = 1) { // add n empty partition at the end
		m_data.resize(m_data.size() + PartLen * n);
	}
	void moveData_impl(u32 dest, u32 src, u32 len) {
		if (len == 0) return;

		T* pSrc = m_data.data() + src;
		T* pDst = m_data.data() + dest;

		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memcpy(pDst, pSrc, len * sizeof(T));
		} else {
			for (u32 i = 0; i < len; ++i) {
				pDst[i] = std::move(pSrc[i]);
			}
		}
	}

	// arrangements

	void movePartToEnd_impl(u32 partIndex) {
		PartAttrib_impl& attrib = partAttribs[partIndex];
		if (attrib.memoryIndex >= partOrder.size() - 1) return;
		// meta
		for (u32 i = attrib.memoryIndex + 1; i < partOrder.size(); i++) {
			partAttribs[partOrder[i]].memoryIndex--;
		}
		std::rotate(partOrder.begin() + attrib.memoryIndex,
		            partOrder.begin() + attrib.memoryIndex + 1,
		            partOrder.end());
		attrib.memoryIndex = partOrder.size() - 1;
		// memory
		u32 nextOffset = m_data.size();
		pushPart_impl(attrib.partCount);
		moveData_impl(nextOffset, attrib.offset, attrib.len);
		attrib.offset = nextOffset;
	}

	void reservePart_impl(u32 partIndex, u32 newsize) {
		PartAttrib_impl& attrib = partAttribs[partIndex];
		int expandPartCount = max(static_cast<int>((newsize + PartLen - 1) / PartLen) - static_cast<int>(attrib.partCount), 0);
		attrib.partCount += expandPartCount;
		// memory
		if (attrib.memoryIndex >= partOrder.size() - 1) { // at the end
			pushPart_impl(expandPartCount);
		} else {
			while (expandPartCount > 0) {
				u32 next = partOrder[attrib.memoryIndex + 1];
				movePartToEnd_impl(next);
				expandPartCount -= partAttribs[next].partCount;
			}
			attrib.partCount -= expandPartCount;
		}
	}


	// linear reallocate
	void push_back_impl(u32 partIndex, const T& val) {
		PartAttrib_impl& attrib = partAttribs[partIndex];
		if (attrib.len >= attrib.partCount * PartLen) { // if exceeded -> resize
			reservePart_impl(partIndex, attrib.len + 1);
		}
		m_data[attrib.offset + attrib.len++] = val;
	}
};

template <class T>
using PartedArr_Partition = typename PartedArr<T>::Partition_impl;
template <class T>
using PartedArr_ConstPartition = typename PartedArr<T>::ConstPartition_impl;
} // namespace tx