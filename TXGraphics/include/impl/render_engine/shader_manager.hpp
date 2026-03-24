// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: shader_manager.hpp

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include "impl/render_engine/shader.hpp"
#include "impl/render_engine/log.hpp"

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

enum class ShaderManageStyle {
	Program,
	Pipeline
};
using SMStyle = ShaderManageStyle;
using SMType = ShaderManageStyle;

template <ShaderManageStyle type = ShaderManageStyle::Program>
class ShaderManager;
template <ShaderManageStyle type = ShaderManageStyle::Program>
using SM = ShaderManager<type>;

#if 0
template <>
class ShaderManager<ShaderManageStyle::Program> {
public:
	ShaderManageStyle type = ShaderManageStyle::Program;

	ShaderManager() = default;
	~ShaderManager() = default;

	// Disable Copy
	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;

	// Move Constructor/Assignment
	ShaderManager(ShaderManager&& other) noexcept = default;
	ShaderManager& operator=(ShaderManager&& other) noexcept = default;

	template <class... Args>
	ShaderId addVertexShader(Args&&... args) {
		vertexShaders.emplace_back(std::forward<Args>(args)...);
		updateGSVert();
		return ShaderId{ ShaderType::Vertex, static_cast<u32>(vertexShaders.size() - 1) };
	}

	template <class... Args>
	ShaderId addFragmentShader(Args&&... args) {
		fragmentShaders.emplace_back(std::forward<Args>(args)...);
		updateGSFrag();
		return ShaderId{ ShaderType::Fragment, static_cast<u32>(fragmentShaders.size() - 1) };
	}

	ProgramId linkShaders(ShaderId vertId, ShaderId fragId) {
		linkProgram_impl(vertId.id, fragId.id);
		return ProgramId{ vertId, fragId };
	}

	ShaderProgram& getShaderProgram(ProgramId pid) {
		ShaderProgram& program = programTable[pid.vertId][pid.fragId];
		if (!program.id()) {
			linkProgram_impl(pid.vertId, pid.fragId); // lazy linking
		}
		return program;
	}
	ShaderProgram& getShaderProgram(ShaderId vertId, ShaderId fragId) {
		return getShaderProgram(ProgramId{ vertId, fragId });
	}

	// debug

	bool compileSucceed(ShaderId sid) {
		bool success = false;
		visit_impl(sid, [&](const auto& shader) {
			success = shader.valid();
		});
		return success;
	}
	std::string getCompileLog(ShaderId sid) {
		std::string str;
		visit_impl(sid, [&](const auto& shader) {
			str = shader.getLog();
		});
		return str;
	}
	bool linkSucceed(ProgramId pid) {
		return getShaderProgram(pid).valid();
	}
	std::string getLinkLog(ProgramId pid) {
		return getShaderProgram(pid).getLog();
	}

private:
	std::vector<VertexShader> vertexShaders;
	std::vector<FragmentShader> fragmentShaders;
	std::vector<std::vector<ShaderProgram>> programTable; // [vertId][fragId]

	void linkProgram_impl(u32 vertId, u32 fragId) {
		programTable[vertId][fragId] = ShaderProgram(vertexShaders[vertId], fragmentShaders[fragId]);
	}

	// GridSystem
	void updateGSVert() {
		programTable.emplace_back(fragmentShaders.size());
	}
	void updateGSFrag() {
		for (auto& i : programTable) {
			i.emplace_back();
		}
	}

	template <class Func>
	void visit_impl(ShaderId sid, Func f) {
		if (sid.type == ShaderType::Vertex)
			f(vertexShaders[sid.id]);
		else if (sid.type == ShaderType::Fragment)
			f(fragmentShaders[sid.id]);
	}
};

template <>
class ShaderManager<ShaderManageStyle::Pipeline> {
public:
	using Initializer = ShaderManager<ShaderManageStyle::Pipeline>;
	ShaderManageStyle type = ShaderManageStyle::Pipeline;

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

