#pragma once

#include <glm/vec2.hpp>

#include <string>
#include <vector>
#include <experimental/filesystem>

#include "opengl/Buffer.h"

class Font {
public:
	Font() = default;
	Font(const std::experimental::filesystem::path& name, int height);
	Font(const Font&) = delete;
	Font& operator=(const Font&) = delete;
	Font(Font&& other) noexcept;
	Font& operator=(Font&& other) noexcept;
	~Font();

	friend void renderText(gl::Buffer& buffer, int nX, int nY, const Font& font, const std::string& text, float sx = 1, float sy = 1);

private:
	struct Glyph {
		glm::vec2 advance;
		glm::uvec2 size;
		glm::ivec2 bearing;
		float texX;
	};

	void swap(Font& other);

	std::vector<Glyph> m_glyphs;
	glm::vec2 m_atlasSize;
	GLuint m_id = 0;
};
