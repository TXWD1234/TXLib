// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: shader_manager.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"
#include "impl/shader.hpp"
#include "impl/log.hpp"

namespace tx {
namespace RenderEngine {
struct ShaderId {
	ShaderType type;
	u32 id;
	bool valid() const { return type != ShaderType::Invalid; }
};

struct ShaderPair {
	u32 vertId = 0, fragId = 0;
	bool m_valid = false;
	ShaderPair() = default;
	ShaderPair(u32 vert, u32 frag, bool valid = 1) : vertId(vert), fragId(frag), m_valid(valid) {}
	ShaderPair(ShaderId vert, ShaderId frag, bool valid = 1) : vertId(vert.id), fragId(frag.id), m_valid(valid) {}

	bool valid() const { return m_valid; }
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

	template <class Logger, class... Args>
	std::enable_if_t<Log::first_is_log<Logger, Args...>::value, ShaderId>
	addVertexShader(Logger&& logger, Args&&... args) {
		vertexShaders.emplace_back(std::forward<Args>(args)...);
		updateGSVert();
		ShaderId sid{ ShaderType::Vertex, static_cast<u32>(vertexShaders.size() - 1) };
		bool success = compileSucceed(sid);
		logger(success, getCompileLog(sid));
		return success ? sid : ShaderId{ ShaderType::Invalid, 0 };
	}
	template <class... Args>
	std::enable_if_t<!Log::first_is_log<Args...>::value, ShaderId>
	addVertexShader(Args&&... args) {
		return addVertexShader(Log::Compilation{}, std::forward<Args>(args)...);
	}

	template <class Logger, class... Args>
	std::enable_if_t<Log::first_is_log<Logger, Args...>::value, ShaderId>
	addFragmentShader(Logger&& logger, Args&&... args) {
		fragmentShaders.emplace_back(std::forward<Args>(args)...);
		updateGSFrag();
		ShaderId sid{ ShaderType::Fragment, static_cast<u32>(fragmentShaders.size() - 1) };
		bool success = compileSucceed(sid);
		logger(success, getCompileLog(sid));
		return success ? sid : ShaderId{ ShaderType::Invalid, 0 };
	}
	template <class... Args>
	std::enable_if_t<!Log::first_is_log<Args...>::value, ShaderId>
	addFragmentShader(Args&&... args) {
		return addFragmentShader(Log::Compilation{}, std::forward<Args>(args)...);
	}

	template <class Logger = Log::Linking>
	ProgramId linkShaders(ShaderId vertId, ShaderId fragId, Logger logger = Log::Linking{}) {
		linkProgram_impl(vertId.id, fragId.id);
		logger(linkSucceed(ProgramId{ vertId, fragId }), getLinkLog(ProgramId{ vertId, fragId }));
		return ProgramId{ vertId, fragId };
	}