	PipelineId linkShaders(ShaderId vertId, ShaderId fragId) {
		linkPipeline_impl(vertId.id, fragId.id);
		return PipelineId{ vertId, fragId };
	}

	ShaderPipeline& getShaderPipeline(PipelineId pid) {
		ShaderPipeline& pipeline = pipelineTable[pid.vertId][pid.fragId];
		if (!pipeline.id()) {
			linkPipeline_impl(pid.vertId, pid.fragId); // lazy linking
		}
		return pipeline;
	}
	ShaderPipeline& getShaderPipeline(ShaderId vertId, ShaderId fragId) {
		return getShaderPipeline(PipelineId{ vertId, fragId });
	}

	// debug

	bool compileSucceed(ShaderId sid) {
		bool valid = false;
		visit_impl(sid, [&](const auto& program) {
			valid = program.compileSucceed();
		});
		return valid;
	}
	std::string getCompileLog(ShaderId sid) {
		std::string str;
		visit_impl(sid, [&](const auto& program) {
			str = program.getCompileLog();
		});
		return str;
	}
	bool componentLinkSucceed(ShaderId sid) {
		bool valid = false;
		visit_impl(sid, [&](const auto& program) {
			valid = program.linkSucceed();
		});
		return valid;
	}
	std::string getComponentLinkLog(ShaderId sid) {
		std::string str;
		visit_impl(sid, [&](const auto& program) {
			str = program.getLinkLog();
		});
		return str;
	}
	bool linkSucceed(PipelineId pid) {
		return getShaderPipeline(pid).valid();
	}
	std::string getLinkLog(PipelineId pid) {
		return getShaderPipeline(pid).getLog();
	}

private:
	std::vector<ShaderProgramComponent<ShaderType::Vertex>> vertexComponents;
	std::vector<ShaderProgramComponent<ShaderType::Fragment>> fragmentComponents;
	std::vector<std::vector<ShaderPipeline>> pipelineTable; // [vertId][fragId]

	void linkPipeline_impl(u32 vertId, u32 fragId) {
		pipelineTable[vertId][fragId] = ShaderPipeline(vertexComponents[vertId], fragmentComponents[fragId]);
	}

	// GridSystem
	void updateGSVert() {
		pipelineTable.emplace_back(fragmentComponents.size());
	}
	void updateGSFrag() {
		for (auto& i : pipelineTable) {
			i.emplace_back();
		}
	}

	template <class Func>
	void visit_impl(ShaderId sid, Func f) {
		if (sid.type == ShaderType::Vertex)
			f(vertexComponents[sid.id]);
		else if (sid.type == ShaderType::Fragment)
			f(fragmentComponents[sid.id]);
	}
};
#endif

template <ShaderManageStyle style>
struct SMTraits;

template <>
struct SMTraits<ShaderManageStyle::Program> {
private:
	friend class ShaderManager<ShaderManageStyle::Program>;
	using VertComponent = VertexShader;
	using FragComponent = FragmentShader;
	using Product = ShaderProgram;

	static bool compileSucceed(const VertComponent& comp) { return comp.valid(); }
	static bool compileSucceed(const FragComponent& comp) { return comp.valid(); }

	static std::string getCompileLog(const VertComponent& comp) { return comp.getLog(); }
	static std::string getCompileLog(const FragComponent& comp) { return comp.getLog(); }

	static Product make_linked(const VertComponent& v, const FragComponent& f) {
		return ShaderProgram(v, f);
	}
};

template <>
struct SMTraits<ShaderManageStyle::Pipeline> {
private:
	friend class ShaderManager<ShaderManageStyle::Pipeline>;
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

template <ShaderManageStyle style>
class ShaderManager {
public:
	using Initializer = ShaderManager<style>;
	using Traits = SMTraits<style>;
	using VertComponent = typename Traits::VertComponent;
	using FragComponent = typename Traits::FragComponent;
	using Product = typename Traits::Product;

	ShaderManageStyle type = style;

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

	// Aliases strictly to prevent breaking previously written usage
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

} // namespace RenderEngine
} // namespace tx
