#include "Shader.h"

#include <iostream>

#include "../IO.h"

namespace gl {
	Shader::Shader(GLenum shaderType, const std::string& source) {
		m_id = glCreateShader(shaderType);
		const char* p = source.c_str();
		glShaderSource(m_id, 1, &p, nullptr);
		glCompileShader(m_id);

		GLint status;
		glGetShaderiv(m_id, GL_COMPILE_STATUS, &status);
		GLint length;
		glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &length);
		std::string buildLog;
		buildLog.resize(length);
		glGetShaderInfoLog(m_id, length, nullptr, buildLog.data());

		if (status != GL_TRUE)
			throw std::runtime_error("Failed to compile shader:\n" + buildLog);
		else
			std::clog << "Shader compile log:\n"
					  << buildLog << "\n";
	}

	Shader::Shader(GLenum shaderType, const std::experimental::filesystem::path& file)
		: Shader(shaderType, readTextFile(file)) {}

	Shader::Shader(Shader&& other) {
		swap(other);
	}

	Shader& Shader::operator=(Shader&& other) {
		swap(other);
		return *this;
	}

	Shader::~Shader() {
		if (m_id != 0)
			glDeleteShader(m_id);
	}

	auto Shader::id() const -> GLuint {
		return m_id;
	}

	void Shader::swap(Shader& other) {
		using std::swap;
		swap(m_id, other.m_id);
	}
}