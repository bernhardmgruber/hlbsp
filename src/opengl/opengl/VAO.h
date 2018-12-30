#pragma once

#include <GL/glew.h>

namespace gl {
	class VAO {
	public:
		VAO();
		VAO(const VAO&) = delete;
		auto operator=(const VAO&) -> VAO& = delete;
		VAO(VAO&& other);
		auto operator=(VAO&& other) -> VAO&;
		~VAO();

		auto id() const -> GLuint;
		void bind();

	private:
		void swap(VAO& other);

		GLuint m_id = 0;
	};
}