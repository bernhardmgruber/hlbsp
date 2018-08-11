#pragma once

#include "Image.h"
#include "bspdef.h"
#include <cstdint>
#include <fstream>
#include <optional>

struct WadHeader {
	char    magic[4];  // should be WAD2/WAD3
	int32_t nDir;      // number of directory entries
	int32_t dirOffset; // offset into directory
};

// Directory entry structure
struct WadDirEntry {
	int32_t nFilePos;             // offset in WAD
	int32_t nDiskSize;            // size in file
	int32_t nSize;                // uncompressed size
	int8_t  type;                // type of entry
	bool    compressed;           // 0 if none
	int16_t nDummy;               // not used
	char    name[bsp30::MAXTEXTURENAME]; // must be null terminated
};

// Structure for holding data for mipmap textires
struct MipmapTexture {
	Image Img[bsp30::MIPLEVELS];
};

class Wad {
public:
	explicit Wad(const fs::path& path); // Opens a WAD File and loads it's directory for texture searching

	auto        loadTexture(const char* name) -> std::optional<MipmapTexture>;
	auto        LoadDecalTexture(const char* name) -> std::optional<MipmapTexture>;
	static void CreateMipTexture(const std::vector<uint8_t>& rawTexture, MipmapTexture& pMipTex); // Creates a Miptexture out of the raw texture data

private:
	std::ifstream            wadFile;
	std::vector<WadDirEntry> dirEntries;

	void LoadDirectory(); // Loads the directory of the WAD file for further texture finding
	auto GetTexture(const char* name) -> std::vector<uint8_t>;
	void CreateDecalTexture(const std::vector<uint8_t>& rawTexture, MipmapTexture& pMipTex);
};