	ShaderProgram& getShaderProgram(ProgramId pid) {
		ShaderProgram& program = programTable[pid.vertId][pid.fragId];
		if (!program.id()) {
			linkProgram_impl(pid.vertId, pid.fragId); // lazy linking
			Log::Linking()(linkSucceed(pid), getLinkLog(pid));
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

// template <>
// class ShaderManager<ShaderManageStyle::Pipeline> {
// public:
// 	class Initializer;
// 	ShaderManageStyle type = ShaderManageStyle::Pipeline;

// 	ShaderManager() {}
// 	template <class InitFunc>
// 	ShaderManager(InitFunc f) {
// 		static_assert(
// 		    std::is_invocable_v<InitFunc, Initializer&>,
// 		    "tx::RE::ShaderManager<ShaderManageStyle::Pipeline>::ShaderManager(): The provided function need to have an parameter of (auto) or (tx::RE::ShaderManager<ShaderManageStyle::Pipeline>::Initializer)");
// 		Initializer initer;
// 		f(initer);
// 		init_impl(std::move(initer));
// 	}
// 	ShaderManager(Initializer&& initer) {
// 		init_impl(std::move(initer));
// 	}
// 	~ShaderManager() {
// 		clearPipelines_impl();
// 	}

// 	// Disable Copying
// 	ShaderManager(const ShaderManager&) = delete;
// 	ShaderManager& operator=(const ShaderManager&) = delete;
// 	ShaderManager(ShaderManager&& other) noexcept
// 	    : vertexPrograms(std::move(other.vertexPrograms)),
// 	      fragmentPrograms(std::move(other.fragmentPrograms)),
// 	      pipelineTable(std::move(other.pipelineTable)) {}
// 	ShaderManager& operator=(ShaderManager&& other) noexcept {
// 		if (this != &other) {
// 			clearPipelines_impl();

// 			vertexPrograms = std::move(other.vertexPrograms);
// 			fragmentPrograms = std::move(other.fragmentPrograms);
// 			pipelineTable = std::move(other.pipelineTable);
// 		}
// 		return *this;
// 	}

// private:
// 	template <ShaderType ST>
// 	class Program;

// public:
// 	class Initializer {
// 		friend ShaderManager<ShaderManageStyle::Pipeline>;

// 	public:
// 		Initializer() {}

// 		// Disable Copy
// 		Initializer(const Initializer&) = delete;
// 		Initializer& operator=(const Initializer&) = delete;
// 		Initializer(Initializer&& other) noexcept
// 		    : vertexPrograms(std::move(other.vertexPrograms)),
// 		      fragmentPrograms(std::move(other.fragmentPrograms)),
// 		      pipelineTable(std::move(other.pipelineTable)) {}
// 		Initializer& operator=(Initializer&& other) noexcept {
// 			if (this != &other) {
// 				vertexPrograms = std::move(other.vertexPrograms);
// 				fragmentPrograms = std::move(other.fragmentPrograms);
// 				pipelineTable = std::move(other.pipelineTable);
// 			}
// 			return *this;
// 		}

// 		~Initializer() = default;


// 		template <class Logger, class... Args>
// 		std::enable_if_t<Log::first_is_log<Logger, Args...>::value, ShaderId>
// 		addVertexShader(Logger&& logger, Args&&... args) {
// 			vertexPrograms.emplace_back(VertexShader{ std::forward<Args>(args)... });
// 			updateGSVert();
// 			ShaderId sid = { ShaderType::Vertex, static_cast<u32>(vertexPrograms.size() - 1) };
// 			bool success = compileSucceed(sid);
// 			logger(success, getCompileLog(sid));
// 			return success ? sid : ShaderId{ ShaderType::Invalid, 0 };
// 		}
// 		template <class... Args>
// 		std::enable_if_t<!Log::first_is_log<Args...>::value, ShaderId>
// 		addVertexShader(Args&&... args) {
// 			return addVertexShader(Log::Compilation{}, std::forward<Args>(args)...);
// 		}

// 		template <class Logger, class... Args>
// 		std::enable_if_t<Log::first_is_log<Logger, Args...>::value, ShaderId>
// 		addFragmentShader(Logger&& logger, Args&&... args) {
// 			fragmentPrograms.emplace_back(FragmentShader{ std::forward<Args>(args)... });
// 			updateGSFrag();
// 			ShaderId sid = { ShaderType::Fragment, static_cast<u32>(fragmentPrograms.size() - 1) };
// 			bool success = compileSucceed(sid);
// 			logger(success, getCompileLog(sid));
// 			return success ? sid : ShaderId{ ShaderType::Invalid, 0 };
// 		}
// 		template <class... Args>
// 		std::enable_if_t<!Log::first_is_log<Args...>::value, ShaderId>
// 		addFragmentShader(Args&&... args) {
// 			return addFragmentShader(Log::Compilation{}, std::forward<Args>(args)...);
// 		}

// 		template <class Logger = Log::Linking>
// 		void linkShaders(PipelineId pid, Logger logger = Log::Linking{}) {
// 			getId_impl(pid) = makePipeline_impl(
// 			    vertexPrograms[pid.vertId].id(), fragmentPrograms[pid.fragId].id());
// 			logger(linkSucceed(pid), getLinkLog(pid));
// 		}





// 		// debug

// 		bool linkSucceed(PipelineId pid) {
// 			if (!vertexPrograms[pid.vertId].valid() || !fragmentPrograms[pid.fragId].valid()) return false;
// 			int success;
// 			gid id = getId(pid);
// 			glValidateProgramPipeline(id);
// 			glGetProgramPipelineiv(id, GL_VALIDATE_STATUS, &success);
// 			return success;
// 		}
// 		std::string getLinkLog(PipelineId pid) {
// 			std::string str = vertexPrograms[pid.vertId].getLog();
// 			str.append(fragmentPrograms[pid.fragId].getLog());
// 			str.append(Initializer::getPipelineLog(getId(pid)));
// 			return str;
// 		}
// 		bool compileSucceed(ShaderId sid) {
// 			bool valid;
// 			visit_impl(sid, [&](const auto& program) {
// 				valid = program.shaderValid();
// 			});
// 			return valid;
// 		}
// 		std::string getCompileLog(ShaderId sid) {
// 			std::string str;
// 			visit_impl(sid, [&](const auto& program) {
// 				str = program.getShaderLog();
// 			});
// 			return str;
// 		}

// 		gid getId(PipelineId pid) const { return pipelineTable[pid.fragId][pid.vertId]; }

// 	private:
// 		std::vector<Program<ShaderType::Vertex>> vertexPrograms;
// 		std::vector<Program<ShaderType::Fragment>> fragmentPrograms;
// 		std::vector<std::vector<gid>> pipelineTable; // [fragId][vertId]

// 		gid& getId_impl(PipelineId pid) { return pipelineTable[pid.fragId][pid.vertId]; }

// 		// GridSystem
// 		void updateGSFrag() {
// 			pipelineTable.push_back(std::vector<gid>(vertexPrograms.size(), 0));
// 		}
// 		void updateGSVert() {
// 			for (std::vector<gid>& i : pipelineTable) {
// 				i.push_back(0);
// 			}
// 		}

// 		// OpenGL
// 		static gid makePipeline_impl(gid vertId, gid fragId) {
// 			gid id;
// 			glCreateProgramPipelines(1, &id);
// 			glUseProgramStages(id, GL_VERTEX_SHADER_BIT, vertId);
// 			glUseProgramStages(id, GL_FRAGMENT_SHADER_BIT, fragId);
// 			return id;
// 		}
// 		static bool validPipeline_impl(gid id) {
// 			int success;
// 			glValidateProgramPipeline(id);
// 			glGetProgramPipelineiv(id, GL_VALIDATE_STATUS, &success);
// 			return success;
// 		}
// 		static std::string getPipelineLog(gid id) {
// 			int length = 0;
// 			glGetProgramPipelineiv(id, GL_INFO_LOG_LENGTH, &length);
// 			if (length <= 1) return "";

// 			std::string str(length - 1, '\0');
// 			glGetProgramPipelineInfoLog(id, length, nullptr, str.data());
// 			return str;
// 		}

// 		template <class Func>
// 		void visit_impl(ShaderId sid, Func f) {
// 			if (sid.type == ShaderType::Vertex)
// 				f(vertexPrograms[sid.id]);
// 			else if (sid.type == ShaderType::Fragment)
// 				f(fragmentPrograms[sid.id]);
// 		}
// 	};

// 	bool linkSucceed(PipelineId pid) {
// 		return (vertexPrograms[pid.vertId].valid() && fragmentPrograms[pid.fragId].valid() && Initializer::validPipeline_impl(getId_impl(pid)));
// 	}
// 	std::string getLinkLog(PipelineId pid) {
// 		std::string str = vertexPrograms[pid.vertId].getLog();
// 		str.append(fragmentPrograms[pid.fragId].getLog());
// 		str.append(Initializer::getPipelineLog(getId_impl(pid)));
// 		return str;
// 	}
// 	bool compileSucceed(ShaderId sid) {
// 		bool valid;
// 		visit_impl(sid, [&](const auto& program) {
// 			valid = program.shaderValid();
// 		});
// 		return valid;
// 	}
// 	std::string getCompileLog(ShaderId sid) {
// 		std::string str;
// 		visit_impl(sid, [&](const auto& program) {
// 			str = program.getShaderLog();
// 		});
// 		return str;
// 	}

// 	gid getPipelineId(PipelineId pid) {
// 		gid& id = getId_impl(pid);
// 		if (id) return id;
// 		// not linked yet
// 		id = Initializer::makePipeline_impl(
// 		    vertexPrograms[pid.vertId].id(), fragmentPrograms[pid.fragId].id());
// 		return id;
// 	}

// 	// things to add:
// 	// 1. u32 id wrapper: contain type

// private:
// 	template <ShaderType ST>
// 	class Program {
// 	public:
// 		Program(Shader<ST>&& shader) : m_shader(std::move(shader)) {
// 			init_impl(m_shader.id());
// 		}
// 		~Program() {
// 			glDeleteProgram(m_id);
// 		}

// 		Program(const Program<ST>&) = delete;
// 		Program& operator=(const Program<ST>&) = delete;
// 		Program(Program<ST>&& other)
// 		    : m_id(other.m_id),
// 		      m_valid(other.m_valid),
// 		      m_shader(std::move(other.m_shader)) {
// 			other.m_id = 0;
// 			other.m_valid = 0;
// 		}
// 		Program& operator=(Program<ST>&& other) {
// 			if (&other != this) {
// 				if (m_id) glDeleteProgram(m_id);
// 				m_id = other.m_id;
// 				m_valid = other.m_valid;
// 				m_shader = std::move(other.m_shader);
// 				other.m_id = 0;
// 				other.m_valid = 0;
// 			}
// 			return *this;
// 		}

// 		gid id() const { return m_id; }
// 		bool valid() const { return m_valid; }
// 		bool shaderValid() const { return m_shader.valid(); }
// 		std::string getLog() const {
// 			int length = 0;
// 			glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
// 			if (length <= 1) return "";

// 			std::string str(length - 1, '\0');
// 			glGetProgramInfoLog(m_id, length, nullptr, str.data());
// 			return str;
// 		}
// 		std::string getShaderLog() const { return m_shader.getLog(); }

// 	private:
// 		Shader<ST> m_shader;
// 		gid m_id = 0;
// 		int m_valid = 0;

// 		void init_impl(gid shaderId) {
// 			m_id = glCreateProgram();
// 			glProgramParameteri(m_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
// 			glAttachShader(m_id, shaderId);
// 			glLinkProgram(m_id);

// 			// check valid
// 			glGetProgramiv(m_id, GL_LINK_STATUS, &m_valid);
// 		}
// 	};

// 	std::vector<Program<ShaderType::Vertex>> vertexPrograms;
// 	std::vector<Program<ShaderType::Fragment>> fragmentPrograms;
// 	std::vector<gid> pipelineTable;

// 	// GridSystem
// 	inline u32 index_impl(PipelineId pid) const { return pid.fragId * vertexPrograms.size() + pid.vertId; }
// 	gid& getId_impl(PipelineId pid) { return pipelineTable[index_impl(pid)]; }


// 	void init_impl(Initializer&& initer) {
// 		// init with initer
// 		vertexPrograms = std::move(initer.vertexPrograms);
// 		fragmentPrograms = std::move(initer.fragmentPrograms);
// 		pipelineTable.resize(vertexPrograms.size() * fragmentPrograms.size());
// 		for (int i = 0; i < initer.pipelineTable.size(); i++) {
// 			std::copy(initer.pipelineTable[i].begin(), initer.pipelineTable[i].end(), pipelineTable.begin() + i * vertexPrograms.size());
// 		}
// 	}
// 	void clearPipelines_impl() {
// 		for (gid i : pipelineTable) {
// 			if (i) glDeleteProgramPipelines(1, &i);
// 		}
// 		pipelineTable.clear();
// 	}
// 	template <class Func>
// 	void visit_impl(ShaderId sid, Func f) {
// 		if (sid.type == ShaderType::Vertex)
// 			f(vertexPrograms[sid.id]);
// 		else if (sid.type == ShaderType::Fragment)
// 			f(fragmentPrograms[sid.id]);
// 	}
// };


} // namespace RenderEngine
} // namespace tx
