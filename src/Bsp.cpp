#include "Bsp.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "IO.h"

namespace {
	const auto WAD_DIR = fs::path("../../data/wads");
	const auto SKY_DIR = fs::path("../../data/textures/sky");

	constexpr auto DECAL_WAD_COUNT = 2;

	// Checks whether or not a texture has power of two extends and scales it if neccessary
	void AdjustTextureToPowerOfTwo(Image* pImg) {
		if (GLEW_ARB_texture_non_power_of_two)
			return;

		if (((pImg->width & (pImg->width - 1)) == 0) && ((pImg->height & (pImg->height - 1)) == 0))
			return;

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		unsigned int nPOT = 1;
		while (nPOT < pImg->height || nPOT < pImg->width)
			nPOT *= 2;

		std::vector<uint8_t> newData(nPOT * nPOT * pImg->channels);
		gluScaleImage(pImg->channels == 4 ? GL_RGBA : GL_RGB, pImg->width, pImg->height, GL_UNSIGNED_BYTE, pImg->data.data(), nPOT, nPOT, GL_UNSIGNED_BYTE, newData.data());

		pImg->width = nPOT;
		pImg->height = nPOT;
		pImg->data = std::move(newData);
	}
}

void Bsp::LoadWadFiles(std::string wadStr) {
	for (auto& c : wadStr)
		if (c == '\\')
			c = '/';

	int nWadCount = 0;
	std::size_t pos = 0;
	while (true) {
		pos++;
		const auto next = wadStr.find(';', pos);
		if (next == std::string::npos)
			break;
		auto path = wadStr.substr(pos, next - pos);

		// remove leading path except the parent folder of the wad file
		auto it = path.rfind('/');
		if (it != std::string::npos) {
			it = path.rfind('/', it - 1);
			if (it != std::string::npos)
				path.erase(0, it + 1);
		}

		wadFiles.emplace_back(WAD_DIR / path);
		std::clog << "#" << std::setw(2) << nWadCount++ << " Loaded " << path << "\n";
		pos = next;
	}

	std::clog << "Loaded " << nWadCount << " WADs ";
	std::clog << "OK\n";
}

void Bsp::UnloadWadFiles() {
	wadFiles.clear();
}

