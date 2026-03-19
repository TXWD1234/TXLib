// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: shader_manager.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"
#include "impl/log.hpp"

namespace tx {
namespace RenderEngine {
enum class ShaderType : u32 {
	Invalid = 0,
	Vertex = GL_VERTEX_SHADER,
	Fragment = GL_FRAGMENT_SHADER,
	Geometry = GL_GEOMETRY_SHADER,
	Compute = GL_COMPUTE_SHADER
};

struct ShaderId {
	ShaderType type;
	u32 id;
	bool valid() const { return type != ShaderType::Invalid; }
};
// Program / Pipeline id
struct ShaderPair {
	ShaderPair() = default;
	ShaderPair(u32 vert, u32 frag) : vertId(vert), fragId(frag) {}
	ShaderPair(ShaderId vert, ShaderId frag)
	    : vertId(vert.id),
	      fragId(frag.id) {}

	u32 vertId, fragId;
};
using ProgramId = ShaderPair;
using PipelineId = ShaderPair;

template <ShaderType type>
class Shader {
public:
	Shader(const std::string& str) {
		const char* strptr = str.c_str();
		init_impl(1, &strptr);
	}
	Shader(const std::vector<std::string>& source) {
		std::vector<const char*> sourceArray(source.size());
		for (size_t i = 0; i < source.size(); i++) {
			sourceArray[i] = source[i].c_str();
		}
		init_impl(static_cast<u32>(source.size()), sourceArray.data());
	}
	Shader(u32 sourceCount, const char* const source[]) {
		init_impl(sourceCount, source);
	}

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader(Shader&& other) {
		m_id = other.m_id;
		m_valid = other.m_valid;
		other.m_id = 0;
		other.m_valid = 0;
	}
	Shader& operator=(Shader&& other) {
		if (&other != this) {
			if (m_id) glDeleteShader(m_id);
			m_id = other.m_id;
			m_valid = other.m_valid;
			other.m_id = 0;
			other.m_valid = 0;
		}
		return *this;
	}

	~Shader() {
		if (m_id) glDeleteShader(m_id);
	}

	gid id() const { return m_id; }
	bool valid() const { return m_valid; }
	std::string getLog() const {
		int length = 0;
		glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);
		if (length <= 1) return "";

		std::string str(length - 1, '\0');
		glGetShaderInfoLog(m_id, length, nullptr, str.data());
		return str;
	}


private:
	gid m_id = 0;
	int m_valid = 0;

	void init_impl(u32 sourceCount, const char* const source[]) {
		m_id = glCreateShader(enumval(type));
		glShaderSource(m_id, sourceCount, source, nullptr);
		glCompileShader(m_id);

		// check valid
		glGetShaderiv(m_id, GL_COMPILE_STATUS, &m_valid);
	}
};
using VertexShader = Shader<ShaderType::Vertex>;
using FragmentShader = Shader<ShaderType::Fragment>;
using GeometryShader = Shader<ShaderType::Geometry>;
using ComputeShader = Shader<ShaderType::Compute>;

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
	class Initializer;
	ShaderManageStyle type = ShaderManageStyle::Program;

	ShaderManager() {}
	template <class InitFunc>
	ShaderManager(InitFunc f) {
		static_assert(
		    std::is_invocable_v<InitFunc, Initializer&>,
		    "tx::RE::ShaderManager<ShaderManageStyle::Program>::ShaderManager(): The provided function need to have an parameter of (auto) or (tx::RE::ShaderManager<ShaderManageStyle::Program>::Initializer)");
		Initializer initer;
		f(initer);
		init_impl(std::move(initer));
	}
	ShaderManager(Initializer&& initer) {
		init_impl(std::move(initer));
	}
	~ShaderManager() {
		clearPrograms_impl();
	}

	// Disable Copy
	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;

