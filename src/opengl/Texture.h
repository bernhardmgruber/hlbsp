#pragma once

#include <GL/glew.h>

namespace gl {
	class Texture {
	public:
		Texture();
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&& other);
		Texture& operator=(Texture&& other);
		~Texture();

		auto id() const -> GLuint;
		void bind(GLenum target);

	private:
		void swap(Texture& other);

		GLuint m_id = 0;
	};
}