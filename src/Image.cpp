#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Image::Image(unsigned int width, unsigned int height, unsigned int channels)
	: width(width), height(height), channels(channels), data(width * height * channels) {}

Image::Image(const Image& img, unsigned int channels)
	: Image(img.width, img.height, channels) {
	const auto count = std::min(img.channels, channels);
	for (auto y = 0; y < height; y++) {
		for (auto x = 0; x < width; x++) {
			auto src = img(x, y);
			auto dst = (*this)(x, y);
			std::copy(src, src + count, dst);
		}
	}
}

Image::Image(const fs::path& path) {
	int x, y, n;
	auto d = stbi_load(path.string().c_str(), &x, &y, &n, 0);
	if (!d)
		throw std::ios::failure("Failed to load image file: " + path.string());

	width = x;
	height = y;
	channels = n;
	data.assign(d, d + x * y * n);

	stbi_image_free(d);
}

auto Image::operator()(unsigned int x, unsigned int y) -> std::uint8_t* {
	return &data[(y * width + x) * channels];
}

auto Image::operator()(unsigned int x, unsigned int y) const -> const std::uint8_t* {
	return &data[(y * width + x) * channels];
}

void Image::Save(const fs::path& path) const {
	if (!path.has_extension())
		throw std::runtime_error("no extension");

	stbi_write_bmp(path.string().c_str(), width, height, channels, data.data());
}
