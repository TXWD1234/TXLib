// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/buffer.hpp"
#include "impl/data_utils.hpp"
#include "tx/basic_types.hpp"
#include "tx/upset.hpp"
#include <algorithm>
#include <cstring>

namespace tx::RenderEngine {

namespace impl::details {
const u32 UniformObjectAlignment = [] {
	i32 alignment;
	gl::getIntegerv(
	    gl::enums::UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
	return static_cast<u32>(alignment);
}();
} // namespace impl::details


template <class T>
    requires std::is_trivially_copyable_v<T>
class UniformBufferObject {
public:
	UniformBufferObject(u32 objectCount = DefaultStorageCount)
	    : m_buffer(objectCount, findObjectStorageCount_impl()) {}

	UniformBufferObject(const UniformBufferObject& other) = delete;
	UniformBufferObject& operator=(const UniformBufferObject& other) = delete;
	UniformBufferObject(UniformBufferObject&& other) = default;
	UniformBufferObject& operator=(UniformBufferObject&& other) = default;

	const BufferObject<T>& buffer() const { return m_buffer.buffer(); }
	operator const BufferObject<T>&() const { return m_buffer; }

	// ================ Buffer Management ================

	template <std::invocable<impl::RingBufferObjectDeleter> SubmitDeleter>
	void stage(const T& object, SubmitDeleter&& submitDeleter) {
		m_buffer.stage(
		    findObjectStorageCount_impl(),
		    std::forward<SubmitDeleter>(submitDeleter),
		    [&object](std::span<std::byte> buffer) {
			    std::memcpy(buffer.data(), std::addressof(object), sizeof(T));
		    });
	}

	// @return unit: bytes
	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	[[nodiscard]] u32 commit(SubmitMarker&& submitMarker) {
		return m_buffer.commit(
		           std::forward<SubmitMarker>(submitMarker)) *
		       DefaultStorageSize;
	}

	// ================ Draw Call ================

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	void bind(u32 binding, SubmitMarker&& submitMarker) {
		gl::bindBufferRange(
		    gl::enums::UNIFORM_BUFFER,
		    binding, m_buffer.buffer().id(),
		    commit(std::forward<SubmitMarker>(submitMarker)),
		    sizeof(T));
	}

private:
	static constexpr u32 DefaultStorageSize = 64;
	static constexpr u32 DefaultStorageCount = 4;
	struct alignas(16) Storage {
		std::byte data[DefaultStorageSize];
	};

private:
	RingBufferObject<Storage> m_buffer;

	static u32 findObjectStorageCount_impl() {
		return tx::nextAlign(
		           static_cast<u32>(sizeof(T)),
		           static_cast<u32>(impl::details::UniformObjectAlignment)) /
		       DefaultStorageSize;
	}
};
template <class T>
using UBO = UniformBufferObject<T>;

template <class T>
struct UniformObject : public T {
public:
	const UniformBufferObject<T>& buffer() const { return m_buffer; }

	template <std::invocable<impl::RingBufferObjectDeleter> SubmitDeleter>
	void stage(SubmitDeleter&& submitDeleter) {
		m_buffer.stage(*this, std::forward<SubmitDeleter>(submitDeleter));
	}

	// @return unit: bytes
	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	[[nodiscard]] u32 commit(SubmitMarker&& submitMarker) {
		return m_buffer.commit(std::forward<SubmitMarker>(submitMarker));
	}

	u32 m_binding = 0;

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	void bind(SubmitMarker&& submitMarker) {
		m_buffer.bind(m_binding, std::forward<SubmitMarker>(submitMarker));
	}

private:
	UniformBufferObject<T> m_buffer;
};
} // namespace tx::RenderEngine