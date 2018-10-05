#include "Texture.h"

#include <algorithm>

namespace gl {
	Texture::Texture() {
		glGenTextures(1, &m_id);
	}

	Texture::Texture(Texture&& other) {
		swap(other);
	}

	Texture& Texture::operator=(Texture&& other) {
		swap(other);
		return *this;
	}

	Texture::~Texture() {
		glDeleteTextures(1, &m_id);
	}

	auto Texture::id() const -> GLuint {
		return m_id;
	}

	void Texture::bind(GLenum target) {
		glBindTexture(target, m_id);
	}

	void Texture::swap(Texture& other) {
		using std::swap;
		swap(m_id, other.m_id);
	}
}