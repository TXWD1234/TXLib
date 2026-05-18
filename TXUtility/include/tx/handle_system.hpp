// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "tx/basic_types.hpp"

namespace tx {
// be aware of the dangling reference of the deleted handles
template <std::integral T>
class HandleSystemBase {
public:
	using value_type = T;

	T addHandle() {
		if (!m_availableHandleBuffer.empty()) {
			T handle = pop_back();
			m_active[handle] = 1;
			return handle;
		}
		m_active.push_back(1);
		return m_handleMax++;
	}
	void deleteHandle(T handle) {
		if (handle < m_handleMax && m_active[handle]) {
			m_active[handle] = 0;
			m_availableHandleBuffer.push_back(handle);
		}
	}

	bool valid(T handle) const { return handle < m_handleMax && m_active[handle]; }

	tx::u32 count() { return m_handleMax - static_cast<tx::u32>(m_availableHandleBuffer.size()); }
	tx::u32 countMax() { return m_handleMax; }

	void reserve(tx::u32 count) { m_active.reserve(count); }
	void reserveDeletion(tx::u32 count) { m_availableHandleBuffer.reserve(count); }

private:
	std::vector<T> m_availableHandleBuffer;
	std::vector<tx::u8> m_active;
	T m_handleMax = 0; // aka the next handle

	T pop_back() {
		T back = m_availableHandleBuffer.back();
		m_availableHandleBuffer.pop_back();
		return back;
	}
};
using HandleSystem = HandleSystemBase<tx::u32>;
template <class T>
class HandleContainer {
public:
	tx::u32 addHandle(const T& value) {
		tx::u32 handle = m_hs.addHandle();
		resizeToFit(handle);
		m_handleValues[handle] = value;
		return handle;
	}
	tx::u32 addHandle(T&& value) {
		tx::u32 handle = m_hs.addHandle();
		resizeToFit(handle);
		m_handleValues[handle] = std::move(value);
		return handle;
	}

	void deleteHandle(tx::u32 handle) {
		if (handle < m_handleValues.size()) m_handleValues[handle] = T{};
		m_hs.deleteHandle(handle);
	}

	bool valid(tx::u32 handle) const { return m_hs.valid(handle); }

	T& operator[](tx::u32 handle) { return m_handleValues[handle]; }
	const T& operator[](tx::u32 handle) const { return m_handleValues[handle]; }

private:
	HandleSystem m_hs;
	std::vector<T> m_handleValues;

	void resizeToFit(tx::u32 index) {
		if (index >= m_handleValues.size())
			m_handleValues.resize(index + 1);
	}
};
}