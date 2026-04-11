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



class RendererFramework;
class RendererBufferBase {
	friend RendererFramework;
	virtual void regVAM(VAMIniter&);
	virtual void init(VAM&);
};
using RendererSBufferBase = RendererBufferBase;
class RendererDBufferBase : RendererBufferBase {
	friend RendererFramework;
	using ShaderProduct = std::variant<
	    ShaderProgram*,
	    ShaderPipeline*>;
	virtual void addSection();
	virtual u32 getOffset(u32 sectionIndex);
	virtual u32 getSize(u32 sectionIndex);
	[[nodiscard]] virtual u64 updateRingBufferOffset(FenceManager&);
	virtual void updateVAM(VAM& vam);
	// and a updateDynamicBuffer -> update ring buffer
};

template <class T>
class RendererSBuffer : RendererSBufferBase {
public:
	RendererSBuffer(std::span<T> data, bool isInstanced = 0)
	    : m_data(data), m_isInstanced(isInstanced) {}

private:
	BufferHandle<StaticBufferObject<T>> buffer;
	std::span<T> m_data;
	bool m_isInstanced = 0;

	void regVAM(VAMIniter& initer) override {
		if (m_isInstanced)
			buffer.id = initer.addAttrib<T>();
		else
			buffer.id = initer.addAttribInstanced<T>();
	}
	void init(VAM& vam) override {
		buffer.bo.alloc(m_data.size(), m_data.data());
		VAMSetBuffer(vam, buffer);
	}
};
template <class T>
class RendererDBuffer : RendererDBufferBase {
public:
	std::span<T> getSpan(u32 sectionIndex) { return std::span<T>{ stage[sectionIndex].begin(), stage[sectionIndex].end() }; }
	void push_back(u32 sectionIndex, const T& val) { stage[sectionIndex].push_back(val); }
	void push_back(u32 sectionIndex, T&& val) { stage[sectionIndex].push_back(std::move(val)); }

private:
	PartedArr<T> stage;
	BufferHandle<RingBufferObject<T>> buffer;

	void addSection() override { stage.addPartition(); }
	u32 getOffset(u32 sectionIndex) override { return stage[sectionIndex].offset(); }
	u32 getSize(u32 sectionIndex) override { return stage[sectionIndex].size(); }
	[[nodiscard]] u64 updateRingBufferDrawCall(FenceManager& fm) override { return buffer.bo.getNext(FMSubmiter{ fm }); }
	void updateVAM(VAM& vam) override { VAMSetBuffer(vam, buffer); }
};


// ***** render content that's calculated every frame *****
// The RenderContent is only a attrib reading layer

using ShaderProduct = std::variant<
    ShaderProgram*,
    ShaderPipeline*>;

// abstraction class for type erasure
class RenderContentBufferBase {
public:
	virtual u32 getOffset(u32 sectionIndex) const = 0;
	virtual u32 getSize(u32 sectionIndex) const = 0;
	virtual std::span<const std::byte> getData(u32 sectionIndex) const = 0;

	virtual TypeEnum type() const = 0;
};
template <class T>
class RenderContentBuffer : public RenderContentBufferBase {
public:
	RenderContentBuffer(const PartedArr<T>& data) : m_data(&data) {}

public:
	u32 getOffset(u32 sectionIndex) const override { return (*m_data)[sectionIndex].offset(); }
	u32 getSize(u32 sectionIndex) const override { return (*m_data)[sectionIndex].size(); }
	std::span<const std::byte> getData(u32 sectionIndex) const override {
		return std::as_bytes(std::span<T>{ (*m_data)[sectionIndex].begin(), (*m_data)[sectionIndex].end() });
	}

	TypeEnum type() const override {
		return type_enum_v<T>;
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
		void setShaderRegBuffer(const std::vector<ShaderProduct>& shaderRegBuffer) {
			m_shaderReg = &shaderRegBuffer;
			m_shaderRegSetted = 1;
		}

	private:
		std::vector<const RenderContentBufferBase*> m_buffers;
		const std::vector<ShaderProduct>* m_shaderReg;
		bool m_shaderRegSetted = 0;
	};
	template <std::invocable<Initer&> Func>
	RenderContent(Func&& f) {
		Initer initer;
		f(initer);
		init_impl(std::move(initer));
	}

	template <std::invocable<const RenderContentBufferBase*> Func>
	void foreach (Func&& f) const {
		for (const RenderContentBufferBase* i : m_buffers) {
			f(i);
		}
	}

	bool empty() const { return m_buffers.empty() || m_shaderReg->empty(); }
	bool valid() const { return m_valid; }
	u32 bufferCount() const { return m_buffers.size(); }
	u32 sectionCount() const { return m_shaderReg->size(); }

	const RenderContentBufferBase* buffer(u32 bufferIndex) const { return m_buffers[bufferIndex]; }
	std::span<const std::byte> getData(u32 bufferIndex, u32 sectionIndex) const { return m_buffers[bufferIndex]->getData(sectionIndex); }

