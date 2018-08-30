#include "font.h"

#include <algorithm>
#include <iostream>

#include <ft2build.h>
#include FT_FREETYPE_H

// cf. https://learnopengl.com/In-Practice/Text-Rendering and https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Text_Rendering_02

Font::Font(const std::experimental::filesystem::path& path, int height) {
	FT_Library ft;
	if (auto error = FT_Init_FreeType(&ft))
		throw std::runtime_error("Faile to init FreeType library: " + std::to_string(error));

	auto fn = std::experimental::filesystem::absolute(path).string();
	for (char& c : fn)
		if (c == '\\')
			c = '/';

	FT_Face face;
	if (auto error = FT_New_Face(ft, fn.c_str(), 0, &face))
		throw std::runtime_error("Faile to load font: " + path.string() + ": " + std::to_string(error));

	FT_Set_Pixel_Sizes(face, 0, height);

	FT_GlyphSlot g = face->glyph;

	glm::uvec2 atlasSize = {0, 0};
	for (auto i = 32; i < 256; i++) {
		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			std::clog << "Failed to load glyth for character " << static_cast<char>(i) << "\n";
			continue;
		}

		atlasSize.x += g->bitmap.width;
		atlasSize.y = std::max(atlasSize.y, g->bitmap.rows);
	}

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasSize.x, atlasSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

	int x = 0;
	for (auto i = 32; i < 256; i++) {
		if (FT_Load_Char(face, i, FT_LOAD_RENDER))
			continue;

		auto& c = m_glyphs.emplace_back();
		c.advance.x = g->advance.x >> 6;
		c.advance.y = g->advance.y >> 6;
		c.size.x = g->bitmap.width;
		c.size.y = g->bitmap.rows;
		c.bearing.x = g->bitmap_left;
		c.bearing.y = g->bitmap_top;
		c.texX = static_cast<float>(x) / atlasSize.x;

		glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

		x += g->bitmap.width;
	}

	m_atlasSize = atlasSize;

	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

Font::Font(Font&& other) noexcept {
	swap(other);
}

Font& Font::operator=(Font&& other) noexcept {
	swap(other);
	return *this;
}

Font::~Font() {
	if (m_id != 0)
		glDeleteTextures(1, &m_id);
}

void renderText(gl::Buffer& buffer, int x, int y, const Font& font, const std::string& text, float sx, float sy) {
	struct Vertex {
		float x;
		float y;
		float s;
		float t;
	};
	std::vector<Vertex> vertices;
	vertices.reserve(6 * text.size());

	for (const auto& c : text) {
		const auto& g = font.m_glyphs[std::clamp(static_cast<unsigned char>(c) - 32, 0, 255 - 32)];
		float x2 = x + g.bearing.x * sx;
		float y2 = y + g.bearing.y * sy;
		float w = g.size.x * sx;
		float h = g.size.y * sy;

		// advance the cursor to the start of the next character
		x += g.advance.x * sx;
		y += g.advance.y * sy;

		// skip m_glyphs that have no pixels
		if (!w || !h)
			continue;

		vertices.push_back({x2, y2, g.texX, 0});
		vertices.push_back({x2 + w, y2, g.texX + g.size.x / font.m_atlasSize.x, 0});
		vertices.push_back({x2, y2 - h, g.texX, g.size.y / font.m_atlasSize.y});
		vertices.push_back({x2, y2 - h, g.texX, g.size.y / font.m_atlasSize.y});
		vertices.push_back({x2 + w, y2, g.texX + g.size.x / font.m_atlasSize.x, 0});
		vertices.push_back({x2 + w, y2 - h, g.texX + g.size.x / font.m_atlasSize.x, g.size.y / font.m_atlasSize.y});
	}

	glBindBuffer(GL_ARRAY_BUFFER, buffer.id());
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex), nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(2 * sizeof(float)));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, font.m_id);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void Font::swap(Font& other) {
	using std::swap;
	swap(m_id, other.m_id);
	swap(m_atlasSize, other.m_atlasSize);
	swap(m_glyphs, other.m_glyphs);
}