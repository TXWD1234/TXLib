// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/fence.hpp"
#include <memory>
#include <deque>
#include <concepts>
#include <span>

namespace tx::RenderEngine {
// strict
enum class BufferType : u32 {
	Static = 0,
	Dynamic = gl::enums::MAP_WRITE_BIT | gl::enums::MAP_PERSISTENT_BIT | gl::enums::MAP_COHERENT_BIT
};
using ioMode = BufferType;
using iom = BufferType;

// fencing stuff
struct RingBufferObjectDeleter {
	gid id = 0;
	void operator()() const {
		if (id) gl::deleteBuffers(1, &id);
	}
};
struct RingBufferObjectMarker {
	u64 targetOffset = 0;
	std::shared_ptr<u64> currentOffset;
	void operator()() const {
		if (currentOffset) (*currentOffset) = targetOffset;
	}
};

// Can only be move or referenced. Copying is disabled.
template <class T>
class BufferObject {
public:
	using value_type = T;
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
		if (m_id) {
			gl::deleteBuffers(1, &m_id);
		}
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
	StaticBufferObject()
	    : BufferObject<T>() {}
	StaticBufferObject(u64 count, const T* ptr = nullptr)
	    : BufferObject<T>() { alloc(count, ptr); }

	void alloc(u64 count, const T* ptr = nullptr) {
		if (m_allocated)
			throw std::runtime_error("tx::RenderEngine::BufferObject: alloc() called twice.");
		if (!this->m_id) gl::createBuffers(1, &this->m_id);
		gl::namedBufferStorage(this->m_id, count * sizeof(T), (void*)ptr, enumval(ioMode::Static));
		m_allocated = 1;
		m_size = count;
	}

	u64 size() const { return m_size; }

private:
	bool m_allocated = 0;
	u32 m_size;
};
template <class T>
using SBO = StaticBufferObject<T>;

// Can only be move or refernced. Copying is disabled.
template <class T>
class DynamicBufferObject : public BufferObject<T> {
public:
	DynamicBufferObject()
	    : BufferObject<T>() {}
	DynamicBufferObject(u64 count, const T* ptr = nullptr)
	    : BufferObject<T>() { alloc(count, ptr); }
	~DynamicBufferObject() override {
		if (m_data) gl::unmapNamedBuffer(this->m_id);
	}

	DynamicBufferObject(DynamicBufferObject&& other) noexcept
	    : BufferObject<T>(std::move(other)), // Move the ID
	      m_data(other.m_data),
	      m_size(other.m_size),
	      m_allocated(other.m_allocated) {
		other.m_data = nullptr;
		other.m_size = 0;
		other.m_allocated = false;
	}
	DynamicBufferObject& operator=(DynamicBufferObject&& other) noexcept {
		if (this != &other) {
			if (m_data) gl::unmapNamedBuffer(this->m_id);
			BufferObject<T>::operator=(std::move(other));
			m_data = other.m_data;
			m_size = other.m_size;
			m_allocated = other.m_allocated;
			other.m_data = nullptr;
			other.m_size = 0;
			other.m_allocated = false;
		}
		return *this;
	}

	void alloc(u64 count, const T* ptr = nullptr) {
		if (m_allocated)
			throw std::runtime_error("tx::RenderEngine::BufferObject: alloc() called twice.");
		if (!this->m_id) gl::createBuffers(1, &this->m_id);
		gl::namedBufferStorage(this->m_id, count * sizeof(T), (void*)ptr, enumval(ioMode::Dynamic));
		m_data = reinterpret_cast<T*>(gl::mapNamedBufferRange(this->m_id, 0, count * sizeof(T), enumval(ioMode::Dynamic)));
		m_size = count;
		m_allocated = 1;
	}
	// after resize, the caller will take the ownership of the old buffer
	gid resize(u64 count, const T* ptr = nullptr) {
		gid oldId = this->reinit();
		if (m_data) gl::unmapNamedBuffer(oldId);
		m_allocated = false;

		alloc(count, ptr);

		return oldId;
	}


	u64 size() const { return m_size; }
	T* data() { return m_data; }

	using It_t = T*;
	It_t begin() { return m_data; }
	It_t end() { return m_data + m_size; }

private:
	T* m_data = nullptr;
	u64 m_size = 0;
	bool m_allocated = 0;
};
template <class T>
using DBO = DynamicBufferObject<T>;

template <class T>
class RingBufferObject {
public:
	using value_type = T;

	struct Region;
	RingBufferObject() = default;

	void setBufferSize(u32 in) {
		if (!m_allocated) bufferSize = in;
	}
	void setBufferCount(u32 in) {
		if (!m_allocated) bufferCount = in;
	}

	void alloc() {
		if (m_allocated)
			throw std::runtime_error("tx::RenderEngine::RingBufferObject: alloc() called twice.");
		buffer.alloc(bufferCount * bufferSize);
		current.offset = 0;
		previous.offset = 0;
		m_allocated = true;
		GPUUsageBegin = std::make_shared<u64>(0);
	}

	// Disable Copy
	RingBufferObject(const RingBufferObject&) = delete;
	RingBufferObject& operator=(const RingBufferObject&) = delete;
	RingBufferObject(RingBufferObject&&) noexcept = default;
	RingBufferObject& operator=(RingBufferObject&&) noexcept = default;

