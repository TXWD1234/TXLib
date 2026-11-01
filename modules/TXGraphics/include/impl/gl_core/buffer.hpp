// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/data_utils.hpp"
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/fence.hpp"
#include "tx/basic_types.hpp"
#include "tx/upset.hpp"
#include <memory>
#include <concepts>
#include <span>
#include <type_traits>

namespace tx::RenderEngine {
namespace impl {
enum class ioMode : u32 {
	Static = 0,
	Dynamic =
	    (gl::enums::MAP_WRITE_BIT |
		 gl::enums::MAP_PERSISTENT_BIT |
		 gl::enums::MAP_COHERENT_BIT)
};
}

// Can only be move or referenced. Copying is disabled.
template <class T>
class BufferObject {
public:
	gid id() const { return m_id; }

	BufferObject(const BufferObject&) = delete;
	BufferObject& operator=(const BufferObject&) = delete;

	BufferObject(BufferObject&& other) {
		m_id = other.m_id;
		other.m_id = 0;
	}
	BufferObject& operator=(BufferObject&& other) {
		if (this != &other) {
			if (m_id) gl::deleteBuffers(1, &m_id);
			m_id = other.m_id;
			other.m_id = 0;
		}
		return *this;
	}

protected:
	BufferObject() = default;
	virtual ~BufferObject() {
		if (m_id) gl::deleteBuffers(1, &m_id);
	}

	// after reinit, the caller of will take ownership of the old buffer
	gid reinit() {
		gid oldId = m_id;
		m_id = 0;
		gl::createBuffers(1, &m_id);
		return oldId;
	}

	gid m_id = 0;
};

// Can only be move or refernced. Copying is disabled.
template <class T>
class StaticBufferObject : public BufferObject<T> {
public:
	StaticBufferObject(u32 size, const T* ptr = nullptr)
	    : BufferObject<T>() { alloc_impl(size, ptr); }

	u32 size() const { return m_size; }

private:
	u32 m_size;

	void alloc_impl(u32 count, const T* ptr = nullptr) {
		if (!this->m_id) gl::createBuffers(1, &this->m_id);
		gl::namedBufferStorage(this->m_id, count * sizeof(T), (void*)ptr, enumval(impl::ioMode::Static));
		m_size = count;
	}
};
template <class T>
using SBO = StaticBufferObject<T>;

// Can only be move or refernced. Copying is disabled.
template <class T>
class DynamicBufferObject : public BufferObject<T> {
public:
	DynamicBufferObject(u32 size, const T* ptr = nullptr)
	    : BufferObject<T>() { alloc_impl(size, ptr); }
	~DynamicBufferObject() override {
		if (m_data) gl::unmapNamedBuffer(this->m_id);
	}

	DynamicBufferObject(DynamicBufferObject&& other) noexcept
	    : BufferObject<T>(std::move(other)), // Move the ID
	      m_data(other.m_data),
	      m_size(other.m_size) {
		other.m_data = nullptr;
		other.m_size = 0;
	}
	DynamicBufferObject& operator=(DynamicBufferObject&& other) noexcept {
		if (this != &other) {
			if (m_data) gl::unmapNamedBuffer(this->m_id);
			BufferObject<T>::operator=(std::move(other));
			m_data = other.m_data;
			m_size = other.m_size;
			other.m_data = nullptr;
			other.m_size = 0;
		}
		return *this;
	}

	// after resize, the caller will take the ownership of the old buffer
	// *Surprise! You own an OpenGL buffer now.*
	[[nodiscard]] gid resize(u32 count, const T* ptr = nullptr) {
		gid oldId = this->reinit();
		if (m_data) gl::unmapNamedBuffer(oldId);

		alloc_impl(count, ptr);

		return oldId;
	}

	u32 size() const { return m_size; }
	T* data() { return m_data; }

private:
	T* m_data = nullptr;
	u32 m_size = 0;

	void alloc_impl(u32 count, const T* ptr = nullptr) {
		if (!this->m_id) gl::createBuffers(1, &this->m_id);
		gl::namedBufferStorage(this->m_id, count * sizeof(T), (void*)ptr, enumval(impl::ioMode::Dynamic));
		m_data = static_cast<T*>(gl::mapNamedBufferRange(this->m_id, 0, count * sizeof(T), enumval(impl::ioMode::Dynamic)));
		m_size = count;
	}
};
template <class T>
using DBO = DynamicBufferObject<T>;

namespace impl {
// fencing stuff
struct RingBufferObjectDeleter {
	gid id = 0;
	void operator()() const {
		if (id) gl::deleteBuffers(1, &id);
	}
};
struct RingBufferObjectMarker {
	u32 targetOffset = 0;
	std::shared_ptr<u32> currentOffset;
	void operator()() const {
		if (currentOffset) (*currentOffset) = targetOffset;
	}
};
} // namespace impl

template <class T>
class RingBufferObject {
public:
	RingBufferObject(u32 bufferCount = DefaultBufferCount,
	                 u32 bufferSize = DefaultBufferSize)
	    : m_buffer(bufferCount * bufferSize),
	      m_usageFence(std::make_shared<u32>(0)) {}

