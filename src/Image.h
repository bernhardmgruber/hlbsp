#pragma once

#include <cstdint>
#include <experimental/filesystem>
#include <vector>

namespace fs = std::experimental::filesystem;

class Image {
public:
	Image() = default;
	Image(unsigned int width, unsigned int height, unsigned int channels);
	explicit Image(const fs::path& path);
	Image(const Image&) = default;
	Image& operator=(const Image&) = default;
	Image(Image&&) = default;
	Image& operator=(Image&&) = default;

	void Save(const fs::path& path) const;

	std::vector<std::uint8_t> data;
	unsigned int channels = 0;
	unsigned int width = 0;
	unsigned int height = 0;
};
