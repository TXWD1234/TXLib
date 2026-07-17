// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "tx/basic_types.hpp"
#include <span>
#include <concepts>

namespace tx {
/**
 * This class doesn't own the data, but it manipulate it
 * Everytime before you push, you should check `full()`.
 *   Any push operation on an full CircularQueue is Undefined Behavior
 * Everytime before you pop, you should check `empty()`.
 *   Any pop operation on an empty CircularQueue is Undefined Behavior
 */
template <class T>
class CircularQueueOverlay {
public:
	CircularQueueOverlay(std::span<T> data)
	    : m_data(data) {}

	using value_type = T;

	// basic operations

	void push(const T& val) {
		m_data[m_end] = val;
		push_impl();
	}
	void push(T&& val) {
		m_data[m_end] = std::move(val);
		push_impl();
	}

	void pop() {
		if constexpr (!std::is_trivial_v<T>) {
			m_data[m_begin] = T{};
		}
		pop_impl();
	}

	void clear() {
		foreach_impl([&](T& val) {
			val = T{};
		});

		m_begin = 0;
		m_end = 0;
		m_wrap = false;
	}


	// basic getter

	bool full() const { return m_begin == m_end && m_wrap; }
	bool empty() const { return m_begin == m_end && !m_wrap; }
	u32 size() const {
		return (m_wrap ?
		            m_data.size() - m_begin + m_end :
		            m_end - m_begin);
	}


private:
	std::span<T> m_data;
	u32 m_begin = 0, m_end = 0;
	bool m_wrap = false;

	void push_impl() {
		m_end++;
		if (m_end == m_data.size()) {
			m_end = 0; // wrapping logic
			m_wrap = true;
		}
	}
	void pop_impl() {
		m_begin++;
		if (m_begin == m_data.size()) {
			m_begin = 0; // wrapping logic
			m_wrap = false;
		}
	}

	template <std::invocable<T&> Func>
	void foreach_impl(Func&& f) {
		if (m_wrap) {
			for (u32 i = m_begin; i < m_data.size(); i++) {
				f(m_data[i]);
			}
			for (u32 i = 0; i < m_end; i++) {
				f(m_data[i]);
			}
		} else {
			for (u32 i = m_begin; i < m_end; i++) {
				f(m_data[i]);
			}
		}
	}
};

template <class T>
class CircularQueue {
public:
	CircularQueue(u32 capacity)
	    : m_data(capacity), m_overlay(m_data) {}
	using value_type = T;

	void push(const T& val) { m_overlay.push(val); }
	void push(T&& val) { m_overlay.push(std::move(val)); }

	void pop() { m_overlay.pop(); }

	void clear() { m_overlay.clear(); }

	// basic getter

	bool full() const { return m_overlay.full(); }
	bool empty() const { return m_overlay.empty(); }
	u32 size() const { return m_overlay.size(); }

private:
	std::vector<T> m_data;
	CircularQueueOverlay<T> m_overlay;
};
} // namespace tx