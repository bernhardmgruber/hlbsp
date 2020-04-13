#include "Bsp.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "IO.h"

namespace {
	const auto WAD_DIR = fs::path("../data/wads");
	const auto SKY_DIR = fs::path("../data/textures/sky");
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
	m_textures.reserve(textureHeader.mipTextureCount);

	std::size_t errors = 0;
	for (unsigned int i = 0; i < textureHeader.mipTextureCount; i++) {
		MipmapTexture& mipTexture = m_textures.emplace_back();

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
	for (auto& decalWad : decalWads)
		if (auto pMipMapTex = decalWad.LoadDecalTexture(name))
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
	std::unordered_map<std::string, unsigned int> loadedTex;

	m_decals.reserve(infodecals.size());

	// Process each decal
	for (const auto& infodecal : infodecals) {
		if (auto originStr = infodecal->findProperty("origin")) {
			int x, y, z;
			std::stringstream(*originStr) >> x >> y >> z;

			const glm::vec3 origin{x, y, z};
			const auto leaf = findLeaf(origin);
			if (!leaf) {
				std::clog << "ERROR finding decal leaf\n";
				continue;
			}

			// Loop through each face in this leaf
			for (int j = 0; j < leaves[*leaf].markSurfaceCount; j++) {
				// Find face
				const auto& face = faces[markSurfaces[leaves[*leaf].firstMarkSurface + j]];

				// Find normal
				glm::vec3 normal = planes[face.planeIndex].normal;

				// Find a vertex on the face
				glm::vec3 vertex;
				const int iEdge = surfEdges[face.firstEdgeIndex]; // This gives the index into the edge lump
				if (iEdge > 0)
					vertex = vertices[edges[iEdge].vertexIndex[0]];
				else
					vertex = vertices[edges[-iEdge].vertexIndex[1]];

				// Check if decal origin is in this face
				if (PointInPlane(origin, normal, glm::dot(normal, vertex))) {
					// texture
					const auto texName = infodecal->findProperty("texture");
					if (!texName) {
						std::clog << "ERROR retrieving texture name from decal\n";
						break;
					}

					// Check if texture has already been loaded
					auto it = loadedTex.find(*texName);
					if (it == end(loadedTex)) {
						// Load new texture
						auto mipTex = LoadDecalTexture(texName->c_str());
						if (!mipTex) {
							std::clog << "ERROR loading mipTexture " << texName << "\n";
							break;
						}
						it = loadedTex.emplace(*texName, m_textures.size()).first;
						m_textures.emplace_back(std::move(*mipTex));
					}

					const auto& texIndex = it->second;
					const auto& img0 = m_textures[texIndex].Img[0];

					const float h2 = img0.height / 2.0f;
					const float w2 = img0.width / 2.0f;

					const auto& s = textureInfos[face.textureInfo].s;
					const auto& t = textureInfos[face.textureInfo].t;

					auto& decal = m_decals.emplace_back();
					decal.normal = normal;
					decal.texIndex = it->second;
					decal.vec[0] = origin - t * h2 - s * w2;
					decal.vec[1] = origin - t * h2 + s * w2;
					decal.vec[2] = origin + t * h2 + s * w2;
					decal.vec[3] = origin + t * h2 - s * w2;

					break;
				}
			}
		}
	}

	std::clog << "Loaded " << m_decals.size() << " decals, " << loadedTex.size() << " decal textures\n";
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

			float fMidPolyU = (fMinU + fMaxU) / 2.0f;
			float fMidPolyV = (fMinV + fMaxV) / 2.0f;
			float fMidTexU = static_cast<float>(nWidth) / 2.0f;
			float fMidTexV = static_cast<float>(nHeight) / 2.0f;

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

			Image& image = m_lightmaps.emplace_back(nWidth, nHeight, 3);
			memcpy(image.data.data(), &pLightMapData[faces[i].lightmapOffset], nWidth * nHeight * 3 * sizeof(unsigned char));

			loadedLightmaps++;
			loadedBytes += nWidth * nHeight * 3;
		} else
			m_lightmaps.emplace_back();
	}

	std::clog << "Loaded " << loadedLightmaps << " lightmaps, lightmapdatadiff: " << loadedBytes - header.lump[bsp30::LumpType::LUMP_LIGHTING].length << " bytes ";
	if ((loadedBytes - header.lump[bsp30::LumpType::LUMP_LIGHTING].length) == 0)
		std::clog << "OK\n";
	else
		std::clog << "ERRORS\n";
}

void Bsp::LoadHulls() {
	{
		// hull 0 is created from normal BSP nodes
		hull0ClipNodes.resize(nodes.size());
		std::transform(begin(nodes), end(nodes), begin(hull0ClipNodes), [&](const bsp30::Node& n) {
			bsp30::ClipNode cn;
			cn.planeIndex = n.planeIndex;
			for (int j = 0; j < 2; j++) {
				if (n.childIndex[j] < 0)
					cn.childIndex[j] = leaves[~n.childIndex[j]].content;
				else
					cn.childIndex[j] = n.childIndex[j];
			}
			return cn;
		});

		auto& hull = hulls[0];
		hull.clipnodes = hull0ClipNodes.data();
		hull.firstclipnode = 0;
		hull.lastclipnode = hull0ClipNodes.size() - 1;
		hull.planes = planes.data();

	}
	for (auto i : {1, 2}) {
		auto& hull = hulls[i];
		hull.clipnodes = clipNodes.data();
		hull.firstclipnode = 0;
		hull.lastclipnode = clipNodes.size() - 1;
		hull.planes = planes.data();
	}
}

// Checks if an entity is a valid brush entity (has a model)
auto IsBrushEntity(const Entity& e) -> bool {
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
			brushEntities.push_back(static_cast<unsigned int>(entities.size() - 1));

			// if entity has property "origin" apply to model struct for rendering
			if (auto szOrigin = e.findProperty("origin")) {
				int iModel = std::atoi(&e.findProperty("model")->c_str()[1]);
				sscanf(szOrigin->c_str(), "%f %f %f", &models[iModel].origin.x, &models[iModel].origin.y, &models[iModel].origin.z);
			}
		} else
			specialEntities.push_back(static_cast<unsigned int>(entities.size() - 1));
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
	//const auto* end = compressedVis.data() + (leaves[leaf + 1].visOffset == -1 ? header.lump[bsp30::LUMP_VISIBILITY].length : leaves[leaf + 1].visOffset);

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

	//std::clog << "PVS for leaf " << leaf << "\n";
	//std::clog << "read: " << (void*)read << "\n";
	//std::clog << "end:  " << (void*)end << "\n";
	//std::clog << "diff: " << (end - read) << "\n";

	return pvs;
}

auto Bsp::findLeaf(glm::vec3 pos, int node) const -> std::optional<int> {
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

Bsp::Bsp(const fs::path& filename) {
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

	// hulls
	LoadHulls();

	std::clog << "FINISHED LOADING BSP\n";
}

Bsp::~Bsp() = default;

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

	char size[6][3] = {"ft", "bk", "up", "dn", "rt", "lf"};
	std::array<Image, 6> result;
	for (auto i = 0; i < 6; i++)
		result[i] = Image(SKY_DIR / (*skyname + size[i] + ".tga"));
	return result;
}

auto Bsp::textures() const -> const std::vector<MipmapTexture>& {
	return m_textures;
}

auto Bsp::lightmaps() const -> const std::vector<Image>& {
	return m_lightmaps;
}

auto Bsp::decals() const -> const std::vector<Decal>& {
	return m_decals;
}