	RingBufferObject(const RingBufferObject&) = delete;
	RingBufferObject& operator=(const RingBufferObject&) = delete;
	RingBufferObject(RingBufferObject&&) noexcept = default;
	RingBufferObject& operator=(RingBufferObject&&) noexcept = default;

	gid id() const { return m_buffer.id(); }

	// ================ Buffer Manipulation Interface ================

	// This function may trigger a reallocation, thereby changing the id of
	// the ring buffer.
	// Therefore a VAMSetBuffer must be called after invoking this function.
	// originally named: push
	template <class Func, std::invocable<impl::RingBufferObjectDeleter&&> SubmitDeleter>
	    requires std::invocable<Func, std::span<T>> ||
		         std::invocable<Func, std::span<std::byte>>
	void stage(u32 size, SubmitDeleter&& submitDeleter, Func&& f) {
		if (!size) return;

		m_preparedPart = IndexRange{ m_preparedPart.end(), size };

		// ring buffer logic: wrap the entire partition to the start
		bool wrapped = m_preparedPart.end() > m_buffer.size();
		if (wrapped) m_preparedPart.offset = 0;

		// Collision happens if we wrap and overlap the GPU read head, or if we
		// wrap into an unstarted GPU read head
		bool collision = m_preparedPart.offset <= *m_usageFence &&
		                 m_preparedPart.end() >= *m_usageFence && !m_isFirstIteration;
		// collision or complete overflow both triggers reallocation
		// using while loop and checking overflow here for complete overflow
		// case (size is bigger then the entire buffer)
		while (collision || m_preparedPart.end() > m_buffer.size()) {
			resize_impl(submitDeleter);
			collision = false;
		}
		m_isFirstIteration = false;

		// dispatch writer; writes directly to GPU mapped memory
		if constexpr (std::is_invocable_v<Func, std::span<std::byte>>) {
			f(std::as_writable_bytes(std::span<T>(m_buffer.data() + m_preparedPart.offset, size)));
		} else {
			f(std::span<T>(m_buffer.data() + m_preparedPart.offset, size));
		}
	}
	template <class U, std::invocable<impl::RingBufferObjectDeleter> SubmitDeleter>
	    requires std::same_as<U, const T> || std::same_as<U, const std::byte>
	void stage(std::span<U> dataBuffer, SubmitDeleter&& submitDeleter) {
		stage(
		    static_cast<u32>(dataBuffer.size()),
		    std::forward<SubmitDeleter>(submitDeleter),
		    [dataBuffer](std::span<std::remove_const_t<U>> mappedData) {
			    std::copy(dataBuffer.begin(), dataBuffer.end(), mappedData.begin());
		    });
	}

	// ================ Draw Call Associated Interface ================

	// originally named: getNext
	template <std::invocable<impl::RingBufferObjectMarker&&> SubmitMarker>
	[[nodiscard]] tx::IndexRange commit(SubmitMarker&& submitMarker) {
		// new draw call
		submitMarker(impl::RingBufferObjectMarker{
		    m_preparedPart.offset, m_usageFence });
		return m_preparedPart;
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

public:
	static constexpr u32 DefaultBufferSize = 8192;
	static constexpr u32 DefaultBufferCount = 4;

private:
	u32 m_bufferSize = DefaultBufferSize, m_bufferCount = DefaultBufferCount;

	template <class Func>
	    requires std::invocable<Func, impl::RingBufferObjectDeleter&&>
	void resize_impl(Func&& submitDeleter) {
		m_bufferSize *= 2;
		submitDeleter(impl::RingBufferObjectDeleter{ m_buffer.resize(m_bufferSize * m_bufferCount) });
		m_preparedPart.offset = 0;
		m_usageFence = std::make_shared<u32>(0);
	}

private:
	DynamicBufferObject<T> m_buffer;
	tx::IndexRange m_preparedPart;
	std::shared_ptr<u32> m_usageFence; // GPU Usage Begin
	bool m_isFirstIteration = 1;
};
template <class T>
using RBO = RingBufferObject<T>;

// return InvalidU32 if failed
template <class SubmitMarker, class First, class... Rest>
    requires std::invocable<SubmitMarker, impl::RingBufferObjectMarker&&>
u32 getRingBufferOffset(SubmitMarker&& submitMarker, RingBufferObject<First>& first, RingBufferObject<Rest>&... rest) {
	u32 expected = first.commit(submitMarker).offset;
	u32 result = expected;
	if constexpr (sizeof...(Rest) > 0) {
		((rest.commit(submitMarker).offset != expected ? (result = InvalidU32) : 0), ...);
	}
	return result;
}

template <class T, class SubmitDeleter, class Func>
    requires std::invocable<Func, std::vector<T>&> && std::invocable<SubmitDeleter, impl::RingBufferObjectDeleter&&>
void writeRingBuffer(RingBufferObject<T>& buffer, SubmitDeleter&& submitDeleter, Func&& modifier) {
	std::vector<T> stagingBuffer;
	modifier(stagingBuffer);
	buffer.stage(std::span<T>(stagingBuffer), std::forward<SubmitDeleter>(submitDeleter));
}





} // namespace tx::RenderEngine