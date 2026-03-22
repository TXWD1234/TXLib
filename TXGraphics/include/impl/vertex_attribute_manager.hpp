// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: vertex_attribute_manager.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"
#include "buffer.hpp"

namespace tx {
namespace RenderEngine {
// VAO Wrapper
// SoA Design - One buffer per attribute
class VertexAttributeManager {
public:
	class Initializer;

	VertexAttributeManager() {}
	template <class InitFunc>
	VertexAttributeManager(InitFunc f) {
		static_assert(std::is_invocable_v<InitFunc, Initializer&>, "tx::RE::VertexAttributeManager: InitFunc must take Initializer&");
		Initializer initer;
		f(initer);
		init_impl(std::move(initer));
	}
	VertexAttributeManager(Initializer&& initer) {
		init_impl(std::move(initer));
	}

	~VertexAttributeManager() {
		if (m_id) glDeleteVertexArrays(1, &m_id);
	}
	// Move-Only Semantics
	VertexAttributeManager(const VertexAttributeManager&) = delete;
	VertexAttributeManager& operator=(const VertexAttributeManager&) = delete;
	VertexAttributeManager(VertexAttributeManager&& other) noexcept
	    : m_id(other.m_id), m_entryCount(other.m_entryCount), m_useInstancing(other.m_useInstancing) {
		other.m_id = 0;
		other.m_entryCount = 0;
		other.m_useInstancing = 0;
	}
	VertexAttributeManager& operator=(VertexAttributeManager&& other) noexcept {
		if (this != &other) {
			if (m_id) glDeleteVertexArrays(1, &m_id);
			m_id = other.m_id;
			m_entryCount = other.m_entryCount;
			m_useInstancing = other.m_useInstancing;
			other.m_id = 0;
			other.m_entryCount = 0;
			other.m_useInstancing = 0;
		}
		return *this;
	}

	class Initializer {
		friend class VertexAttributeManager;

	public:
		Initializer() { glCreateVertexArrays(1, &m_id); }
		~Initializer() {
			if (m_id) glDeleteVertexArrays(1, &m_id);
		}

		Initializer(const Initializer&) = delete;
		Initializer& operator=(const Initializer&) = delete;
		Initializer(Initializer&& other) noexcept
		    : m_id(other.m_id), m_entryCount(other.m_entryCount), m_useInstancing(other.m_useInstancing) {
			other.m_id = 0;
		}
		Initializer& operator=(Initializer&& other) noexcept {
			if (this != &other) {
				if (m_id) glDeleteVertexArrays(1, &m_id);
				m_id = other.m_id;
				m_entryCount = other.m_entryCount;
				m_useInstancing = other.m_useInstancing;
				other.m_id = 0;
			}
			return *this;
		}

		// One buffer per attribute
		template <class T>
		u32 addAttrib() {
			setBufferAttrib_impl<T>(m_id, m_entryCount);
			return m_entryCount++;
		}
		// buffer for instancing
		template <class T>
		u32 addAttribInstanced(u32 divisor = 1) {
			setBufferAttrib_impl<T>(m_id, m_entryCount);
			glVertexArrayBindingDivisor(m_id, m_entryCount, divisor);
			m_useInstancing = 1;
			return m_entryCount++;
		}

	private:
		gid m_id = 0;
		u32 m_entryCount = 0;
		bool m_useInstancing = 0;

		template <class T>
		static void setBufferAttrib_impl(u32 vamId, u32 id) {
			if constexpr (glAttributeParameter<T>::is_int) {
				glVertexArrayAttribIFormat(vamId, id, glComponentCount<T>, glType<T>, 0);
			} else {
				glVertexArrayAttribFormat(vamId, id, glComponentCount<T>, glType<T>, GL_FALSE, 0);
			}
			glEnableVertexArrayAttrib(vamId, id);
			glVertexArrayAttribBinding(vamId, id, id);
		}
		template <class T>
		static void setBuffer_impl(u32 vamId, const BufferObject<T>& bo, u32 id, u32 offset = 0) {
			glVertexArrayVertexBuffer(vamId, id, bo.id(), offset * sizeof(T), sizeof(T));
		}
	};


	template <class T>
	void setBuffer(u32 id, const BufferObject<T>& bufferObject, u32 offset = 0) {
		Initializer::setBuffer_impl(m_id, bufferObject, id, offset);
	}
	template <class T>
	void setBuffer(u32 id, const RingBufferObject<T>& bufferObject, u32 offset = 0) {
		Initializer::setBuffer_impl(m_id, static_cast<const BufferObject<T>&>(bufferObject), id, offset);
	}

	gid id() const { return m_id; }
	u32 size() const { return m_entryCount; }

	bool useInstancing() const { return m_useInstancing; }

private:
	gid m_id = 0;
	u32 m_entryCount = 0;
	bool m_useInstancing = 0;


	void init_impl(Initializer&& initer) {
		m_id = initer.m_id;
		m_entryCount = initer.m_entryCount;
		m_useInstancing = initer.m_useInstancing;
		initer.m_id = 0;
	}
};

using VAM = VertexAttributeManager;
using VAMIniter = VertexAttributeManager::Initializer;
} // namespace RenderEngine
} // namespace tx