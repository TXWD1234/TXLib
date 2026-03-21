// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: shader_program.hpp

#pragma once
#include "impl/basic_gl_utils.hpp"
#include "impl/log.hpp"
#include <string>
#include <vector>

namespace tx {
namespace RenderEngine {

enum class ShaderType : u32 {
	Invalid = 0,
	Vertex = GL_VERTEX_SHADER,
	Fragment = GL_FRAGMENT_SHADER,
	Geometry = GL_GEOMETRY_SHADER,
	Compute = GL_COMPUTE_SHADER
};


template <ShaderType type>
class Shader {
public:
	Shader() = default;
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

	~Shader() {
		if (m_id) glDeleteShader(m_id);
	}

	// Disable Copy
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	// Move Semantics
	Shader(Shader&& other) noexcept : m_id(other.m_id), m_valid(other.m_valid) {
		other.m_id = 0;
		other.m_valid = 0;
	}
	Shader& operator=(Shader&& other) noexcept {
		if (&other != this) {
			if (m_id) glDeleteShader(m_id);
			m_id = other.m_id;
			m_valid = other.m_valid;
			other.m_id = 0;
			other.m_valid = 0;
		}
		return *this;
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

class ShaderProgram {
public:
	ShaderProgram() = default;
	ShaderProgram(const VertexShader& vert, const FragmentShader& frag) {
		m_id = glCreateProgram();
		glAttachShader(m_id, vert.id());
		glAttachShader(m_id, frag.id());
		glLinkProgram(m_id);
		glGetProgramiv(m_id, GL_LINK_STATUS, &m_valid);
	}
	~ShaderProgram() {
		if (m_id) glDeleteProgram(m_id);
	}

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram(ShaderProgram&& other) noexcept : m_id(other.m_id), m_valid(other.m_valid) {
		other.m_id = 0;
		other.m_valid = 0;
	}
	ShaderProgram& operator=(ShaderProgram&& other) noexcept {
		if (this != &other) {
			if (m_id) glDeleteProgram(m_id);
			m_id = other.m_id;
			m_valid = other.m_valid;
			other.m_id = 0;
			other.m_valid = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_valid; }
	std::string getLog() const {
		int length = 0;
		glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
		if (length <= 1) return "";
		std::string str(length - 1, '\0');
		glGetProgramInfoLog(m_id, length, nullptr, str.data());
		return str;
	}

private:
	gid m_id = 0;
	int m_valid = 0;
};

template <ShaderType ST>
class ShaderProgramComponent {
public:
	ShaderProgramComponent() = default;
	ShaderProgramComponent(Shader<ST>&& shader) : m_shader(std::move(shader)) {
		m_id = glCreateProgram();
		glProgramParameteri(m_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glAttachShader(m_id, m_shader.id());
		glLinkProgram(m_id);
		// check valid
		glGetProgramiv(m_id, GL_LINK_STATUS, &m_valid);
	}
	~ShaderProgramComponent() {
		if (m_id) glDeleteProgram(m_id);
	}

	ShaderProgramComponent(const ShaderProgramComponent&) = delete;
	ShaderProgramComponent& operator=(const ShaderProgramComponent&) = delete;
	ShaderProgramComponent(ShaderProgramComponent&& other) noexcept : m_id(other.m_id), m_valid(other.m_valid) {
		other.m_id = 0;
		other.m_valid = 0;
	}
	ShaderProgramComponent& operator=(ShaderProgramComponent&& other) noexcept {
		if (this != &other) {
			if (m_id) glDeleteProgram(m_id);
			m_id = other.m_id;
			m_valid = other.m_valid;
			other.m_id = 0;
			other.m_valid = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_valid; }

	bool linkSucceed() const {
		return m_valid;
	}
	bool compileSucceed() const {
		return m_shader.valid();
	}
	std::string getLinkLog() const {
		int length = 0;
		glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
		if (length <= 1) return "";
		std::string str(length - 1, '\0');
		glGetProgramInfoLog(m_id, length, nullptr, str.data());
		return str;
	}
	std::string getCompileLog() const {
		return m_shader.getLog();
	}

private:
	Shader<ST> m_shader;
	gid m_id = 0;
	int m_valid = 0;
};

class ShaderPipeline {
public:
	ShaderPipeline() = default;
	ShaderPipeline(const ShaderProgramComponent<ShaderType::Vertex>& vert, const ShaderProgramComponent<ShaderType::Fragment>& frag) {
		glCreateProgramPipelines(1, &m_id);
		glUseProgramStages(m_id, GL_VERTEX_SHADER_BIT, vert.id());
		glUseProgramStages(m_id, GL_FRAGMENT_SHADER_BIT, frag.id());
		// check valid
		glValidateProgramPipeline(m_id);
		glGetProgramPipelineiv(m_id, GL_VALIDATE_STATUS, &m_valid);
	}
	~ShaderPipeline() {
		if (m_id) glDeleteProgramPipelines(1, &m_id);
	}

	ShaderPipeline(const ShaderPipeline&) = delete;
	ShaderPipeline& operator=(const ShaderPipeline&) = delete;
	ShaderPipeline(ShaderPipeline&& other) noexcept : m_id(other.m_id), m_valid(other.m_valid) {
		other.m_id = 0;
		other.m_valid = 0;
	}
	ShaderPipeline& operator=(ShaderPipeline&& other) noexcept {
		if (this != &other) {
			if (m_id) glDeleteProgramPipelines(1, &m_id);
			m_id = other.m_id;
			m_valid = other.m_valid;
			other.m_id = 0;
			other.m_valid = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_valid; }

	std::string getLog() const {
		int length = 0;
		glGetProgramPipelineiv(m_id, GL_INFO_LOG_LENGTH, &length);
		if (length <= 1) return "";
		std::string str(length - 1, '\0');
		glGetProgramPipelineInfoLog(m_id, length, nullptr, str.data());
		return str;
	}

private:
	gid m_id = 0;
	int m_valid = 0;
};

} // namespace RenderEngine
} // namespace tx