	bool checkOffsetSame(u32 sectionIndex) const {
		if (m_buffers.empty()) throw std::runtime_error("tx::RE::RenderContent::checkOffsetSame(): parameter sectionIndex out of bound");
		u32 expected = m_buffers[0]->getOffset(sectionIndex);
		for (size_t i = 1; i < m_buffers.size(); i++) {
			if (expected != m_buffers[i]->getOffset(sectionIndex)) return false;
		}
		return true;
	}
	bool checkSizeSame(u32 sectionIndex) const {
		if (m_buffers.empty() || sectionIndex >= m_buffers.size()) throw std::runtime_error("tx::RE::RenderContent::checkOffsetSame(): parameter sectionIndex out of bound");
		u32 expected = m_buffers[0]->getSize(sectionIndex);
		for (size_t i = 1; i < m_buffers.size(); i++) {
			if (expected != m_buffers[i]->getSize(sectionIndex)) return false;
		}
		return true;
	}
	bool getSize(u32 sectionIndex) const {
		if (m_buffers.empty() || sectionIndex >= m_buffers.size()) throw std::runtime_error("tx::RE::RenderContent::getSize(): parameter sectionIndex out of bound");
		u32 expected = m_buffers[0]->getSize(sectionIndex);
		for (size_t i = 1; i < m_buffers.size(); i++) {
			if (expected != m_buffers[i]->getSize(sectionIndex)) return InvalidU32;
		}
		return expected;
	}

private:
	std::vector<const RenderContentBufferBase*> m_buffers;
	const std::vector<ShaderProduct>* m_shaderReg; // defines sections
	bool m_valid = 0;

	void init_impl(Initer&& initer) {
		m_buffers = std::move(initer.m_buffers);
		m_shaderReg = std::move(initer.m_shaderReg);
		m_valid = initer.m_shaderRegSetted;
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

private:
	std::vector<ShaderProduct> m_shaderRegBuffer;
	bool m_valid = 0;

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
		      initer.setShaderRegBuffer(m_shaderRegBuffer);
	      }) {
		m_valid = content.valid();
	}

public:
	bool valid() const { return m_valid; }
};


// ***** the context for renderer *****
// manages vam. FenceManage, and buffers (static buffer and ring buffers)

struct RenderContextBufferBase {
	virtual void regVAM(VAMIniter& initer);
	virtual void init(VAM& vam);

	virtual TypeEnum type() const = 0;
};

struct RenderContextStaticBufferBase : public RenderContextBufferBase {
	virtual u64 size() const = 0;
};
// static buffer cannot be instanced
template <class T>
struct RenderContextStaticBuffer : public RenderContextStaticBufferBase {
	RenderContextStaticBuffer(std::span<T> data)
	    : m_data(data) {}

	// static buffer must be non instanced
	void regVAM(VAMIniter& initer) override {
		m_buffer.id = initer.addAttrib<T>();
	}
	void init(VAM& vam) override {
		m_buffer.bo.alloc(m_data.size(), m_data.data());
		VAMSetBuffer(vam, m_buffer);
	}
	u64 size() const override {
		return m_buffer.bo.size();
	}

	TypeEnum type() const override {
		return type_enum_v<T>;
	}

	BufferHandle<StaticBufferObject<T>> m_buffer;
	std::span<T> m_data;
};

struct RenderContextDynamicBufferBase : public RenderContextBufferBase {
public:
	virtual void pushRingBuffer(std::span<std::span<const std::byte>> data, FenceManager& fm, VAM& vam) = 0;
	[[nodiscard]] virtual u32 getDrawCallOffset(FenceManager& fm) = 0;
};
template <class T>
struct RenderContextDynamicBuffer : public RenderContextDynamicBufferBase {
public:
	RenderContextDynamicBuffer(bool instanced) : m_instanced(instanced) {}

	void regVAM(VAMIniter& initer) override {
		if (m_instanced)
			m_buffer.id = initer.addAttribInstanced<T>();
		else
			m_buffer.id = initer.addAttrib<T>();
	}
	void init(VAM& vam) {
		m_buffer.bo.alloc();
		VAMSetBuffer(vam, m_buffer);
	}

	void pushRingBuffer(std::span<std::span<const std::byte>> data, FenceManager& fm, VAM& vam) override {
		u32 size = 0;
		for (std::span<const std::byte> i : data) {
			size += i.size();
		}
		m_buffer.bo.push(
		    size,
		    [&](std::span<std::byte> mappedData) {
			    u32 offset = 0;
			    for (std::span<const std::byte> i : data) {
				    std::copy(i.begin(), i.end(), mappedData.begin() + offset);
				    offset += i.size();
			    }
		    },
		    FMSubmiter{ fm });
		VAMSetBuffer(vam, m_buffer);
	}
	// get offset of the ring buffer
	[[nodiscard]] u32 getDrawCallOffset(FenceManager& fm) override {
		return m_buffer.bo.getNext(FMSubmiter{ fm });
	}

	TypeEnum type() const override {
		return type_enum_v<T>;
	}

