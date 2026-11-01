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
const u32 ShaderStorageObjectAlignment = [] {
	i32 alignment;
	gl::getIntegerv(
	    gl::enums::SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
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

	template <std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
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
		                   std::forward<SubmitMarker>(submitMarker))
		           .offset *
		       DefaultStorageSize;
	}

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	[[nodiscard]] u32 push(
	    const T& object, SubmitDeleter&& submitDeleter, SubmitMarker&& submitMarker) {
		stage(object, std::forward<SubmitDeleter>(submitDeleter));
		return commit(std::forward<SubmitMarker>(submitMarker));
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

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	void sync(const T& object, u32 binding, SubmitDeleter&& submitDeleter, SubmitMarker&& submitMarker) {
		stage(object, std::forward<SubmitDeleter>(submitDeleter));
		bind(binding, std::forward<SubmitMarker>(submitMarker));
	}

private:
	static constexpr u32 DefaultStorageSize = 64;
	static constexpr u32 DefaultStorageCount = 4;
	struct alignas(16) Storage {
		std::byte data[DefaultStorageSize];
	};

private:
	RingBufferObject<Storage> m_buffer;

	// division will not truncate to 0 because UniformObjectAlignment must be
	// either 64 or 256, according to reality
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

	template <std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	void stage(SubmitDeleter&& submitDeleter) {
		m_buffer.stage(*this, std::forward<SubmitDeleter>(submitDeleter));
	}

	// @return unit: bytes
	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	[[nodiscard]] u32 commit(SubmitMarker&& submitMarker) {
		return m_buffer.commit(std::forward<SubmitMarker>(submitMarker));
	}

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	[[nodiscard]] u32 push(
	    SubmitDeleter&& submitDeleter, SubmitMarker&& submitMarker) {
		stage(*this, std::forward<SubmitDeleter>(submitDeleter));
		return commit(std::forward<SubmitMarker>(submitMarker));
	}

	u32 m_binding = 0;

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	void bind(SubmitMarker&& submitMarker) {
		m_buffer.bind(m_binding, std::forward<SubmitMarker>(submitMarker));
	}

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	void sync(SubmitDeleter&& submitDeleter, SubmitMarker&& submitMarker) {
		m_buffer.sync(*this, m_binding,
		              std::forward<SubmitDeleter>(submitDeleter),
		              std::forward<SubmitMarker>(submitMarker));
	}

private:
	UniformBufferObject<T> m_buffer;
};

template <class T>
    requires std::is_trivially_copyable_v<T>
class ShaderStorageBufferObject {
public:
	ShaderStorageBufferObject(u32 objectCount = DefaultStorageCount)
	    : m_buffer(objectCount, findObjectStorageCount_impl()) {}

	ShaderStorageBufferObject(const ShaderStorageBufferObject& other) = delete;
	ShaderStorageBufferObject& operator=(const ShaderStorageBufferObject& other) = delete;
	ShaderStorageBufferObject(ShaderStorageBufferObject&& other) = default;
	ShaderStorageBufferObject& operator=(ShaderStorageBufferObject&& other) = default;

	const BufferObject<u8>& buffer() const { return m_buffer.buffer(); }
	operator const BufferObject<u8>&() const { return m_buffer; }

	// ================ Buffer Management ================

	template <class Func, std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	    requires std::invocable<Func, std::span<T>> ||
		         std::invocable<Func, std::span<std::byte>>
	void stage(u32 size, SubmitDeleter&& submitDeleter, Func&& f) {
		m_buffer.stage(
		    tx::nextAlign(size * sizeof(T), impl::details::ShaderStorageObjectAlignment),
		    std::forward<SubmitDeleter>(submitDeleter),
		    [&](std::span<std::byte> data) {
			    if constexpr (std::invocable<Func, std::span<T>>)
				    f(std::span<T>(reinterpret_cast<T*>(data.data()), size));
			    else { f(data.subspan(0, size * sizeof(T))); }
		    });
	}
	template <class U, std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	    requires std::same_as<U, const T> || std::same_as<U, const std::byte>
	void stage(std::span<U> dataBuffer, SubmitDeleter&& submitDeleter) {
		m_buffer.stage(
		    static_cast<u32>(dataBuffer.size() * sizeof(U)),
		    std::forward<SubmitDeleter>(submitDeleter),
		    [dataBuffer](std::span<std::remove_const_t<U>> mappedData) {
			    std::copy(dataBuffer.begin(), dataBuffer.end(), mappedData.begin());
		    });
	}

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	IndexRange commit(SubmitMarker&& submitMarker) {
		return m_buffer.commit(std::forward<SubmitMarker>(submitMarker));
	}


	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter,
	          class Func>
	    requires std::invocable<Func, std::span<T>> ||
		         std::invocable<Func, std::span<std::byte>>
	[[nodiscard]] tx::IndexRange push(
	    u32 size, SubmitDeleter&& submitDeleter, SubmitMarker&& submitMarker, Func&& f) {
		stage(size, std::forward<SubmitDeleter>(submitDeleter), std::forward<Func>(f));
		return commit(std::forward<SubmitMarker>(submitMarker));
	}
	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter,
	          class U>
	    requires std::same_as<U, const T> || std::same_as<U, const std::byte>
	[[nodiscard]] tx::IndexRange push(
	    std::span<U> dataBuffer, SubmitDeleter&& submitDeleter, SubmitMarker&& submitMarker) {
		stage(dataBuffer, std::forward<SubmitDeleter>(submitDeleter));
		return commit(std::forward<SubmitMarker>(submitMarker));
	}

	// ================ Draw Call ================

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	void bind(u32 binding, SubmitMarker&& submitMarker) {
		IndexRange range = m_buffer.commit(std::forward<SubmitMarker>(submitMarker));
		gl::bindBufferRange(gl::enums::SHADER_STORAGE_BUFFER,
		                    binding, m_buffer.buffer().id(),
		                    range.offset, range.size);
	}

	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter,
	          class Func>
	    requires std::invocable<Func, std::span<T>> ||
		         std::invocable<Func, std::span<std::byte>>
	void sync(u32 size, u32 binding, SubmitDeleter&& submitDeleter,
	          SubmitMarker&& submitMarker, Func&& f) {
		stage(size, std::forward<SubmitDeleter>(submitDeleter), std::forward<Func>(f));
		bind(binding, std::forward<SubmitMarker>(submitMarker));
	}
	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker,
	          std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter,
	          class U>
	    requires std::same_as<U, const T> || std::same_as<U, const std::byte>
	void sync(std::span<U> dataBuffer, u32 binding, SubmitDeleter&& submitDeleter,
	          SubmitMarker&& submitMarker) {
		stage(dataBuffer, std::forward<SubmitDeleter>(submitDeleter));
		bind(binding, std::forward<SubmitMarker>(submitMarker));
	}

private:
	static constexpr u32 DefaultStorageCount = 4;

private:
	RingBufferObject<u8> m_buffer;

	static u32 findObjectStorageCount_impl() {
		return tx::nextAlign(
		    static_cast<u32>(sizeof(T)),
		    static_cast<u32>(impl::details::ShaderStorageObjectAlignment));
	}
};
template <class T>
using SSBO = ShaderStorageBufferObject<T>;
} // namespace tx::RenderEngine