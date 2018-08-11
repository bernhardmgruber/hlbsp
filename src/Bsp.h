#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

#include <experimental/filesystem>
#include <string_view>

#include "bspdef.h"
#include "entity.h"
#include "wad.h"

namespace fs = std::experimental::filesystem;

// texture coordinates for a vertex
struct TexCoord {
	float s;
	float t;
};

struct FaceTexCoords {
	std::vector<TexCoord> texCoords;
	std::vector<TexCoord> lightmapCoords;
};

struct Decal {
	GLuint nTex{};
	vec3   normal;
	vec3   vec[4];
};

class Bsp {
public:
	explicit Bsp(const fs::path& filename, bool& g_bTextures, bool& g_bLightmaps);
	~Bsp();

	auto FindEntity(std::string_view name) -> Entity*;
	auto FindEntities(std::string_view name) -> std::vector<Entity*>;

	auto hasSkyBox() const -> bool;

	// Rendering
	void RenderSkybox(vec3 vPos) const;
	void RenderStaticGeometry(vec3 vPos) const;
	void RenderBrushEntities(vec3 vPos) const;
	void RenderDecals() const;
	void RenderLeavesOutlines() const;
	void RenderLeafOutlines(int iLeaf) const;

private:
	bsp30::Header                     header{};     // Stores the header
	std::vector<bsp30::Vertex>        vertices;     // Stores the vertices
	std::vector<bsp30::Edge>          edges;        // Stores the edges
	std::vector<bsp30::SurfEdge>      surfEdges;    // Stores the surface edges
	std::vector<bsp30::Node>          nodes;        // Stores the nodes
	std::vector<bsp30::Leaf>          leaves;       // Stores the leafs
	std::vector<bsp30::MarkSurface>   markSurfaces; // Stores the marksurfaces
	std::vector<bsp30::Plane>         planes;       // Stores the planes
	std::vector<bsp30::Face>          faces;        // Stores the faces
	std::vector<bsp30::ClipNode>      clipNodes;
	std::vector<bsp30::Model>         models;            // Stores the models
	bsp30::TextureHeader              textureHeader{};   // Stores the texture header
	std::vector<bsp30::MipTex>        mipTextures;       // Stores the miptextures
	std::vector<bsp30::MipTexOffset>  mipTextureOffsets; // Stores the miptexture offsets
	std::vector<bsp30::TextureInfo>   textureInfos;      // Stores the texture infos

	std::vector<FaceTexCoords> faceTexCoords;     // Stores precalculated texture and lightmap coordinates for every vertex

	std::vector<Entity>            entities;
	std::vector<unsigned int>      brushEntities;   // Indices of brush entities in entities
	std::vector<unsigned int>      specialEntities; // IndicUnloadWadFileses of special entities in entities
	std::vector<Wad>               wadFiles;
	std::vector<Wad>               decalWads;
	std::vector<Decal>             decals;
	std::vector<std::vector<bool>> visLists; // Stores the vis lists for all faces

	std::vector<GLuint>   lightmapTexIds; // Stores a lookup table where faces use their index to find the index of their lightmap texture
	std::vector<GLuint>   textureIds;     // Stores a lookup table where faces use their index to find the index of their texture
	std::optional<GLuint> skyBoxDL;       // displaylist index which stores the skybox, if NULL, there is no skybox.
	mutable std::vector<bool>     facesDrawn;     // Boolarray which avoids drawing faces twice by marking each drawn face's index in the array

	bool& g_bTextures;
	bool& g_bLightmaps;

	void LoadSkyTextures();                                                                    // Loads the sky textures from disk and creates a display list for faster rendering
	void LoadWadFiles(std::string wadStr);                                                     // Loads and prepares the wad files for further texture loading
	void UnloadWadFiles();                                                                     // Unloads all wad files and frees allocated memory
	void LoadTextures(std::ifstream& file);                                                    // Loads the textures either from the wad file or directly from the bsp file
	auto LoadTextureFromWads(const char* name) -> std::optional<MipmapTexture>; // Finds and loads a texture from a wad file by the given name
	auto LoadDecalTexture(const char* name) -> std::optional<MipmapTexture>;
	void LoadDecals();
	void LoadLightMaps(const std::vector<std::uint8_t>& pLightMapData); // Loads lightmaps and calculates extends and coordinates

	void ParseEntities(const std::string& entitiesString); // Parses the entity lump of the bsp file into single entity classes

	void              CountVisLeafs(int iNode, int& count);                         // Counts the number of visLeaves recursively
	auto uncompressPVS(int iLeaf, const std::vector<std::uint8_t>& pVisList) const -> std::vector<bool>; // Get the PVS for a given leaf and return it in the form of a pointer to a bool array

	int findLeaf(vec3 pos, int node = 0) const; // Recursivly walks through the BSP tree to find the leaf where the camera is in

	/** Rendering **/
	void RenderFace(int iFace) const;                             // Renders a face (polygon) by the given index
	void RenderLeaf(int iLeaf) const;                             // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
	void RenderBSP(int iNode, int iCurrentLeaf, vec3 vPos) const; // Recursively walks through the BSP tree and draws it
	void RenderBrushEntity(int iEntity, vec3 vPos) const;         // Renders a brush entity by rendering each face of the associated model by the given index

	friend class GLRenderer;
};
