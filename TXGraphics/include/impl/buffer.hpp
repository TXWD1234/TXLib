// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: buffer_manager.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"
#include "fence.hpp"
#include <deque>
#include <concepts>
#include <span>

namespace tx::RenderEngine {
// strict
enum class BufferType : u32 {
	Static = 0,
	Dynamic = 0x0002 | 0x0040 | 0x0080 // GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
};
using ioMode = BufferType;
using iom = BufferType;

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
	}

private:
	bool m_allocated = 0;
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
		previous_previous.offset = 0;
		m_allocated = true;
	}

	// Disable Copy
	RingBufferObject(const RingBufferObject&) = delete;
	RingBufferObject& operator=(const RingBufferObject&) = delete;
	RingBufferObject(RingBufferObject&&) noexcept = default;
	RingBufferObject& operator=(RingBufferObject&&) noexcept = default;

	// buffer manipulation

	//std::vector<T>& getStagingBuffer() { return dataBuffer; }
	//const std::vector<T>& getStagingBuffer() const { return dataBuffer; }


	void finish(std::span<T> dataBuffer) {

		if (dataBuffer.empty()) {
			return;
		}

		current.count = dataBuffer.size();

		if (current.end() > buffer.size()) current.offset = 0;

		bool collision = false;
		for (const auto& f : fences) {
			if (f.range.count > 0 && current.offset < f.range.end() && current.end() > f.range.offset) {
				collision = true;
				break;
			}
		}
		while (collision || current.end() > buffer.size()) {
			resize();
			collision = false;
		}

		std::copy(dataBuffer.begin(), dataBuffer.end(), buffer.data() + current.offset);
		//userdataBuffer.clear();

		previous = current;
		current.offset = current.end();
		current.count = 0;
	}

	// draw call associated

	u64 getNext() {
		// new draw call
		fences.push_back(FenceEntry_impl{ Fence{}, std::move(deleteId), previous_previous });
		previous_previous = previous;
		deleteId = std::vector<gid>();

		resolveFences();

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

	// fencing stuff
	struct FenceEntry_impl {
		Fence fence;
		std::vector<gid> deleteIds;
		Region range;
	};

	std::deque<FenceEntry_impl> fences;

	void resolveFences() {
		while (!fences.empty() && fences.front().fence.isFinished()) {
			FenceEntry_impl& entry = fences.front();
			for (gid i : entry.deleteIds) {
				gl::deleteBuffers(1, &i);
			}
			fences.pop_front();
		}
	}

	void resize() {
		bufferSize *= 2;
		deleteId.push_back(buffer.resize(bufferSize * bufferCount));
		current.offset = 0;
		for (auto& f : fences) f.range.count = 0; // Clear collisions for the NEW blank buffer!
		previous_previous.count = 0;
	}

private:
	DynamicBufferObject<T> buffer;
	//std::vector<T> dataBuffer;
	Region previous_previous;
	Region previous;
	Region current;
	std::vector<gid> deleteId;
	bool m_allocated = false;
};


// return UINT64_MAX if failed
template <class First, class... Rest>
u64 getRingBufferOffset(RingBufferObject<First>& first, RingBufferObject<Rest>&... rest) {
	u64 expected = first.getNext();
	u64 result = expected;
	if constexpr (sizeof...(Rest) > 0) {
		((rest.getNext() != expected ? (result = UINT64_MAX) : 0), ...);
	}
	return result;
}

template <class T, std::invocable<std::vector<T>&> Func>
void writeRingBuffer(RingBufferObject<T>& buffer, Func&& modifier) {
	std::vector<T> stagingBuffer;
	modifier(stagingBuffer);
	buffer.finish(std::span<T>(stagingBuffer));
}





} // namespace tx::RenderEngine