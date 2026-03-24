// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: shader_program.hpp

#pragma once
#include "impl/render_engine/basic_gl_utils.hpp"
#include "impl/render_engine/log.hpp"
#include <string>
#include <vector>

namespace tx {
namespace RenderEngine {

enum class ShaderType : u32 {
	Vertex = gl::VERTEX_SHADER,
	Fragment = gl::FRAGMENT_SHADER,
	Geometry = gl::GEOMETRY_SHADER,
	Compute = gl::COMPUTE_SHADER
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
		if (m_id) gl::deleteShader(m_id);
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
			if (m_id) gl::deleteShader(m_id);
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
		gl::getShaderiv(m_id, gl::INFO_LOG_LENGTH, &length);
		if (length <= 1) return "";

		std::string str(length - 1, '\0');
		gl::getShaderInfoLog(m_id, length, nullptr, str.data());
		return str;
	}

private:
	gid m_id = 0;
	int m_valid = 0;

	void init_impl(u32 sourceCount, const char* const source[]) {
		m_id = gl::createShader(enumval(type));
		gl::shaderSource(m_id, sourceCount, source, nullptr);
		gl::compileShader(m_id);
		// check valid
		gl::getShaderiv(m_id, gl::COMPILE_STATUS, &m_valid);
	}
};

using VertexShader = Shader<ShaderType::Vertex>;
using FragmentShader = Shader<ShaderType::Fragment>;
using GeometryShader = Shader<ShaderType::Geometry>;
using ComputeShader = Shader<ShaderType::Compute>;

class ShaderProgram {
public:
	ShaderProgram() = default;
	ShaderProgram(const VertexShader& vert, const FragmentShader& frag);
	~ShaderProgram() {
		if (m_id) gl::deleteProgram(m_id);
	}

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;
	ShaderProgram(ShaderProgram&& other) noexcept : m_id(other.m_id), m_valid(other.m_valid) {
		other.m_id = 0;
		other.m_valid = 0;
	}
	ShaderProgram& operator=(ShaderProgram&& other) noexcept {
		if (this != &other) {
			if (m_id) gl::deleteProgram(m_id);
			m_id = other.m_id;
			m_valid = other.m_valid;
			other.m_id = 0;
			other.m_valid = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_valid; }
	std::string getLog() const;

private:
	gid m_id = 0;
	int m_valid = 0;
};

template <ShaderType ST>
class ShaderProgramComponent {
public:
	ShaderProgramComponent() = default;
	ShaderProgramComponent(Shader<ST>&& shader) : m_shader(std::move(shader)) {
		m_id = gl::createProgram();
		gl::programParameteri(m_id, gl::PROGRAM_SEPARABLE, gl::TRUE);
		gl::attachShader(m_id, m_shader.id());
		gl::linkProgram(m_id);
		// check valid
		gl::getProgramiv(m_id, gl::LINK_STATUS, &m_valid);
	}
	~ShaderProgramComponent() {
		if (m_id) gl::deleteProgram(m_id);
	}

	ShaderProgramComponent(const ShaderProgramComponent&) = delete;
	ShaderProgramComponent& operator=(const ShaderProgramComponent&) = delete;
	ShaderProgramComponent(ShaderProgramComponent&& other) noexcept : m_id(other.m_id), m_valid(other.m_valid) {
		other.m_id = 0;
		other.m_valid = 0;
	}
	ShaderProgramComponent& operator=(ShaderProgramComponent&& other) noexcept {
		if (this != &other) {
			if (m_id) gl::deleteProgram(m_id);
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
		gl::getProgramiv(m_id, gl::INFO_LOG_LENGTH, &length);
		if (length <= 1) return "";
		std::string str(length - 1, '\0');
		gl::getProgramInfoLog(m_id, length, nullptr, str.data());
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
	ShaderPipeline(const ShaderProgramComponent<ShaderType::Vertex>& vert, const ShaderProgramComponent<ShaderType::Fragment>& frag);
	~ShaderPipeline() {
		if (m_id) gl::deleteProgramPipelines(1, &m_id);
	}

	ShaderPipeline(const ShaderPipeline&) = delete;
	ShaderPipeline& operator=(const ShaderPipeline&) = delete;
	ShaderPipeline(ShaderPipeline&& other) noexcept : m_id(other.m_id), m_valid(other.m_valid) {
		other.m_id = 0;
		other.m_valid = 0;
	}
	ShaderPipeline& operator=(ShaderPipeline&& other) noexcept {
		if (this != &other) {
			if (m_id) gl::deleteProgramPipelines(1, &m_id);
			m_id = other.m_id;
			m_valid = other.m_valid;
			other.m_id = 0;
			other.m_valid = 0;
		}
		return *this;
	}

	gid id() const { return m_id; }
	bool valid() const { return m_valid; }

	std::string getLog() const;

private:
	gid m_id = 0;
	int m_valid = 0;
};

} // namespace RenderEngine
} // namespace tx
