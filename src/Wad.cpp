#include "Wad.h"

#include <boost/algorithm/string.hpp>

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "IO.h"

namespace {
	const auto sqrt2 = std::sqrt(2.0);

	void ApplyAlphaSections(Image& pTex) {
		std::vector<unsigned char> pRGBTexture(pTex.width * pTex.height * 4, 0x00);

		// Color pRGBTexture totally blue
		for (int i = 0; i < pTex.height * pTex.width; i++)
			pRGBTexture[i * 4 + 2] = 255;

		for (int y = 0; y < pTex.height; y++) {
			for (int x = 0; x < pTex.width; x++) {
				int index = y * pTex.width + x;

				if ((pTex.data[index * 4] == 0) && (pTex.data[index * 4 + 1] == 0) && (pTex.data[index * 4 + 2] == 255)) {
					// Blue color signifies a transparent portion of the texture. zero alpha for blending and
					// to get rid of blue edges choose the average color of the nearest non blue pixels

					//First set pixel black and transparent
					pTex.data[index * 4 + 2] = 0;
					pTex.data[index * 4 + 3] = 0;

					int count = 0;
					unsigned int RGBColorSum[3] = {0, 0, 0};

					//left above pixel
					if ((x > 0) && (y > 0)) {
						int iPixel = ((y - 1) * pTex.width + (x - 1)) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += (unsigned int)((float)pTex.data[iPixel + 0] * sqrt2);
							RGBColorSum[1] += (unsigned int)((float)pTex.data[iPixel + 1] * sqrt2);
							RGBColorSum[2] += (unsigned int)((float)pTex.data[iPixel + 2] * sqrt2);
							count++;
						}
					}

					//above pixel
					if ((x >= 0) && (y > 0)) {
						int iPixel = ((y - 1) * pTex.width + x) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += pTex.data[iPixel];
							RGBColorSum[1] += pTex.data[iPixel + 1];
							RGBColorSum[2] += pTex.data[iPixel + 2];
							count++;
						}
					}

					//right above pixel
					if ((x < pTex.width - 1) && (y > 0)) {
						int iPixel = ((y - 1) * pTex.width + (x + 1)) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += (unsigned int)((float)pTex.data[iPixel + 0] * sqrt2);
							RGBColorSum[1] += (unsigned int)((float)pTex.data[iPixel + 1] * sqrt2);
							RGBColorSum[2] += (unsigned int)((float)pTex.data[iPixel + 2] * sqrt2);
							count++;
						}
					}

					//left pixel
					if (x > 0) {
						int iPixel = (y * pTex.width + (x - 1)) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += pTex.data[iPixel];
							RGBColorSum[1] += pTex.data[iPixel + 1];
							RGBColorSum[2] += pTex.data[iPixel + 2];
							count++;
						}
					}

					//right pixel
					if (x < pTex.width - 1) {
						int iPixel = (y * pTex.width + (x + 1)) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += pTex.data[iPixel];
							RGBColorSum[1] += pTex.data[iPixel + 1];
							RGBColorSum[2] += pTex.data[iPixel + 2];
							count++;
						}
					}

					//left underneath pixel
					if ((x > 0) && (y < pTex.height - 1)) {
						int iPixel = ((y + 1) * pTex.width + (x - 1)) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += (unsigned int)((float)pTex.data[iPixel + 0] * sqrt2);
							RGBColorSum[1] += (unsigned int)((float)pTex.data[iPixel + 1] * sqrt2);
							RGBColorSum[2] += (unsigned int)((float)pTex.data[iPixel + 2] * sqrt2);
							count++;
						}
					}

					//underneath pixel
					if ((x >= 0) && (y < pTex.height - 1)) {
						int iPixel = ((y + 1) * pTex.width + x) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += pTex.data[iPixel];
							RGBColorSum[1] += pTex.data[iPixel + 1];
							RGBColorSum[2] += pTex.data[iPixel + 2];
							count++;
						}
					}

					//right underneath pixel
					if ((x < pTex.width - 1) && (y < pTex.height - 1)) {
						int iPixel = ((y + 1) * pTex.width + (x + 1)) * 4;
						if (!((pTex.data[iPixel] == 0) && (pTex.data[iPixel + 1] == 0) && (pTex.data[iPixel + 2] == 255))) {
							RGBColorSum[0] += (unsigned int)((float)pTex.data[iPixel + 0] * sqrt2);
							RGBColorSum[1] += (unsigned int)((float)pTex.data[iPixel + 1] * sqrt2);
							RGBColorSum[2] += (unsigned int)((float)pTex.data[iPixel + 2] * sqrt2);
							count++;
						}
					}

					if (count > 0) {
						RGBColorSum[0] /= count;
						RGBColorSum[1] /= count;
						RGBColorSum[2] /= count;

						pRGBTexture[index * 4 + 0] = RGBColorSum[0];
						pRGBTexture[index * 4 + 1] = RGBColorSum[1];
						pRGBTexture[index * 4 + 2] = RGBColorSum[2];
					}
				}
			}
		}

		//Merge pTex and pRGBTexture
		for (int y = 0; y < pTex.height; y++) {
			for (int x = 0; x < pTex.width; x++) {
				int index = y * pTex.width + x;

				if ((pRGBTexture[index * 4] != 0) || (pRGBTexture[index * 4 + 1] != 0) || (pRGBTexture[index * 4 + 2] != 255) || (pRGBTexture[index * 4 + 3] != 0))
					memcpy(&pTex.data[index * 4], &pRGBTexture[index * 4], sizeof(unsigned char) * 4);
			}
		}
	}

	auto texNameLess(const char* a, const char* b) {
#ifdef WIN32
		return _stricmp(a, b) < 0;
#else
		return strcasecmp(a, b) < 0;
#endif
	}

	auto texNameEqual(const char* a, const char* b) {
#ifdef WIN32
		return _stricmp(a, b) == 0;
#else
		return strcasecmp(a, b) == 0;
#endif
	}
}

