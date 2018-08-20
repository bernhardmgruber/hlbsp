#include "Program.h"

#include <iostream>

#include "../IO.h"

namespace gl {
	Program::Program(std::initializer_list<Shader> shaders) {
		m_id = glCreateProgram();
		for (const auto& shader : shaders)
			glAttachShader(m_id, shader.id());
		glLinkProgram(m_id);

		GLint status;
		glGetProgramiv(m_id, GL_LINK_STATUS, &status);
		GLint length;
		glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);
		std::string buildLog;
		buildLog.resize(length);
		glGetProgramInfoLog(m_id, length, nullptr, buildLog.data());

		if (status != GL_TRUE)
			throw std::runtime_error("Failed to link program:\n" + buildLog);
		else
			std::clog << "Program link log:\n"
					  << buildLog << "\n";
	}

	Program::Program(Program&& other) {
		swap(other);
	}

	Program& Program::operator=(Program&& other) {
		swap(other);
		return *this;
	}

	Program::~Program() {
		if (m_id != 0)
			glDeleteProgram(m_id);
	}

	auto Program::id() const -> GLuint {
		return m_id;
	}

	void Program::use() const {
		glUseProgram(m_id);
	}

	auto Program::uniformLocation(std::string_view name) const -> GLint {
		const auto loc = glGetUniformLocation(m_id, name.data());
		if (loc == -1)
			throw std::runtime_error(std::string("could not find location of uniform ") + name.data());
		return loc;
	}

	auto Program::attributeLocation(std::string_view name) const -> GLint {
		const auto loc = glGetAttribLocation(m_id, name.data());
		if (loc == -1)
			throw std::runtime_error(std::string("could not find location of attribute ") + name.data());
		return loc;
	}

	void Program::swap(Program& other) {
		using std::swap;
		swap(m_id, other.m_id);
	}
}