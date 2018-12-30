#pragma once

#include <GL/glew.h>

namespace gl {
	class Texture {
	public:
		Texture();
		Texture(const Texture&) = delete;
		auto operator=(const Texture&) -> Texture& = delete;
		Texture(Texture&& other);
		auto operator=(Texture&& other) -> Texture&;
		~Texture();

		auto id() const -> GLuint;
		void bind(GLenum target);

	private:
		void swap(Texture& other);

		GLuint m_id = 0;
	};
}