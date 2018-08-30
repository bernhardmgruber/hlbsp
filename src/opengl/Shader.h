#pragma once

#include <gl/glew.h>

#include <experimental/filesystem>

namespace gl {
	class Shader {
	public:
		Shader() = default;
		Shader(GLenum shaderType, const std::string& source);
		Shader(GLenum shaderType, const std::experimental::filesystem::path& file);
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;
		Shader(Shader&& other);
		Shader& operator=(Shader&& other);
		~Shader();

		auto id() const -> GLuint;

	private:
		void swap(Shader& other);

		GLuint m_id = 0;
	};
}