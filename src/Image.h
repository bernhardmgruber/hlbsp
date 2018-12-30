#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class Image {
public:
	Image() = default;
	Image(unsigned int width, unsigned int height, unsigned int channels);
	explicit Image(const fs::path& path);
	Image(const Image& img, unsigned int channels);
	Image(const Image&) = default;
	auto operator=(const Image&) -> Image& = default;
	Image(Image&&) = default;
	auto operator=(Image&&) -> Image& = default;

	auto operator()(unsigned int x, unsigned int y) -> std::uint8_t*;
	auto operator()(unsigned int x, unsigned int y) const -> const std::uint8_t*;

	void Save(const fs::path& path) const;

	std::vector<std::uint8_t> data;
	unsigned int channels = 0;
	unsigned int width = 0;
	unsigned int height = 0;
};
