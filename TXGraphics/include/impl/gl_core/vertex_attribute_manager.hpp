// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: vertex_attribute_manager.hpp

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "buffer.hpp"
#include <concepts>
#include <vector>

namespace tx {
namespace RenderEngine {
// VAO Wrapper
// SoA Design - One buffer per attribute
class VertexAttributeManager {
public:
	struct BindingState {
		gid bufferId = 0;
		u32 byteOffset = 0;
	};

	class Initializer;

	VertexAttributeManager() {}
	template <class InitFunc>
	    requires std::invocable<InitFunc, Initializer&>
	VertexAttributeManager(InitFunc f) {
		Initializer initer;
		f(initer);
		init_impl(std::move(initer));
	}
	VertexAttributeManager(Initializer&& initer) {
		init_impl(std::move(initer));
	}

	~VertexAttributeManager() {
		if (m_id) gl::deleteVertexArrays(1, &m_id);
	}
	// Move-Only Semantics
	VertexAttributeManager(const VertexAttributeManager&) = delete;
	VertexAttributeManager& operator=(const VertexAttributeManager&) = delete;
	VertexAttributeManager(VertexAttributeManager&& other) noexcept
	    : m_id(other.m_id), m_bindings(std::move(other.m_bindings)), m_useInstancing(other.m_useInstancing) {
		other.m_id = 0;
		other.m_useInstancing = 0;
	}
	VertexAttributeManager& operator=(VertexAttributeManager&& other) noexcept {
		if (this != &other) {
			if (m_id) gl::deleteVertexArrays(1, &m_id);
			m_id = other.m_id;
			m_bindings = std::move(other.m_bindings);
			m_useInstancing = other.m_useInstancing;
			other.m_id = 0;
			other.m_useInstancing = 0;
		}
		return *this;
	}

	class Initializer {
		friend class VertexAttributeManager;

	public:
		Initializer() { gl::createVertexArrays(1, &m_id); }
		~Initializer() {
			if (m_id) gl::deleteVertexArrays(1, &m_id);
		}

		Initializer(const Initializer&) = delete;
		Initializer& operator=(const Initializer&) = delete;
		Initializer(Initializer&& other) noexcept
		    : m_id(other.m_id), m_bindings(std::move(other.m_bindings)), m_useInstancing(other.m_useInstancing) {
			other.m_id = 0;
		}
		Initializer& operator=(Initializer&& other) noexcept {
			if (this != &other) {
				if (m_id) gl::deleteVertexArrays(1, &m_id);
				m_id = other.m_id;
				m_bindings = std::move(other.m_bindings);
				m_useInstancing = other.m_useInstancing;
				other.m_id = 0;
			}
			return *this;
		}

		// One buffer per attribute
		template <class T>
		u32 addAttrib() {
			u32 id = m_bindings.size();
			setBufferAttrib_impl<T>(m_id, id, m_attribCount);
			m_bindings.push_back({ 0, 0 });
			return id;
		}
		// buffer for instancing
		template <class T>
		u32 addAttribInstanced(u32 divisor = 1) {
			u32 id = m_bindings.size();
			setBufferAttrib_impl<T>(m_id, id, m_attribCount);
			gl::vertexArrayBindingDivisor(m_id, id, divisor);
			m_useInstancing = 1;
			m_bindings.push_back({ 0, 0 });
			return id;
		}

	private:
		gid m_id = 0;
		std::vector<BindingState> m_bindings;
		bool m_useInstancing = 0;
		u32 m_attribCount = 0;

		template <class T>
		static void setBufferAttrib_impl(u32 vamId, u32 id, u32& attribId) {
			using underlying = typename glAttributeParameter<T>::underlying;
			u32 relativeOffset = 0;
			for (u32 i = 0; i < glAttribCount<T>; i++) {
				if constexpr (sizeof(underlying) <= 4) {
					if constexpr (glAttributeParameter<T>::is_int) {
						gl::vertexArrayAttribIFormat(vamId, attribId, glComponentCount<T>, glType<T>, relativeOffset);
					} else {
						gl::vertexArrayAttribFormat(vamId, attribId, glComponentCount<T>, glType<T>, gl::enums::FALSE, relativeOffset);
					}
				} else if constexpr (sizeof(underlying) == 8) {
					if constexpr (glAttributeParameter<T>::is_int) {
						gl::vertexArrayAttribIFormat(vamId, attribId, glComponentCount<T> * 2, gl::enums::UNSIGNED_INT, relativeOffset); // Safe bindless mapping
					} else {
						gl::vertexArrayAttribLFormat(vamId, attribId, glComponentCount<T>, glType<T>, relativeOffset);
					}
				} else {
					static_assert(false_v<T>, "tx::RenderEngine::VertexAttributeManager::Initializer::setBufferAttrib_impl: Unsupported underlying type length.");
				}
				gl::enableVertexArrayAttrib(vamId, attribId);
				gl::vertexArrayAttribBinding(vamId, attribId++, id);
				relativeOffset += glComponentCount<T> * sizeof(underlying);
			}
		}
	};


	template <class T>
	void setBuffer(u32 id, const BufferObject<T>& bufferObject, u32 offset = 0) {
		if (id >= m_bindings.size()) return;
		u32 byteOffset = offset * sizeof(T);
		gid boId = bufferObject.id();

		if (m_bindings[id].bufferId != boId || m_bindings[id].byteOffset != byteOffset) {
			gl::vertexArrayVertexBuffer(m_id, id, boId, byteOffset, sizeof(T));
			m_bindings[id].bufferId = boId;
			m_bindings[id].byteOffset = byteOffset;
		}
	}
	template <class T>
	void setBuffer(u32 id, const RingBufferObject<T>& bufferObject, u32 offset = 0) {
		setBuffer(id, static_cast<const BufferObject<T>&>(bufferObject), offset);
	}

	gid id() const { return m_id; }
	u32 size() const { return m_bindings.size(); }

	bool useInstancing() const { return m_useInstancing; }

private:
	gid m_id = 0;
	std::vector<BindingState> m_bindings;
	bool m_useInstancing = 0;


	void init_impl(Initializer&& initer) {
		m_id = initer.m_id;
		m_bindings = std::move(initer.m_bindings);
		m_useInstancing = initer.m_useInstancing;
		initer.m_id = 0;
	}
};

using VAM = VertexAttributeManager;
using VAMIniter = VertexAttributeManager::Initializer;
} // namespace RenderEngine
} // namespace tx