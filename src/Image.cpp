#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Image::Image(unsigned int width, unsigned int height, unsigned int channels)
	: width(width), height(height), channels(channels), data(width * height * channels) {}

Image::Image(const fs::path& path) {
	int  x, y, n;
	auto d = stbi_load(path.string().c_str(), &x, &y, &n, 0);
	if (!d)
		throw std::ios::failure("Failed to load image file: " + path.string());

	width    = x;
	height   = y;
	channels = n;
	data.assign(d, d + x * y * n);

	stbi_image_free(d);
}

void Image::Save(const fs::path& path) {
	if (!path.has_extension())
		throw std::runtime_error("no extension");

	stbi_write_bmp(path.string().c_str(), width, height, channels, data.data());
}