	// Move Constructor
	ShaderManager(ShaderManager&& other) noexcept
	    : vertexShaders(std::move(other.vertexShaders)),
	      fragmentShaders(std::move(other.fragmentShaders)),
	      programTable(std::move(other.programTable)) {}
	ShaderManager& operator=(ShaderManager&& other) noexcept {
		if (this != &other) {
			clearPrograms_impl(); // Clean up current programs before taking new ones
			vertexShaders = std::move(other.vertexShaders);
			fragmentShaders = std::move(other.fragmentShaders);
			programTable = std::move(other.programTable);
		}
		return *this;
	}


	// provide methods for the initialing function
	class Initializer {
		friend ShaderManager<ShaderManageStyle::Program>;

	public:
		Initializer() {}

		// Disable Copy
		Initializer(const Initializer&) = delete;
		Initializer& operator=(const Initializer&) = delete;
		Initializer(Initializer&& other) noexcept
		    : vertexShaders(std::move(other.vertexShaders)),
		      fragmentShaders(std::move(other.fragmentShaders)),
		      programTable(std::move(other.programTable)) {}
		Initializer& operator=(Initializer&& other) noexcept {
			if (this != &other) {
				vertexShaders = std::move(other.vertexShaders);
				fragmentShaders = std::move(other.fragmentShaders);
				programTable = std::move(other.programTable);
			}
			return *this;
		}

		// Destructor: Initializer doesn't delete programs because
		// it's meant to pass them to the Manager.
		~Initializer() = default;


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
		void linkShaders(ProgramId pid, Logger logger = Log::Linking{}) {
			getId_impl(pid) = makeProgram_impl(
			    vertexShaders[pid.vertId].id(), fragmentShaders[pid.fragId].id());
			logger(linkSucceed(pid), getLinkLog(pid));
		}

		// debug

		bool compileSucceed(ShaderId sid) {
			int success;
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
			int success;
			gid id = getId(pid);
			glGetProgramiv(id, GL_LINK_STATUS, &success);
			return success;
		}
		std::string getLinkLog(ProgramId pid) {
			gid id = getId(pid);

			int length = 0;
			glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
			if (length <= 1) return "";

			std::string str(length - 1, '\0');
			glGetProgramInfoLog(id, length, nullptr, str.data());
			return str;
		}

		gid getId(ProgramId pid) const { return programTable[pid.fragId][pid.vertId]; }

	private:
		std::vector<VertexShader> vertexShaders;
		std::vector<FragmentShader> fragmentShaders;
		std::vector<std::vector<gid>> programTable; // [fragId][vertId]

		gid& getId_impl(ProgramId pid) { return programTable[pid.fragId][pid.vertId]; }

		// GridSystem
		void updateGSFrag() {
			programTable.push_back(std::vector<gid>(vertexShaders.size(), 0));
		}
		void updateGSVert() {
			for (std::vector<gid>& i : programTable) {
				i.push_back(0);
			}
		}

		// OpenGL
		static gid makeProgram_impl(gid vertId, gid fragId) {
			gid id = glCreateProgram();
			glAttachShader(id, vertId);
			glAttachShader(id, fragId);
			glLinkProgram(id);
			return id;
		}

		template <class Func>
		void visit_impl(ShaderId sid, Func f) {
			if (sid.type == ShaderType::Vertex)
				f(vertexShaders[sid.id]);
			else if (sid.type == ShaderType::Fragment)
				f(fragmentShaders[sid.id]);
		}
	};

	// debug

	bool compileSucceed(ShaderId sid) {
		int success;
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
		int success;
		gid id = getId_impl(pid);
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		return success;
	}
	std::string getLinkLog(ProgramId pid) {
		gid id = getId_impl(pid);

		int length = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
		if (length <= 1) return "";

		std::string str(length - 1, '\0');
		glGetProgramInfoLog(id, length, nullptr, str.data());
		return str;
	}