void Bsp::LoadTextures(std::ifstream& file) {
	std::clog << "Loading WADs ...\n";
	if (const auto worldSpawn = FindEntity("worldspawn"))
		if (const auto wad = worldSpawn->findProperty("wad"))
			LoadWadFiles(*wad);

	std::clog << "Loading textures ...\n";

	// generate textures from OpenGL
	textureIds.resize(textureHeader.mipTextureCount);
	glGenTextures(textureHeader.mipTextureCount, textureIds.data());

	std::size_t errors = 0;
	for (unsigned int i = 0; i < textureHeader.mipTextureCount; i++) {
		MipmapTexture mipTexture;

		if (mipTextures[i].offsets[0] == 0) {
			// texture is stored externally
			if (auto tex = LoadTextureFromWads(mipTextures[i].name))
				mipTexture = std::move(*tex);
			else {
				std::clog << "Failed to load texture " << mipTextures[i].name << " from WAD files\n";
				continue;
			}
		} else {
			// internal texture
			const auto dataSize = sizeof(uint8_t) * (mipTextures[i].offsets[3] + (mipTextures[i].height / 8) * (mipTextures[i].width / 8) + 2 + 768);
			std::vector<uint8_t> imgData(dataSize);

			file.seekg(header.lump[bsp30::LumpType::LUMP_TEXTURES].offset + mipTextureOffsets[i]);
			readVector(file, imgData);

			Wad::CreateMipTexture(imgData, mipTexture);
		}

		glBindTexture(GL_TEXTURE_2D, textureIds[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, bsp30::MIPLEVELS - 1);

		for (int j = 0; j < bsp30::MIPLEVELS; j++) {
			AdjustTextureToPowerOfTwo(&mipTexture.Img[j]);
			glTexImage2D(GL_TEXTURE_2D, j, GL_RGBA, mipTexture.Img[j].width, mipTexture.Img[j].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mipTexture.Img[j].data.data());
		}
	}

	UnloadWadFiles();

	std::clog << "Loaded " << textureHeader.mipTextureCount << " textures, " << errors << " failed ";
	if (errors == 0)
		std::clog << "OK\n";
	else
		std::clog << "ERRORS\n";

	// calculate texture coordinates
	faceTexCoords.resize(faces.size());
	for (int i = 0; i < faces.size(); ++i) {
		faceTexCoords[i].texCoords.resize(faces[i].edgeCount);

		const auto& curTexInfo = textureInfos[faces[i].textureInfo];

		for (int j = 0; j < faces[i].edgeCount; j++) {
			int edgeIndex = surfEdges[faces[i].firstEdgeIndex + j]; // This gives the index into the edge lump
			if (edgeIndex > 0) {
				faceTexCoords[i].texCoords[j].s = (glm::dot(vertices[edges[edgeIndex].vertexIndex[0]], curTexInfo.s) + curTexInfo.sShift) / mipTextures[curTexInfo.miptexIndex].width;
				faceTexCoords[i].texCoords[j].t = (glm::dot(vertices[edges[edgeIndex].vertexIndex[0]], curTexInfo.t) + curTexInfo.tShift) / mipTextures[curTexInfo.miptexIndex].height;
			} else {
				edgeIndex *= -1;
				faceTexCoords[i].texCoords[j].s = (glm::dot(vertices[edges[edgeIndex].vertexIndex[1]], curTexInfo.s) + curTexInfo.sShift) / mipTextures[curTexInfo.miptexIndex].width;
				faceTexCoords[i].texCoords[j].t = (glm::dot(vertices[edges[edgeIndex].vertexIndex[1]], curTexInfo.t) + curTexInfo.tShift) / mipTextures[curTexInfo.miptexIndex].height;
			}
		}
	}
}

auto Bsp::LoadTextureFromWads(const char* name) -> std::optional<MipmapTexture> {
	for (auto& wad : wadFiles)
		if (auto pMipMapTex = wad.loadTexture(name))
			return pMipMapTex;
	return {};
}

auto Bsp::LoadDecalTexture(const char* name) -> std::optional<MipmapTexture> {
	for (int i = 0; i < DECAL_WAD_COUNT; i++)
		if (auto pMipMapTex = decalWads[i].LoadDecalTexture(name))
			return pMipMapTex;
	return {};
}

void Bsp::LoadDecals() {
	// load Decal WADs
	decalWads.emplace_back(WAD_DIR / "valve/decals.wad");
	decalWads.emplace_back(WAD_DIR / "cstrike/decals.wad");

	// Count decals
	const auto& infodecals = FindEntities("infodecal");
	if (infodecals.empty()) {
		std::clog << "(no decals)\n";
		return;
	}

	// Texture name table for texture loading
	int nLoadedTex = 0;
	struct LoadedTex {
		std::string name;
		GLuint texID{};
		int width{};
		int height{};
	};
	std::vector<LoadedTex> aLoadedTex(infodecals.size());

	// Allocate new decals
	decals.resize(infodecals.size());

	// Process each decal
	for (int i = 0; i < infodecals.size(); i++) {
		if (auto pszOrigin = infodecals[i]->findProperty("origin")) {
			int x, y, z;
			sscanf(pszOrigin->c_str(), "%d %d %d", &x, &y, &z);

			const vec3 origin{ x, y, z };

			const auto leaf = findLeaf(origin);
			if (!leaf) {
				std::clog << "ERROR finding decal leaf\n";
				continue;
			}

			// Loop through each face in this leaf
			for (int j = 0; j < leaves[*leaf].markSurfaceCount; j++) {
				// Find face
				int iFace = markSurfaces[leaves[*leaf].firstMarkSurface + j];

				// Find normal
				vec3 normal = planes[faces[iFace].planeIndex].normal;

				// Find a vertex on the face
				vec3 vertex;
				const int iEdge = surfEdges[faces[iFace].firstEdgeIndex]; // This gives the index into the edge lump
				if (iEdge > 0)
					vertex = vertices[edges[iEdge].vertexIndex[0]];
				else
					vertex = vertices[edges[-iEdge].vertexIndex[1]];

				// Check if decal origin is in this face
				if (PointInPlane(origin, normal, glm::dot(normal, vertex))) {
					// TEXTURE
					GLuint texID = 0;
					int width = 0;
					int height = 0;

					auto texName = infodecals[i]->findProperty("texture");
					if (texName == nullptr) {
						std::clog << "ERROR retrieving texture name from decal\n";
						continue;
					}

					// Check if texture has already been loaded
					for (int k = 0; k < nLoadedTex; k++) {
						if (*texName == aLoadedTex[k].name) {
							// Found already loaded texture
							texID = aLoadedTex[k].texID;
							width = aLoadedTex[k].width;
							height = aLoadedTex[k].height;
							break;
						}
					}

					if (texID == 0) {
						// Load new texture
						auto pMipTex = LoadDecalTexture(texName->c_str());
						if (!pMipTex) {
							std::clog << "#" << std::setw(3) << i + 1 << " ERROR loading mipTexture " << texName << "\n";
							continue;
						}

						glGenTextures(1, &texID);

						glBindTexture(GL_TEXTURE_2D, texID);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, bsp30::MIPLEVELS - 1);

						for (int k = 0; k < bsp30::MIPLEVELS; k++) {
							AdjustTextureToPowerOfTwo(&pMipTex->Img[k]);
							glTexImage2D(GL_TEXTURE_2D, k, GL_RGBA, pMipTex->Img[k].width, pMipTex->Img[k].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pMipTex->Img[k].data.data());
						}

						// Decal size
						width = pMipTex->Img[0].width;
						height = pMipTex->Img[0].height;

						// Add to loaded textures
						aLoadedTex[nLoadedTex].name = *texName;
						aLoadedTex[nLoadedTex].texID = texID;
						aLoadedTex[nLoadedTex].width = width;
						aLoadedTex[nLoadedTex].height = height;
						nLoadedTex++;
					}

					const float h2 = height / 2.0f;
					const float w2 = width / 2.0f;

					const auto& s = textureInfos[faces[iFace].textureInfo].s;
					const auto& t = textureInfos[faces[iFace].textureInfo].t;

					decals[i].normal = normal;
					decals[i].nTex = texID;

					decals[i].vec[0] = origin - t * h2 - s * w2;
					decals[i].vec[1] = origin - t * h2 + s * w2;
					decals[i].vec[2] = origin + t * h2 + s * w2;
					decals[i].vec[3] = origin + t * h2 - s * w2;

					break;
				}
			}
		}
	}

	std::clog << "Loaded " << decals.size() << " decals, " << nLoadedTex << " decal textures\n";
}

