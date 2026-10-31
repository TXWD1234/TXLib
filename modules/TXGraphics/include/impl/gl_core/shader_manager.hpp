// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXGraphics

#pragma once
#include "impl/gl_core/basic_gl_utils.hpp"
#include "impl/gl_core/shader.hpp"

namespace tx {
namespace RenderEngine {
struct ShaderId {
	ShaderType type;
	u32 id;
};

struct ShaderPair {
	u32 vertId = 0, fragId = 0;
	ShaderPair() = default;
	ShaderPair(u32 vert, u32 frag) : vertId(vert), fragId(frag) {}
	ShaderPair(ShaderId vert, ShaderId frag) : vertId(vert.id), fragId(frag.id) {}
};
using ProgramId = ShaderPair;
using PipelineId = ShaderPair;

// shader manage style
enum class SMStyle {
	Program,
	Pipeline
};

template <SMStyle type = SMStyle::Program>
class ShaderManager;

namespace impl {
template <SMStyle style>
struct SMTraits;

template <>
struct SMTraits<SMStyle::Program> {
private:
	friend class ShaderManager<SMStyle::Program>;
	using VertComponent = VertexShader;
	using FragComponent = FragmentShader;
	using Product = ShaderProgram;

	static bool compileSucceed(const VertComponent& comp) { return comp.valid(); }
	static bool compileSucceed(const FragComponent& comp) { return comp.valid(); }

	static std::string getCompileLog(const VertComponent& comp) { return comp.getLog_impl(); }
	static std::string getCompileLog(const FragComponent& comp) { return comp.getLog_impl(); }

	static Product make_linked(const VertComponent& v, const FragComponent& f) {
		return ShaderProgram(v, f);
	}
};

template <>
struct SMTraits<SMStyle::Pipeline> {
private:
	friend class ShaderManager<SMStyle::Pipeline>;
	using VertComponent = ShaderProgramComponent<ShaderType::Vertex>;
	using FragComponent = ShaderProgramComponent<ShaderType::Fragment>;
	using Product = ShaderPipeline;

	static bool compileSucceed(const VertComponent& comp) { return comp.compileSucceed() && comp.linkSucceed(); }
	static bool compileSucceed(const FragComponent& comp) { return comp.compileSucceed() && comp.linkSucceed(); }

	static std::string getCompileLog(const VertComponent& comp) { return comp.getCompileLog() + comp.getLinkLog(); }
	static std::string getCompileLog(const FragComponent& comp) { return comp.getCompileLog() + comp.getLinkLog(); }

	static Product make_linked(const VertComponent& v, const FragComponent& f) {
		return ShaderPipeline(v, f);
	}
};
} // namespace impl

template <SMStyle style>
class ShaderManager {
public:
	using Traits = impl::SMTraits<style>;
	using VertComponent = typename Traits::VertComponent;
	using FragComponent = typename Traits::FragComponent;
	using Product = typename Traits::Product;

	SMStyle type = style;

	ShaderManager() = default;
	~ShaderManager() = default;

	// Disable Copying
	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;

	// Move Constructor/Assignment
	ShaderManager(ShaderManager&& other) noexcept = default;
	ShaderManager& operator=(ShaderManager&& other) noexcept = default;

	template <class... Args>
	ShaderId addVertexShader(Args&&... args) {
		vertexComponents.emplace_back(VertexShader{ std::forward<Args>(args)... });
		updateGSVert();
		return ShaderId{ ShaderType::Vertex, static_cast<u32>(vertexComponents.size() - 1) };
	}

	template <class... Args>
	ShaderId addFragmentShader(Args&&... args) {
		fragmentComponents.emplace_back(FragmentShader{ std::forward<Args>(args)... });
		updateGSFrag();
		return ShaderId{ ShaderType::Fragment, static_cast<u32>(fragmentComponents.size() - 1) };
	}

	ShaderPair linkShaders(ShaderId vertId, ShaderId fragId) {
		link_impl(vertId.id, fragId.id);
		return ShaderPair{ vertId, fragId };
	}

	Product& get(ShaderPair pid) {
		Product& linked = table[pid.vertId][pid.fragId];
		if (!linked.id()) {
			link_impl(pid.vertId, pid.fragId); // lazy linking
		}
		return linked;
	}
	Product& get(ShaderId vertId, ShaderId fragId) {
		return get(ShaderPair{ vertId, fragId });
	}

	Product& getShaderProgram(ShaderPair pid) { return get(pid); }
	Product& getShaderProgram(ShaderId vertId, ShaderId fragId) { return get(vertId, fragId); }
	Product& getShaderPipeline(ShaderPair pid) { return get(pid); }
	Product& getShaderPipeline(ShaderId vertId, ShaderId fragId) { return get(vertId, fragId); }

	// debug

	bool compileSucceed(ShaderId sid) {
		bool valid = false;
		visit_impl(sid, [&](const auto& comp) { valid = Traits::compileSucceed(comp); });
		return valid;
	}
	std::string getCompileLog(ShaderId sid) {
		std::string str;
		visit_impl(sid, [&](const auto& comp) { str = Traits::getCompileLog(comp); });
		return str;
	}
	bool linkSucceed(ShaderPair pid) { return get(pid).valid(); }
	std::string getLinkLog(ShaderPair pid) { return get(pid).getLog(); }

private:
	std::vector<VertComponent> vertexComponents;
	std::vector<FragComponent> fragmentComponents;
	std::vector<std::vector<Product>> table; // [vertId][fragId]

	void link_impl(u32 vertId, u32 fragId) {
		table[vertId][fragId] = Traits::make_linked(vertexComponents[vertId], fragmentComponents[fragId]);
	}

	// GridSystem
	void updateGSVert() {
		table.emplace_back(fragmentComponents.size());
	}
	void updateGSFrag() {
		for (auto& i : table) { i.emplace_back(); }
	}

	template <class Func>
	void visit_impl(ShaderId sid, Func f) {
		if (sid.type == ShaderType::Vertex)
			f(vertexComponents[sid.id]);
		else if (sid.type == ShaderType::Fragment)
			f(fragmentComponents[sid.id]);
	}
};
template <SMStyle type = SMStyle::Program>
using SM = ShaderManager<type>;

} // namespace RenderEngine
} // namespace tx
