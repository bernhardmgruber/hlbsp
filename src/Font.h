#pragma once

#include <glm/vec2.hpp>

#include <experimental/filesystem>
#include <vector>

#include "Image.h"

class Font {
public:
	Font() = default;
	Font(const std::experimental::filesystem::path& name, int height);
	Font(const Font&) = delete;
	Font& operator=(const Font&) = delete;
	Font(Font&& other) noexcept;
	Font& operator=(Font&& other) noexcept;
	~Font();

	auto glyphs() const -> const auto& { return m_glyphs; }
	auto atlas() const -> const auto& { return m_atlas; }

private:
	struct Glyph {
		glm::vec2 advance;
		glm::uvec2 size;
		glm::ivec2 bearing;
		float texX;
	};

	void swap(Font& other);

	std::vector<Glyph> m_glyphs;
	Image m_atlas;
};
