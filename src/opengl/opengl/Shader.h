#pragma once

#include <GL/glew.h>

#include <filesystem>

namespace fs = std::filesystem;

namespace gl {
	class Shader {
	public:
		Shader() = default;
		Shader(GLenum shaderType, const std::string& source, const std::string& filename = "");
		Shader(GLenum shaderType, const fs::path& file);
		Shader(const Shader&) = delete;
		auto operator=(const Shader&) -> Shader& = delete;
		Shader(Shader&& other);
		auto operator=(Shader&& other) -> Shader&;
		~Shader();

		auto id() const -> GLuint;

	private:
		void swap(Shader& other);

		GLuint m_id = 0;
	};
}