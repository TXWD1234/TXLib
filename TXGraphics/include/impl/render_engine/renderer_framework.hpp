// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: renderer_framework.hpp

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/draw_call_manager.hpp"
#include "impl/gl_core/buffer.hpp"
#include "impl/gl_core/vertex_attribute_manager.hpp"
#include "impl/gl_core/shader.hpp"
#include "impl/gl_core/utility.hpp"
#include "impl/render_engine/texture_manager.hpp"
#include "impl/parted_arr.hpp"

#include <variant>


namespace tx::RenderEngine {





// ***** render content that's calculated every frame *****

// abstraction class for type erasure
class RenderContentBufferBase {
public:
	virtual u32 getOffset(u32 sectionIndex) const = 0;
	virtual u32 getSize(u32 sectionIndex) const = 0;
	virtual void copyTo(u32 sectionIndex, std::span<std::byte> destination) const = 0;
};
template <class T>
class RenderContentBuffer : public RenderContentBufferBase {
public:
	RenderContentBuffer(const PartedArr<T>& data) : m_data(&data) {}

public:
	u32 getOffset(u32 sectionIndex) const override { return (*m_data)[sectionIndex].offset(); }
	u32 getSize(u32 sectionIndex) const override { return (*m_data)[sectionIndex].size(); }
	void copyTo(u32 sectionIndex, std::span<std::byte> dest) const override {
		std::span<std::byte> data = std::as_bytes(std::span<T>{ (*m_data)[sectionIndex].begin(), (*m_data)[sectionIndex].end() });
		if (dest.size_bytes() < data.size_bytes()) std::runtime_error("tx::RenderContentBuffer::copyTo(): provided destination is smaller then m_data.");
		std::copy(data.begin(), data.end(), dest.begin());
	}

private:
	const PartedArr<T>* m_data;
};
// render content that's calculated every frame
class RenderContent {
public:
	struct Initer {
		friend class RenderContent;

	public:
		void addBuffer(const RenderContentBufferBase& buffer) {
			m_buffers.push_back(&buffer);
		}

	private:
		std::vector<const RenderContentBufferBase*> m_buffers;
	};
	template <std::invocable<Initer&> Func>
	RenderContent(Func&& f) {
		Initer initer;
		f(initer);
		init_impl(std::move(initer));
	}

	template <std::invocable<const RenderContentBufferBase*> Func>
	void foreach (Func&& f) {
		for (const RenderContentBufferBase* i : m_buffers) {
			f(i);
		}
	}

	bool empty() { return m_buffers.empty(); }


	u32 getOffset(u32 sectionIndex) {
		if (m_buffers.empty()) return InvalidU32;
		u32 offset = m_buffers[0]->getOffset(sectionIndex);
		for (size_t i = 1; i < m_buffers.size(); i++) {
			if (offset != m_buffers[i]->getOffset(sectionIndex)) return InvalidU32;
		}
		return offset;
	}
	u32 getSize(u32 sectionIndex) {
		if (m_buffers.empty()) return InvalidU32;
		u32 offset = m_buffers[0]->getSize(sectionIndex);
		for (size_t i = 1; i < m_buffers.size(); i++) {
			if (offset != m_buffers[i]->getSize(sectionIndex)) return InvalidU32;
		}
		return offset;
	}

private:
	std::vector<const RenderContentBufferBase*> m_buffers;

	void init_impl(Initer&& initer) {
		m_buffers = std::move(initer.m_buffers);
	}
};
using RC = RenderContent;
using RCIniter = RenderContent::Initer;

struct RenderContent2D {
	PartedArr<vec2> position;
	PartedArr<vec2> scale;

private:
	RenderContentBuffer<vec2> positionBuffer_impl;
	RenderContentBuffer<vec2> scaleBuffer_impl;

public:
	RenderContent content;

	RenderContent2D(const RenderContent2D&) = delete;
	RenderContent2D& operator=(const RenderContent2D&) = delete;
	RenderContent2D(RenderContent2D&&) = delete;
	RenderContent2D& operator=(RenderContent2D&&) = delete;

	RenderContent2D()
	    : positionBuffer_impl(position), scaleBuffer_impl(scale),
	      content([&](RCIniter& initer) {
		      initer.addBuffer(positionBuffer_impl);
		      initer.addBuffer(scaleBuffer_impl);
	      }) {
	}
};

// ***** the  *****

class RenderContextBufferBase {
public:
};



class RenderContext {
public:
	struct Initer {
	public:
	private:
	};



private:
};












class RendererFramework {
public:
	RendererFramework() {
	}
	void init() {
		initBuffers_impl();
	}

	using ShaderProduct = std::variant<
	    ShaderProgram*,
	    ShaderPipeline*>;

	u32 registerSection(ShaderProduct sp) {
		return addSection_impl(sp);
	}
	u32 registerSection(ShaderPipeline& shaderPipeline) { return registerSection(&shaderPipeline); }
	u32 registerSection(ShaderProgram& shaderProgram) { return registerSection(&shaderProgram); }


	// draw call
	void draw();






	FenceManager& getFM() { return fm; }

private:
	template <class T>
	struct DBuffer_impl {
		PartedArr<T> stage;
		BufferHandle<RingBufferObject<T>> buffer;
	};

	// sections are separated by different shaders
	struct SectionMeta {
		ShaderProduct shader;
		u32 offset, size;
	};

private:
	DrawCallManager dcm;
	VertexAttributeManager vam;
	FenceManager fm;

	std::vector<SectionMeta> sectionMetaDatas;
	u32 ringBufferOffset = 0;



	void initBuffers_impl();

	// section & metadata
	// mote: each section is a different shader, which means one draw call

	u32 addSection_impl(const ShaderProduct& sp);

	[[nodiscard]] bool getStageBufferSectionMetaData_impl(u32 sectionIndex);
	[[nodiscard]] bool updateRingBufferOffset();

	// OpenGL

	void addInstance_impl(u32 section, vec2 position, u64 textureHandle, float textureIndex, vec2 scale, mat2 rotation, u32 color);

	// update for id changes
	void updateVAM();

	// draw call
	// note: all dynamic buffers should have the same length in the same section

	template <class T>
	void updateDynamicBuffer_impl(DBuffer_impl<T>& buffer) {
		u32 size = buffer.stage.dataSize();
		buffer.buffer.bo.push(
		    size, [&](std::span<T> input) {
			    std::copy(buffer.stage.begin(), buffer.stage.end(), input.begin());
		    },
		    FMSubmiter{ fm });
	}
	void updateDynamicBuffers_impl();

	// the actual single draw call
	void draw_impl(u32 sectionIndex);

	// clean up

	void clearStage_impl();
};

} // namespace tx::RenderEngine