	// buffer manipulation

	//std::vector<T>& getStagingBuffer() { return dataBuffer; }
	//const std::vector<T>& getStagingBuffer() const { return dataBuffer; }

	// this function might cause a reallocation, changing the id of the ring buffer, therefore a VAMSetBuffer have to be called.
	template <class Func, class SubmitDeleter>
	    requires std::invocable<SubmitDeleter, RingBufferObjectDeleter&&> && (std::invocable<Func, std::span<T>> || std::invocable<Func, std::span<std::byte>>)
	void push(u32 size, Func&& f, SubmitDeleter&& submitDeleter) {
		if (!size) return;

		current.count = size;

		bool wrapped = current.end() > buffer.size();
		if (wrapped) current.offset = 0;

		// Collision happens if we wrap and lap the GPU read head, or if we wrap into an unstarted GPU read head
		bool collision = current.offset <= *GPUUsageBegin && current.end() >= *GPUUsageBegin && !isFirstIteration;
		while (collision || current.end() > buffer.size()) {
			resize_impl(submitDeleter);
			collision = false;
		}
		isFirstIteration = 0;

		if constexpr (std::is_invocable_v<Func, std::span<std::byte>>) {
			f(std::as_writable_bytes(std::span<T>(buffer.data() + current.offset, size)));
		} else {
			f(std::span<T>(buffer.data() + current.offset, size));
		}

		previous = current;
		current.offset = current.end();
		current.count = 0;
	}
	// this function might cause a reallocation, changing the id of the ring buffer, therefore a VAMSetBuffer have to be called.
	template <class SubmitDeleter>
	    requires std::invocable<SubmitDeleter, RingBufferObjectDeleter&&>
	void push(std::span<T> dataBuffer, SubmitDeleter&& submitDeleter) {
		this->push(dataBuffer.size(), [dataBuffer](std::span<T> mappedData) { std::copy(dataBuffer.begin(), dataBuffer.end(), mappedData.begin()); }, std::forward<SubmitDeleter>(submitDeleter));
	}
	// this function might cause a reallocation, changing the id of the ring buffer, therefore a VAMSetBuffer have to be called.
	template <class SubmitDeleter>
	    requires std::invocable<SubmitDeleter, RingBufferObjectDeleter&&>
	void push(std::span<const std::byte> dataBuffer, SubmitDeleter&& submitDeleter) {
		this->push(
		    dataBuffer.size(),
		    [dataBuffer](std::span<T> mappedData) {
			    std::span<std::byte> mappedDataByte = std::as_writable_bytes(mappedData);
			    std::copy(dataBuffer.begin(), dataBuffer.end(), mappedDataByte.begin());
		    },
		    std::forward<SubmitDeleter>(submitDeleter));
	}

	// draw call associated

	template <class Func>
	    requires std::invocable<Func, RingBufferObjectMarker&&>
	u64 getNext(Func&& submitMarker) {
		// new draw call
		submitMarker(RingBufferObjectMarker{ previous.offset, GPUUsageBegin });
		return previous.offset;
	}
	operator const BufferObject<T>&() const { return buffer; }



	struct Region {
		u64 offset = 0;
		u32 count = 0;
		u64 end() const { return offset + count; }
	};

	u64 getBufferSize() const { return bufferSize; }


private:
	constexpr inline static u32 InitialBufferSize = 8192;
	constexpr inline static u32 InitialBufferCount = 4;
	u32 bufferSize = InitialBufferSize, bufferCount = InitialBufferCount;

	template <class Func>
	    requires std::invocable<Func, RingBufferObjectDeleter&&>
	void resize_impl(Func&& submitDeleter) {
		bufferSize *= 2;
		submitDeleter(RingBufferObjectDeleter{ buffer.resize(bufferSize * bufferCount) });
		current.offset = 0;
		GPUUsageBegin = std::make_shared<u64>(0);
	}

private:
	DynamicBufferObject<T> buffer;
	Region previous;
	Region current;
	std::shared_ptr<u64> GPUUsageBegin;
	bool m_allocated = false;
	bool isFirstIteration = 1;
};


// return UINT64_MAX if failed
template <class SubmitMarker, class First, class... Rest>
    requires std::invocable<SubmitMarker, RingBufferObjectMarker&&>
u64 getRingBufferOffset(SubmitMarker&& submitMarker, RingBufferObject<First>& first, RingBufferObject<Rest>&... rest) {
	u64 expected = first.getNext(submitMarker);
	u64 result = expected;
	if constexpr (sizeof...(Rest) > 0) {
		((rest.getNext(submitMarker) != expected ? (result = UINT64_MAX) : 0), ...);
	}
	return result;
}

template <class T, class SubmitDeleter, class Func>
    requires std::invocable<Func, std::vector<T>&> && std::invocable<SubmitDeleter, RingBufferObjectDeleter&&>
void writeRingBuffer(RingBufferObject<T>& buffer, SubmitDeleter&& submitDeleter, Func&& modifier) {
	std::vector<T> stagingBuffer;
	modifier(stagingBuffer);
	buffer.push(std::span<T>(stagingBuffer), std::forward<SubmitDeleter>(submitDeleter));
}





} // namespace tx::RenderEngine