void Bsp::LoadLightMaps(const std::vector<std::uint8_t>& pLightMapData) {
	std::int64_t loadedBytes = 0;
	std::size_t loadedLightmaps = 0;

	for (int i = 0; i < faces.size(); i++) {
		if (faces[i].styles[0] == 0 && static_cast<signed>(faces[i].lightmapOffset) >= -1) {
			faceTexCoords[i].lightmapCoords.resize(faces[i].edgeCount);

			/* *********** QRAD ********** */

			float fMinU = 999999;
			float fMinV = 999999;
			float fMaxU = -99999;
			float fMaxV = -99999;

			const auto& texInfo = textureInfos[faces[i].textureInfo];
			for (int j = 0; j < faces[i].edgeCount; j++) {
				int iEdge = surfEdges[faces[i].firstEdgeIndex + j];
				const auto vertex = [&] {
					if (iEdge >= 0)
						return vertices[edges[iEdge].vertexIndex[0]];
					else
						return vertices[edges[-iEdge].vertexIndex[1]];
				}();

				float fU = glm::dot(texInfo.s, vertex) + texInfo.sShift;
				if (fU < fMinU)
					fMinU = fU;
				if (fU > fMaxU)
					fMaxU = fU;

				float fV = glm::dot(texInfo.t, vertex) + texInfo.tShift;
				if (fV < fMinV)
					fMinV = fV;
				if (fV > fMaxV)
					fMaxV = fV;
			}

			auto fTexMinU = floor(fMinU / 16.0f);
			auto fTexMinV = floor(fMinV / 16.0f);
			auto fTexMaxU = ceil(fMaxU / 16.0f);
			auto fTexMaxV = ceil(fMaxV / 16.0f);

			int nWidth = static_cast<int>(fTexMaxU - fTexMinU) + 1;
			int nHeight = static_cast<int>(fTexMaxV - fTexMinV) + 1;

			/* *********** end QRAD ********* */

			/* ********** http://www.gamedev.net/community/forums/topic.asp?topic_id=538713 (last refresh: 20.02.2010) ********** */

			float fMidPolyU = (fMinU + fMaxU) / 2.0;
			float fMidPolyV = (fMinV + fMaxV) / 2.0;
			float fMidTexU = static_cast<float>(nWidth) / 2.0;
			float fMidTexV = static_cast<float>(nHeight) / 2.0;

			for (int j = 0; j < faces[i].edgeCount; ++j) {
				int iEdge = surfEdges[faces[i].firstEdgeIndex + j];
				const auto vertex = [&] {
					if (iEdge >= 0)
						return vertices[edges[iEdge].vertexIndex[0]];
					else
						return vertices[edges[-iEdge].vertexIndex[1]];
				}();

				float fU = glm::dot(texInfo.s, vertex) + texInfo.sShift;
				float fV = glm::dot(texInfo.t, vertex) + texInfo.tShift;

				float fLightMapU = fMidTexU + (fU - fMidPolyU) / 16.0f;
				float fLightMapV = fMidTexV + (fV - fMidPolyV) / 16.0f;

				faceTexCoords[i].lightmapCoords[j].s = fLightMapU / static_cast<float>(nWidth);
				faceTexCoords[i].lightmapCoords[j].t = fLightMapV / static_cast<float>(nHeight);
			}

			/* ********** end http://www.gamedev.net/community/forums/topic.asp?topic_id=538713 ********** */

			// Find unbound texture slots
			glGenTextures(1, &lightmapTexIds[i]);

			Image image(nWidth, nHeight, 3);
			memcpy(image.data.data(), &pLightMapData[faces[i].lightmapOffset], nWidth * nHeight * 3 * sizeof(unsigned char));

			AdjustTextureToPowerOfTwo(&image);

			glBindTexture(GL_TEXTURE_2D, lightmapTexIds[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data.data());

			loadedLightmaps++;
			loadedBytes += nWidth * nHeight * 3;
		} else
			lightmapTexIds[i] = 0;
	}

	std::clog << "Loaded " << loadedLightmaps << " lightmaps, lightmapdatadiff: " << loadedBytes - header.lump[bsp30::LumpType::LUMP_LIGHTING].length << " bytes ";
	if ((loadedBytes - header.lump[bsp30::LumpType::LUMP_LIGHTING].length) == 0)
		std::clog << "OK\n";
	else
		std::clog << "ERRORS\n";
}

// Checks if an entity is a valid brush entity (has a model)
bool IsBrushEntity(const Entity& e) {
	if (e.findProperty("model") != nullptr) {
		if (auto classname = e.findProperty("classname")) {
			if (*classname == "func_door_rotating" ||
				*classname == "func_door" ||
				*classname == "func_illusionary" ||
				*classname == "func_wall" ||
				*classname == "func_breakable" ||
				*classname == "func_button")
				return true;
		}
	}

	return false;
}

void Bsp::ParseEntities(const std::string& entitiesString) {
	// Loop for each entity and parse it. count number of solid and special pEntities
	std::size_t pos = 0;
	while (true) {
		pos = entitiesString.find('{', pos);
		if (pos == std::string::npos)
			break;

		auto end = entitiesString.find('}', pos);
		auto& e = entities.emplace_back(entitiesString.substr(pos + 1, end - pos - 1));
		pos = end + 1;

		if (IsBrushEntity(e)) {
			brushEntities.push_back(entities.size() - 1);

			// if entity has property "origin" apply to model struct for rendering
			if (auto szOrigin = e.findProperty("origin")) {
				int iModel = std::atoi(&e.findProperty("model")->c_str()[1]);
				sscanf(szOrigin->c_str(), "%f %f %f", &models[iModel].vOrigin.x, &models[iModel].vOrigin.y, &models[iModel].vOrigin.z);
			}
		} else
			specialEntities.push_back(entities.size() - 1);
	}

	// order brush entities so that those with RENDER_MODE_TEXTURE are at the back
	std::partition(begin(brushEntities), end(brushEntities), [this](unsigned int i) {
		if (auto szRenderMode1 = entities[i].findProperty("rendermode"))
			if (static_cast<bsp30::RenderMode>(std::stoi(*szRenderMode1)) == bsp30::RENDER_MODE_TEXTURE)
				return false;
		return true;
	});
}

void Bsp::CountVisLeafs(int iNode, int& count) {
	if (iNode < 0) {
		if (iNode == -1) // leaf 0
			return;

		if (leaves[~iNode].content == bsp30::CONTENTS_SOLID)
			return;

		count++;
		return;
	}

	CountVisLeafs(nodes[iNode].childIndex[0], count);
	CountVisLeafs(nodes[iNode].childIndex[1], count);
}

auto Bsp::decompressVIS(int leaf, const std::vector<std::uint8_t>& compressedVis) const -> boost::dynamic_bitset<std::uint8_t> {
	boost::dynamic_bitset<std::uint8_t> pvs;
	pvs.reserve(leaves.size() - 1);

	const auto* read = &compressedVis[leaves[leaf].visOffset];
	const auto* end = compressedVis.data() + (leaves[leaf + 1].visOffset == -1 ? header.lump[bsp30::LUMP_VISIBILITY].length : leaves[leaf + 1].visOffset);

	const auto row = (visLists.size() + 7) / 8;
	while (pvs.size() / 8 < row) {
		if (*read)
			pvs.append(*read);
		else {
			// run of 0s, number of 0s is stored in next byte
			read++;
			for (auto i = 0; i < *read; i++) {
				pvs.append(0x00);
				if (pvs.size() / 8 >= row)
					break;
			}
		}

		read++;
	}

	std::clog << "PVS for leaf " << leaf << "\n";
	std::clog << "read: " << (void*)read << "\n";
	std::clog << "end:  " << (void*)end << "\n";
	std::clog << "diff: " << (end - read) << "\n";

	return pvs;
}

auto Bsp::findLeaf(vec3 pos, int node) const -> std::optional<int> {
	// Run once for each child
	for (const auto& childIndex : nodes[node].childIndex) {
		// If the index is positive  it is an index into the nodes array
		if (childIndex >= 0) {
			if (PointInBox(pos, nodes[childIndex].lower, nodes[childIndex].upper))
				return findLeaf(pos, childIndex);
		}
		// Else, bitwise inversed, it is an index into the leaf array
		// Do not test solid leaf 0
		else if (~childIndex != 0) {
			if (PointInBox(pos, leaves[~childIndex].lower, leaves[~childIndex].upper))
				return ~childIndex;
		}
	}

	return {};
}

Bsp::Bsp(const fs::path& filename, bool& g_bTextures, bool& g_bLightmaps)
	: g_bTextures(g_bTextures), g_bLightmaps(g_bLightmaps) {
	std::ifstream file(filename, std::ios::binary);
	if (!file)
		throw std::ios::failure("Failed to open file " + filename.string() + " for reading");

	std::clog << "LOADING BSP FILE: " << filename << "\n";

	// Read in the header
	read(file, header);
	if (header.version != 30)
		throw std::runtime_error("Invalid BSP version (" + std::to_string(header.version) + ") instead of 30)");

	// =================================================================
	// Get number of elements and allocate memory for them
	// =================================================================

	nodes.resize(header.lump[bsp30::LumpType::LUMP_NODES].length / sizeof(bsp30::Node));
	leaves.resize(header.lump[bsp30::LumpType::LUMP_LEAFS].length / sizeof(bsp30::Leaf));
	markSurfaces.resize(header.lump[bsp30::LumpType::LUMP_MARKSURFACES].length / sizeof(bsp30::MarkSurface));
	faces.resize(header.lump[bsp30::LumpType::LUMP_FACES].length / sizeof(bsp30::Face));
	clipNodes.resize(header.lump[bsp30::LumpType::LUMP_CLIPNODES].length / sizeof(bsp30::ClipNode));
	surfEdges.resize(header.lump[bsp30::LumpType::LUMP_SURFEDGES].length / sizeof(bsp30::SurfEdge));
	edges.resize(header.lump[bsp30::LumpType::LUMP_EDGES].length / sizeof(bsp30::Edge));
	vertices.resize(header.lump[bsp30::LumpType::LUMP_VERTEXES].length / sizeof(bsp30::Vertex));
	planes.resize(header.lump[bsp30::LumpType::LUMP_PLANES].length / sizeof(bsp30::Plane));
	models.resize(header.lump[bsp30::LumpType::LUMP_MODELS].length / sizeof(bsp30::Model));

	// =================================================================
	// Seek to and read in the data
	// =================================================================

	file.seekg(header.lump[bsp30::LumpType::LUMP_NODES].offset);
	readVector(file, nodes);

	file.seekg(header.lump[bsp30::LumpType::LUMP_LEAFS].offset);
	readVector(file, leaves);

	file.seekg(header.lump[bsp30::LumpType::LUMP_MARKSURFACES].offset);
	readVector(file, markSurfaces);

	file.seekg(header.lump[bsp30::LumpType::LUMP_FACES].offset);
	readVector(file, faces);

	file.seekg(header.lump[bsp30::LumpType::LUMP_CLIPNODES].offset);
	readVector(file, clipNodes);

	file.seekg(header.lump[bsp30::LumpType::LUMP_SURFEDGES].offset);
	readVector(file, surfEdges);

	file.seekg(header.lump[bsp30::LumpType::LUMP_EDGES].offset);
	readVector(file, edges);

	file.seekg(header.lump[bsp30::LumpType::LUMP_VERTEXES].offset);
	readVector(file, vertices);

	file.seekg(header.lump[bsp30::LumpType::LUMP_PLANES].offset);
	readVector(file, planes);

	file.seekg(header.lump[bsp30::LumpType::LUMP_MODELS].offset);
	readVector(file, models);

	// ===========================
	// Entity Operations
	// ===========================

	std::string entityBuffer;
	entityBuffer.resize(header.lump[bsp30::LumpType::LUMP_ENTITIES].length);
	file.seekg(header.lump[bsp30::LumpType::LUMP_ENTITIES].offset);
	file.read(entityBuffer.data(), entityBuffer.size());
	ParseEntities(std::move(entityBuffer));

	// ===========================
	// Texture Operations
	// ===========================

	textureInfos.resize(header.lump[bsp30::LumpType::LUMP_TEXINFO].length / sizeof(bsp30::TextureInfo));
	file.seekg(header.lump[bsp30::LumpType::LUMP_TEXINFO].offset);
	readVector(file, textureInfos);

	file.seekg(header.lump[bsp30::LumpType::LUMP_TEXTURES].offset);
	read(file, textureHeader);

	mipTextures.resize(textureHeader.mipTextureCount);
	mipTextureOffsets.resize(textureHeader.mipTextureCount);
	readVector(file, mipTextureOffsets);

	for (unsigned int i = 0; i < textureHeader.mipTextureCount; i++) {
		file.seekg(header.lump[bsp30::LumpType::LUMP_TEXTURES].offset + mipTextureOffsets[i]);
		read(file, mipTextures[i]);
	}

	// Load in the texture images and calculate the texture cordinates for each vertex to save render time
	LoadTextures(file);

	// ===========================
	// Lightmap Operations
	// ===========================

	std::clog << "Loading lightmaps ...\n";
	if (header.lump[bsp30::LumpType::LUMP_LIGHTING].length == 0)
		std::clog << "No lightmapdata found\n";
	else {
		// Allocate the memory for the OpenGL texture unit IDs
		lightmapTexIds.resize(faces.size());

		std::vector<std::uint8_t> pLightMapData(header.lump[bsp30::LumpType::LUMP_LIGHTING].length);
		file.seekg(header.lump[bsp30::LumpType::LUMP_LIGHTING].offset);
		readVector(file, pLightMapData);

		LoadLightMaps(pLightMapData);
	}

	// ===========================
	// Decal Operations
	// ===========================

	std::clog << "Loading decals ...\n";
	LoadDecals();

	// ===========================
	// Visibility List Operations
	// ===========================

	if (header.lump[bsp30::LumpType::LUMP_VISIBILITY].length > 0) {
		// Allocate memory for the compressed vis lists
		std::vector<std::uint8_t> compressedVis(header.lump[bsp30::LumpType::LUMP_VISIBILITY].length);

		file.seekg(header.lump[bsp30::LumpType::LUMP_VISIBILITY].offset);
		readVector(file, compressedVis);

		std::clog << "Decompressing VIS ...\n";

		int count = 0;
		CountVisLeafs(0, count);

		visLists.resize(count);

		for (int i = 0; i < count; i++) {
			if (leaves[i + 1].visOffset >= 0)
				visLists[i] = decompressVIS(i + 1, compressedVis);
		}
	} else
		std::clog << "No VIS found\n";

	file.close();

	std::clog << "FINISHED LOADING BSP\n";
}

Bsp::~Bsp() {
	glDeleteTextures(textureIds.size(), textureIds.data());

	//lightmaps
	for (unsigned int& lightmapTexId : lightmapTexIds) {
		if (lightmapTexId != 0)
			glDeleteTextures(1, &lightmapTexId);
	}
}

auto Bsp::FindEntity(std::string_view name) -> Entity* {
	for (auto& e : entities)
		if (auto classname = e.findProperty("classname"))
			if (*classname == name)
				return &e;
	return nullptr;
}

auto Bsp::FindEntity(std::string_view name) const -> const Entity* {
	return const_cast<Bsp&>(*this).FindEntity(name);
}

std::vector<Entity*> Bsp::FindEntities(std::string_view name) {
	std::vector<Entity*> result;
	for (auto& e : entities)
		if (auto classname = e.findProperty("classname"))
			if (*classname == name)
				result.push_back(&e);
	return result;
}

auto Bsp::loadSkyBox() const -> std::optional<std::array<Image, 6>> {
	const auto worldspawn = FindEntity("worldspawn");
	if (worldspawn == nullptr)
		return {};
	const auto skyname = worldspawn->findProperty("skyname");
	if (skyname == nullptr)
		return {}; // we don't have a sky texture

	GLuint nSkyTex[6];
	glGenTextures(6, nSkyTex);

	char size[6][3] = {"ft", "bk", "up", "dn", "rt", "lf"};
	std::array<Image, 6> result;
	for (auto i = 0; i < 6; i++)
		result[i] = Image(SKY_DIR / (*skyname + size[i] + ".tga"));
	return result;
}