Wad::Wad(const fs::path& path)
	: wadFile(path, std::ios::binary) {
	if (!wadFile)
		throw std::ios::failure("Failed to open file " + path.string() + " for reading");
	wadFile.exceptions(std::ios::badbit | std::ios::failbit);
	LoadDirectory();
}

auto Wad::loadTexture(const char* name) -> std::optional<MipmapTexture> {
	auto rawTex = GetTexture(name);
	if (rawTex.empty())
		return {};

	MipmapTexture tex;
	CreateMipTexture(rawTex, tex);
	return tex;
}

auto Wad::LoadDecalTexture(const char* name) -> std::optional<MipmapTexture> {
	auto rawTex = GetTexture(name);
	if (rawTex.empty())
		return {};

	MipmapTexture tex;
	CreateDecalTexture(rawTex, tex);
	return tex;
}

void Wad::LoadDirectory() {
	auto header = read<WadHeader>(wadFile);

	// check magic
	if (header.magic[0] != 'W' || header.magic[1] != 'A' || header.magic[2] != 'D' || (header.magic[3] != '2' && header.magic[3] != '3'))
		throw std::ios::failure("Unknown WAD magic number: " + std::string(header.magic, 4));

	// read and sort directory
	dirEntries.resize(header.nDir);
	wadFile.seekg(header.dirOffset);
	readVector(wadFile, dirEntries);

	std::sort(begin(dirEntries), end(dirEntries), [](const WadDirEntry& a, const WadDirEntry& b) {
		return texNameLess(a.name, b.name);
	});
}

auto Wad::GetTexture(const char* name) -> std::vector<uint8_t> {
	const auto it = std::lower_bound(begin(dirEntries), end(dirEntries), name, [](const WadDirEntry& e, const char* name) {
		return texNameLess(e.name, name);
	});

	if (it == end(dirEntries) || !texNameEqual(it->name, name))
		return {};

	// we can only handle uncompressed formats
	if (it->compressed)
		throw std::runtime_error("WAD texture cannot be loaded. Cannot read compressed items");

	wadFile.seekg(it->nFilePos);
	return readVector<uint8_t>(wadFile, it->nSize);
}

void Wad::CreateMipTexture(const std::vector<uint8_t>& rawTexture, MipmapTexture& mipTex) {
	const auto* rawMipTex = (bsp30::MipTex*)rawTexture.data();

	auto width = rawMipTex->width;
	auto height = rawMipTex->height;
	const auto palOffset = rawMipTex->offsets[3] + (width / 8) * (height / 8) + 2;
	const auto* palette = rawTexture.data() + palOffset;

	for (int level = 0; level < bsp30::MIPLEVELS; level++) {
		const auto* pixel = &(rawTexture[rawMipTex->offsets[level]]);

		auto& img = mipTex.Img[level];
		img.channels = 4;
		img.width = width;
		img.height = height;
		img.data.resize(width * height * 4);

		for (int i = 0; i < height * width; i++) {
			int palIndex = pixel[i] * 3;

			img.data[i * 4 + 0] = palette[palIndex + 0];
			img.data[i * 4 + 1] = palette[palIndex + 1];
			img.data[i * 4 + 2] = palette[palIndex + 2];
			img.data[i * 4 + 3] = 255;
		}

		ApplyAlphaSections(mipTex.Img[level]);

		width /= 2;
		height /= 2;
	}
}

void Wad::CreateDecalTexture(const std::vector<uint8_t>& rawTexture, MipmapTexture& mipTex) {
	const auto* rawMipTex = (bsp30::MipTex*)rawTexture.data();

	auto width = rawMipTex->width;
	auto height = rawMipTex->height;
	const auto palOffset = rawMipTex->offsets[3] + (width / 8) * (height / 8) + 2;
	const auto* palette = rawTexture.data() + palOffset;
	const auto* color = palette + 255 * 3;

	for (int level = 0; level < bsp30::MIPLEVELS; level++) {
		const auto* pixel = &(rawTexture[rawMipTex->offsets[level]]);

		auto& img = mipTex.Img[level];
		img.channels = 4;
		img.width = width;
		img.height = height;
		img.data.resize(width * height * 4);

		for (int i = 0; i < height * width; i++) {
			int palIndex = pixel[i] * 3;

			img.data[i * 4 + 0] = color[0];
			img.data[i * 4 + 1] = color[1];
			img.data[i * 4 + 2] = color[2];
			img.data[i * 4 + 3] = 255 - palette[palIndex];
		}

		ApplyAlphaSections(mipTex.Img[level]);

		width /= 2;
		height /= 2;
	}
}
