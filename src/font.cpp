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

	m_atlas = Image(atlasSize.x, atlasSize.y, 1);

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

		for (auto gy = 0; gy < g->bitmap.rows; gy++)
			for (auto gx = 0; gx < g->bitmap.width; gx++)
				m_atlas.data[gy * m_atlas.width + x + gx] = g->bitmap.buffer[gy * g->bitmap.width + gx];

		x += g->bitmap.width;
	}

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

Font::~Font() = default;

void Font::swap(Font& other) {
	using std::swap;
	swap(m_atlas, other.m_atlas);
	swap(m_glyphs, other.m_glyphs);
}