	gid getProgramId(ProgramId pid) {
		gid& id = getId_impl(pid);
		if (id) return id;
		// not linked yet
		id = Initializer::makeProgram_impl(
		    vertexShaders[pid.vertId].id(), fragmentShaders[pid.fragId].id());
		return id;
	}

private:
	std::vector<VertexShader> vertexShaders;
	std::vector<FragmentShader> fragmentShaders;
	std::vector<gid> programTable; // Grid system: vert = x, frag = y

	void init_impl(Initializer&& initer) {
		// init with initer
		vertexShaders = std::move(initer.vertexShaders);
		fragmentShaders = std::move(initer.fragmentShaders);
		programTable.resize(vertexShaders.size() * fragmentShaders.size());
		for (int i = 0; i < initer.programTable.size(); i++) {
			std::copy(initer.programTable[i].begin(), initer.programTable[i].end(), programTable.begin() + i * vertexShaders.size());
		}
	}

	// GridSystem
	inline u32 index_impl(ProgramId pid) const { return pid.fragId * vertexShaders.size() + pid.vertId; }
	gid& getId_impl(ProgramId pid) { return programTable[index_impl(pid)]; }


	void clearPrograms_impl() {
		for (gid i : programTable) {
			if (i) glDeleteProgram(i);
		}
		programTable.clear();
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
	class Initializer;
	ShaderManageStyle type = ShaderManageStyle::Pipeline;

	ShaderManager() {}
	template <class InitFunc>
	ShaderManager(InitFunc f) {
		static_assert(
		    std::is_invocable_v<InitFunc, Initializer&>,
		    "tx::RE::ShaderManager<ShaderManageStyle::Pipeline>::ShaderManager(): The provided function need to have an parameter of (auto) or (tx::RE::ShaderManager<ShaderManageStyle::Pipeline>::Initializer)");
		Initializer initer;
		f(initer);
		init_impl(std::move(initer));
	}
	ShaderManager(Initializer&& initer) {
		init_impl(std::move(initer));
	}
	~ShaderManager() {
		clearPipelines_impl();
	}

	// Disable Copying
	ShaderManager(const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;
	ShaderManager(ShaderManager&& other) noexcept
	    : vertexPrograms(std::move(other.vertexPrograms)),
	      fragmentPrograms(std::move(other.fragmentPrograms)),
	      pipelineTable(std::move(other.pipelineTable)) {}
	ShaderManager& operator=(ShaderManager&& other) noexcept {
		if (this != &other) {
			clearPipelines_impl();

			vertexPrograms = std::move(other.vertexPrograms);
			fragmentPrograms = std::move(other.fragmentPrograms);
			pipelineTable = std::move(other.pipelineTable);
		}
		return *this;
	}

private:
	template <ShaderType ST>
	class Program;

public:
	class Initializer {
		friend ShaderManager<ShaderManageStyle::Pipeline>;

	public:
		Initializer() {}

		// Disable Copy
		Initializer(const Initializer&) = delete;
		Initializer& operator=(const Initializer&) = delete;
		Initializer(Initializer&& other) noexcept
		    : vertexPrograms(std::move(other.vertexPrograms)),
		      fragmentPrograms(std::move(other.fragmentPrograms)),
		      pipelineTable(std::move(other.pipelineTable)) {}
		Initializer& operator=(Initializer&& other) noexcept {
			if (this != &other) {
				vertexPrograms = std::move(other.vertexPrograms);
				fragmentPrograms = std::move(other.fragmentPrograms);
				pipelineTable = std::move(other.pipelineTable);
			}
			return *this;
		}

		~Initializer() = default;


		template <class Logger, class... Args>
		std::enable_if_t<Log::first_is_log<Logger, Args...>::value, ShaderId>
		addVertexShader(Logger&& logger, Args&&... args) {
			vertexPrograms.emplace_back(VertexShader{ std::forward<Args>(args)... });
			updateGSVert();
			ShaderId sid = { ShaderType::Vertex, static_cast<u32>(vertexPrograms.size() - 1) };
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
			fragmentPrograms.emplace_back(FragmentShader{ std::forward<Args>(args)... });
			updateGSFrag();
			ShaderId sid = { ShaderType::Fragment, static_cast<u32>(fragmentPrograms.size() - 1) };
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
		void linkShaders(PipelineId pid, Logger logger = Log::Linking{}) {
			getId_impl(pid) = makePipeline_impl(
			    vertexPrograms[pid.vertId].id(), fragmentPrograms[pid.fragId].id());
			logger(linkSucceed(pid), getLinkLog(pid));
		}





		// debug

		bool linkSucceed(PipelineId pid) {
			if (!vertexPrograms[pid.vertId].valid() || !fragmentPrograms[pid.fragId].valid()) return false;
			int success;
			gid id = getId(pid);
			glValidateProgramPipeline(id);
			glGetProgramPipelineiv(id, GL_VALIDATE_STATUS, &success);
			return success;
		}
		std::string getLinkLog(PipelineId pid) {
			std::string str = vertexPrograms[pid.vertId].getLog();
			str.append(fragmentPrograms[pid.fragId].getLog());
			str.append(Initializer::getPipelineLog(getId(pid)));
			return str;
		}
		bool compileSucceed(ShaderId sid) {
			bool valid;
			visit_impl(sid, [&](const auto& program) {
				valid = program.shaderValid();
			});
			return valid;
		}
		std::string getCompileLog(ShaderId sid) {
			std::string str;
			visit_impl(sid, [&](const auto& program) {
				str = program.getShaderLog();
			});
			return str;
		}

		gid getId(PipelineId pid) const { return pipelineTable[pid.fragId][pid.vertId]; }

	private:
		std::vector<Program<ShaderType::Vertex>> vertexPrograms;
		std::vector<Program<ShaderType::Fragment>> fragmentPrograms;
		std::vector<std::vector<gid>> pipelineTable; // [fragId][vertId]

		gid& getId_impl(PipelineId pid) { return pipelineTable[pid.fragId][pid.vertId]; }

		// GridSystem
		void updateGSFrag() {
			pipelineTable.push_back(std::vector<gid>(vertexPrograms.size(), 0));
		}
		void updateGSVert() {
			for (std::vector<gid>& i : pipelineTable) {
				i.push_back(0);
			}
		}

		// OpenGL
		static gid makePipeline_impl(gid vertId, gid fragId) {
			gid id;
			glCreateProgramPipelines(1, &id);
			glUseProgramStages(id, GL_VERTEX_SHADER_BIT, vertId);
			glUseProgramStages(id, GL_FRAGMENT_SHADER_BIT, fragId);
			return id;
		}
		static bool validPipeline_impl(gid id) {
			int success;
			glValidateProgramPipeline(id);
			glGetProgramPipelineiv(id, GL_VALIDATE_STATUS, &success);
			return success;
		}
		static std::string getPipelineLog(gid id) {
			int length = 0;
			glGetProgramPipelineiv(id, GL_INFO_LOG_LENGTH, &length);
			if (length <= 1) return "";

			std::string str(length - 1, '\0');
			glGetProgramPipelineInfoLog(id, length, nullptr, str.data());
			return str;
		}

		template <class Func>
		void visit_impl(ShaderId sid, Func f) {
			if (sid.type == ShaderType::Vertex)
				f(vertexPrograms[sid.id]);
			else if (sid.type == ShaderType::Fragment)
				f(fragmentPrograms[sid.id]);
		}
	};

	bool linkSucceed(PipelineId pid) {
		return (vertexPrograms[pid.vertId].valid() && fragmentPrograms[pid.fragId].valid() && Initializer::validPipeline_impl(getId_impl(pid)));
	}
	std::string getLinkLog(PipelineId pid) {
		std::string str = vertexPrograms[pid.vertId].getLog();
		str.append(fragmentPrograms[pid.fragId].getLog());
		str.append(Initializer::getPipelineLog(getId_impl(pid)));
		return str;
	}
	bool compileSucceed(ShaderId sid) {
		bool valid;
		visit_impl(sid, [&](const auto& program) {
			valid = program.shaderValid();
		});
		return valid;
	}
	std::string getCompileLog(ShaderId sid) {
		std::string str;
		visit_impl(sid, [&](const auto& program) {
			str = program.getShaderLog();
		});
		return str;
	}

	gid getPipelineId(PipelineId pid) {
		gid& id = getId_impl(pid);
		if (id) return id;
		// not linked yet
		id = Initializer::makePipeline_impl(
		    vertexPrograms[pid.vertId].id(), fragmentPrograms[pid.fragId].id());
		return id;
	}

	// things to add:
	// 1. u32 id wrapper: contain type

private:
	template <ShaderType ST>
	class Program {
	public:
		Program(Shader<ST>&& shader) : m_shader(std::move(shader)) {
			init_impl(m_shader.id());
		}
		~Program() {
			glDeleteProgram(m_id);
		}

		Program(const Program<ST>&) = delete;
		Program& operator=(const Program<ST>&) = delete;
		Program(Program<ST>&& other)
		    : m_id(other.m_id),
		      m_valid(other.m_valid),
		      m_shader(std::move(other.m_shader)) {
			other.m_id = 0;
			other.m_valid = 0;
		}
		Program& operator=(Program<ST>&& other) {
			if (&other != this) {
				if (m_id) glDeleteProgram(m_id);
				m_id = other.m_id;
				m_valid = other.m_valid;
				m_shader = std::move(other.m_shader);
				other.m_id = 0;
				other.m_valid = 0;
			}
			return *this;
		}

		gid id() const { return m_id; }
		bool valid() const { return m_valid; }
		bool shaderValid() const { return m_shader.valid(); }
		std::string getLog() const {
			int length = 0;
			glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
			if (length <= 1) return "";

			std::string str(length - 1, '\0');
			glGetProgramInfoLog(m_id, length, nullptr, str.data());
			return str;
		}
		std::string getShaderLog() const { return m_shader.getLog(); }

	private:
		Shader<ST> m_shader;
		gid m_id = 0;
		int m_valid = 0;

		void init_impl(gid shaderId) {
			m_id = glCreateProgram();
			glProgramParameteri(m_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
			glAttachShader(m_id, shaderId);
			glLinkProgram(m_id);

			// check valid
			glGetProgramiv(m_id, GL_LINK_STATUS, &m_valid);
		}
	};

	std::vector<Program<ShaderType::Vertex>> vertexPrograms;
	std::vector<Program<ShaderType::Fragment>> fragmentPrograms;
	std::vector<gid> pipelineTable;

	// GridSystem
	inline u32 index_impl(PipelineId pid) const { return pid.fragId * vertexPrograms.size() + pid.vertId; }
	gid& getId_impl(PipelineId pid) { return pipelineTable[index_impl(pid)]; }


	void init_impl(Initializer&& initer) {
		// init with initer
		vertexPrograms = std::move(initer.vertexPrograms);
		fragmentPrograms = std::move(initer.fragmentPrograms);
		pipelineTable.resize(vertexPrograms.size() * fragmentPrograms.size());
		for (int i = 0; i < initer.pipelineTable.size(); i++) {
			std::copy(initer.pipelineTable[i].begin(), initer.pipelineTable[i].end(), pipelineTable.begin() + i * vertexPrograms.size());
		}
	}
	void clearPipelines_impl() {
		for (gid i : pipelineTable) {
			if (i) glDeleteProgramPipelines(1, &i);
		}
		pipelineTable.clear();
	}
	template <class Func>
	void visit_impl(ShaderId sid, Func f) {
		if (sid.type == ShaderType::Vertex)
			f(vertexPrograms[sid.id]);
		else if (sid.type == ShaderType::Fragment)
			f(fragmentPrograms[sid.id]);
	}
};

template <ShaderManageStyle type>
using SMIniter = typename ShaderManager<type>::Initializer;

} // namespace RenderEngine
} // namespace tx