	bool instanced() const { return m_instanced; }

private:
	BufferHandle<RingBufferObject<T>> m_buffer;
	bool m_instanced;
};





class RenderContext {
	struct RingBufferPushDataComposer;

public:
	struct Initer {
		friend class RenderContext;

	public:
		template <class T>
		void addSBuffer(std::span<T> data) {
			m_SBuffers.emplace_back(std::make_unique<RenderContextStaticBuffer<T>>(data));
		}
		template <class T>
		void addDBufferInstanced() {
			m_DBuffersInstanced.emplace_back(std::make_unique<RenderContextDynamicBuffer<T>>(true));
		}

	private:
		// static buffer cannot be instanced
		std::vector<std::unique_ptr<RenderContextStaticBufferBase>> m_SBuffers;
		// std::vector<std::unique_ptr<RenderContextDynamicBufferBase>> m_DBuffers; add this later: Dynamic Mesh <---------------------------------
		std::vector<std::unique_ptr<RenderContextDynamicBufferBase>> m_DBuffersInstanced;
	};
	template <std::invocable<Initer&> Func>
	RenderContext(Func&& f) {
		Initer initer;
		f(initer);
		init_impl(std::move(initer));
	}

	void push(const RenderContent& content) {
		if (!content.valid()) throw std::runtime_error("tx::RE::RenderContext::push(): invalid content input");
		if (content.bufferCount() != m_DBuffersInstanced.size()) throw std::runtime_error("tx::RE::RenderContext::push(): content input buffer size don't match");
		if (!pushTypeCheck(content)) throw std::runtime_error("tx::RE::RenderContext::push(): content input type don't match");
		if (content.empty()) return;

		for (u32 section = 0; section < content.sectionCount(); section++) {
			if (!content.checkSizeSame(section)) continue; // maybe use throw instead? <------------------------------------------------------------------------------
			for (u32 buffer = 0; buffer < content.bufferCount(); buffer++)
				m_DBuffersInstanced[buffer]->pushRingBuffer(
				    content.getData(buffer, section),
				    fm, vam);
		}
	}


	// getters for renderer

	u32 getStaticSize() const { return m_staticMeta.size; }
	bool staticSizeValid() const { return m_staticMeta.size && tx::valid(m_staticMeta.size); }



private:
	VAM vam;
	FenceManager fm;

	std::vector<std::unique_ptr<RenderContextStaticBufferBase>> m_SBuffers;
	std::vector<std::unique_ptr<RenderContextDynamicBufferBase>> m_DBuffersInstanced;

	struct staticBufferMeta_impl {
		u32 size = InvalidU32;
	} m_staticMeta;


	void init_impl(Initer&& initer) {
		m_SBuffers = std::move(initer.m_SBuffers);
		m_DBuffersInstanced = std::move(initer.m_DBuffersInstanced);

		// init vam
		vam = VAM([&](VAMIniter& initer) {
			foreach_impl([&](RenderContextBufferBase* buffer) {
				buffer->regVAM(initer);
			});
		});

		// init buffer
		foreach_impl([&](RenderContextBufferBase* buffer) {
			buffer->init(vam);
		});

		// init sbuf meta
		if (!m_SBuffers.empty()) {
			m_staticMeta.size = m_SBuffers[0]->size();
			for (u32 i = 1; i < m_SBuffers.size(); i++) {
				if (m_SBuffers[i]->size() != m_staticMeta.size) {
					m_staticMeta.size = tx::InvalidU32;
					break;
				}
			}
		}
	}

	template <std::invocable<RenderContextBufferBase*> Func>
	void foreach_impl(Func&& f) {
		for (std::unique_ptr<RenderContextStaticBufferBase>& i : m_SBuffers) {
			f(i.get());
		}
		for (std::unique_ptr<RenderContextDynamicBufferBase>& i : m_DBuffersInstanced) {
			f(i.get());
		}
	}

	bool pushTypeCheck(const RenderContent& content) const {
		bool valid = 1;
		u32 i = 0;
		content.foreach ([&](const RenderContentBufferBase* contentBuf) {
			if (!valid) {
				i++;
				return;
			}
			valid = contentBuf->type() == m_DBuffersInstanced[i]->type();
			i++;
		});
		return valid;
	}

	struct RingBufferPushDataComposer {
		RingBufferPushDataComposer(RenderContext* context, u32 bufferCount) : m_context(context), m_data(bufferCount, 64) {}

		void push(std::span<const std::byte> data, u32 bufferIndex) {
			m_data[bufferIndex].push_back(data);
		}
		void compose() {
			for (u32 i = 0; i < m_context->m_DBuffersInstanced.size(); i++) {
				m_context->m_DBuffersInstanced[i]->pushRingBuffer(m_data[i].span(), m_context->fm, m_context->vam);
			}
			m_data.clearData();
		}

	private:
		RenderContext* m_context;
		PartedArr<std::span<const std::byte>> m_data; // replacing vector<vector> - each partition is a buffer's data
	};
};












class RendererFramework {
public:
	RendererFramework() {
	}
	void init() {
		initBuffers_impl();
	}


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