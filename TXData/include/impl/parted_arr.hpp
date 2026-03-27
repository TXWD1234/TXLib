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
	    : data(std::vector<T>(partCount * partLen)),
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
		u32 partCount = 1;
	};
	class Partition_impl {
	public:
		Partition_impl(PartedArr* in_parent, u32 in_index)
		    : parent(in_parent), attrib(in_parent->partAttribs[in_index]), partIndex(in_index) {}

		T& operator[](u32 index) { return parent->data[attrib.offset + index]; }

		u32 size() const { return attrib.len; }

		void push_back(const T& val) {
			parent->push_back_impl(partIndex, val);
		}

		void clear() {
			parent->partAttribs[partIndex].len = 0;
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

		const T& operator[](u32 index) const { return parent->data[attrib.offset + index]; }

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

	void addPartition() {
		partAttribs.push_back(PartAttrib_impl{ static_cast<u32>(data.size()), 0, 1 });
		pushPart_impl(1);
	}

	void clear() {
		data.clear();
		partAttribs.clear();
	}
	// keep partitions
	void clearData() {
		for (PartAttrib_impl& i : partAttribs) {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (u32 j = 0; j < i.len; ++j) {
					data[i.offset + j] = T{};
				}
			}
			i.len = 0;
		}
	}


	It_t begin() { return data.begin(); }
	It_t end() { return data.end(); }
	ConstIt_t begin() const { return data.begin(); }
	ConstIt_t end() const { return data.end(); }

private:
	std::vector<T> data;
	std::vector<PartAttrib_impl> partAttribs;
	const u32 PartLen;

	void pushPart_impl(u32 n = 1) { // add n empty partition at the end
		data.resize(data.size() + PartLen * n);
	}
	void moveData_impl(u32 dest, u32 src, u32 len) {
		if (len == 0) return;

		T* pSrc = data.data() + src;
		T* pDst = data.data() + dest;

		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memcpy(pDst, pSrc, len * sizeof(T));
		} else {
			for (u32 i = 0; i < len; ++i) {
				pDst[i] = std::move(pSrc[i]);
			}
		}
	}

	// linear reallocate
	void push_back_impl(u32 partIndex, const T& val) {
		PartAttrib_impl& attrib = partAttribs[partIndex];
		if (attrib.len >= attrib.partCount * PartLen) { // if exceeded -> resize
			u32 targetOffset = attrib.offset + attrib.partCount * PartLen;

			auto it = std::find_if(partAttribs.begin(), partAttribs.end(),
			                       [targetOffset](const PartAttrib_impl& p) { return p.offset == targetOffset; });

			if (it != partAttribs.end()) { // if another partition is in the way -> move it to the back
				u32 nextOffset = data.size();
				pushPart_impl(it->partCount);
				moveData_impl(nextOffset, it->offset, it->len);
				it->offset = nextOffset;
			} else if (targetOffset == data.size()) { // if at the end of the data vector -> push new empty space
				pushPart_impl(1);
			}
			attrib.partCount++;
			// from now, the current partition should have a empty partition after it
		}
		data[attrib.offset + attrib.len++] = val;
	}
};

template <class T>
using PartedArr_Partition = typename PartedArr<T>::Partition_impl;
template <class T>
using PartedArr_ConstPartition = typename PartedArr<T>::ConstPartition_impl;
} // namespace tx