#pragma once

#include <boost/dynamic_bitset.hpp>

#include <optional>
#include <string_view>

#include "bspdef.h"
#include "Entity.h"
#include "Wad.h"
#include "IO.h"

struct FaceTexCoords {
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec2> lightmapCoords;
};

struct Decal {
	uint32_t texIndex;
	glm::vec3 normal;
	glm::vec3 vec[4];
};

struct Hull {
	bsp30::ClipNode* clipnodes;
	bsp30::Plane* planes;
	int firstclipnode;
	int lastclipnode;
};

class Bsp {
public:
	explicit Bsp(const fs::path& filename);
	~Bsp();

	auto FindEntity(std::string_view name) -> Entity*;
	auto FindEntity(std::string_view name) const -> const Entity*;
	auto FindEntities(std::string_view name) -> std::vector<Entity*>;

	auto loadSkyBox() const -> std::optional<std::array<Image, 6>>;

	bsp30::Header header{};                       // Stores the header
	std::vector<bsp30::Vertex> vertices;          // Stores the vertices
	std::vector<bsp30::Edge> edges;               // Stores the edges
	std::vector<bsp30::SurfEdge> surfEdges;       // Stores the surface edges
	std::vector<bsp30::Node> nodes;               // Stores the nodes
	std::vector<bsp30::Leaf> leaves;              // Stores the leafs
	std::vector<bsp30::MarkSurface> markSurfaces; // Stores the marksurfaces
	std::vector<bsp30::Plane> planes;             // Stores the planes
	std::vector<bsp30::Face> faces;               // Stores the faces
	std::vector<bsp30::ClipNode> clipNodes;
	std::vector<bsp30::Model> models;                   // Stores the models
	bsp30::TextureHeader textureHeader{};               // Stores the texture header
	std::vector<bsp30::MipTex> mipTextures;             // Stores the miptextures
	std::vector<bsp30::MipTexOffset> mipTextureOffsets; // Stores the miptexture offsets
	std::vector<bsp30::TextureInfo> textureInfos;       // Stores the texture infos

	std::vector<FaceTexCoords> faceTexCoords; // Stores precalculated texture and lightmap coordinates for every vertex

	std::vector<Entity> entities;
	std::vector<unsigned int> brushEntities;   // Indices of brush entities in entities
	std::vector<unsigned int> specialEntities; // IndicUnloadWadFileses of special entities in entities
	std::vector<Wad> wadFiles;
	std::vector<Wad> decalWads;
	std::vector<Decal> m_decals;
	std::vector<boost::dynamic_bitset<std::uint8_t>> visLists; // Stores the vis lists for all faces

	std::vector<MipmapTexture> m_textures;
	std::vector<Image> m_lightmaps;

	std::vector<bsp30::ClipNode> hull0ClipNodes;
	std::array<Hull, bsp30::MAX_MAP_HULLS> hulls;

private:
	void LoadWadFiles(std::string wadStr);                                      // Loads and prepares the wad files for further texture loading
	void UnloadWadFiles();                                                      // Unloads all wad files and frees allocated memory
	void LoadTextures(std::ifstream& file);                                     // Loads the textures either from the wad file or directly from the bsp file
	auto LoadTextureFromWads(const char* name) -> std::optional<MipmapTexture>; // Finds and loads a texture from a wad file by the given name
	auto LoadDecalTexture(const char* name) -> std::optional<MipmapTexture>;
	void LoadDecals();
	void LoadLightMaps(const std::vector<std::uint8_t>& pLightMapData); // Loads lightmaps and calculates extends and coordinates
	void LoadHulls();

	void ParseEntities(const std::string& entitiesString); // Parses the entity lump of the bsp file into single entity classes

	void CountVisLeafs(int iNode, int& count);                                                                                 // Counts the number of visLeaves recursively
	auto decompressVIS(int leaf, const std::vector<std::uint8_t>& compressedVis) const -> boost::dynamic_bitset<std::uint8_t>; // Get the PVS for a given leaf and return it in the form of a pointer to a bool array

	auto findLeaf(glm::vec3 pos, int node = 0) const -> std::optional<int>; // Recursivly walks through the BSP tree to find the leaf where the camera is in

	friend class BspRenderable;
};
