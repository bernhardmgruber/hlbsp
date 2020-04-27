#pragma once

#include "mathlib.h"
#include <cstdint>

namespace bsp30 {
	constexpr auto MAX_MAP_HULLS = 4;

	constexpr auto MAX_MAP_MODELS = 400;
	constexpr auto MAX_MAP_BRUSHES = 4096;
	constexpr auto MAX_MAP_ENTITIES = 1024;
	constexpr auto MAX_MAP_ENTSTRING = (128 * 1024);

	constexpr auto MAX_MAP_PLANES = 32767;
	constexpr auto MAX_MAP_NODES = 32767; // because negative shorts are leaves
	constexpr auto MAX_MAP_CLIPNODES = 32767;
	constexpr auto MAX_MAP_LEAFS = 8192;
	constexpr auto MAX_MAP_VERTS = 65535;
	constexpr auto MAX_MAP_FACES = 65535;
	constexpr auto MAX_MAP_MARKSURFACES = 65535;
	constexpr auto MAX_MAP_TEXINFO = 8192;
	constexpr auto MAX_MAP_EDGES = 256000;
	constexpr auto MAX_MAP_SURFEDGES = 512000;
	constexpr auto MAX_MAP_TEXTURES = 512;
	constexpr auto MAX_MAP_MIPTEX = 0x200000;
	constexpr auto MAX_MAP_LIGHTING = 0x200000;
	constexpr auto MAX_MAP_VISIBILITY = 0x200000;

	constexpr auto MAX_MAP_PORTALS = 65536;

	constexpr auto MAX_KEY = 32;
	constexpr auto MAX_VALUE = 1024;

	// BSP-30 files contain these lumps
	enum LumpType {
		LUMP_ENTITIES = 0,
		LUMP_PLANES = 1,
		LUMP_TEXTURES = 2,
		LUMP_VERTEXES = 3,
		LUMP_VISIBILITY = 4,
		LUMP_NODES = 5,
		LUMP_TEXINFO = 6,
		LUMP_FACES = 7,
		LUMP_LIGHTING = 8,
		LUMP_CLIPNODES = 9,
		LUMP_LEAFS = 10,
		LUMP_MARKSURFACES = 11,
		LUMP_EDGES = 12,
		LUMP_SURFEDGES = 13,
		LUMP_MODELS = 14,
		HEADER_LUMPS = 15,
	};

	// Leaf content values
	enum ContentType {
		CONTENTS_EMPTY = -1,
		CONTENTS_SOLID = -2,
		CONTENTS_WATER = -3,
		CONTENTS_SLIME = -4,
		CONTENTS_LAVA = -5,
		CONTENTS_SKY = -6,
		CONTENTS_ORIGIN = -7,
		CONTENTS_CLIP = -8,
		CONTENTS_CURRENT_0 = -9,
		CONTENTS_CURRENT_90 = -10,
		CONTENTS_CURRENT_180 = -11,
		CONTENTS_CURRENT_270 = -12,
		CONTENTS_CURRENT_UP = -13,
		CONTENTS_CURRENT_DOWN = -14,
		CONTENTS_TRANSLUCENT = -15,
	};

	// plane types
	enum PlaneType {
		// plane is perpendicular to given axis
		PLANE_X = 0,
		PLANE_Y = 1,
		PLANE_Z = 2,
		// non-axial plane is snapped to the nearest
		PLANE_ANYX = 3,
		PLANE_ANYY = 4,
		PLANE_ANYZ = 5,
	};

	// render modes
	enum RenderMode {
		RENDER_MODE_NORMAL = 0,
		RENDER_MODE_COLOR = 1,
		RENDER_MODE_TEXTURE = 2,
		RENDER_MODE_GLOW = 3,
		RENDER_MODE_SOLID = 4,
		RENDER_MODE_ADDITIVE = 5,
	};

	///  @brief Describes a lump in the BSP file
	///  To read the different lumps from the given BSP file, every lump entry file states the beginning of each lump as an offset relativly to the beginning of the file. Additionally, the lump entry also gives the length of the addressed lump in bytes.
	struct Lump {
		int32_t offset; ///< File offset to data
		int32_t length; ///< Length of data
	};

	/// @brief The BSP file header
	/// The file header begins with an 32bit integer containing the file version of the BSP file (the magic number). This should be 30 for a valid BSP file used by the Half-Life Engine.
	/// Subseqently, there is an array of entries for the so-called lumps. A lump is more or less a section of the file containing a specific type of data. The lump entries in the file header address these lumps, accessed by the 15 predefined indexes.
	struct Header {
		int32_t version;         ///< Version number, must be 30 for a valid HL BSP file
		Lump lump[HEADER_LUMPS]; ///< Stores the directory of lumps.
	};

	///  @brief Describes a node of the BSP Tree
	struct Node {
		uint32_t planeIndex;           // Index into planes lump
		int16_t childIndex[2];         // If > 0, then indices into Nodes otherwise bitwise inverse indices into Leafs
		int16_t lower[3], upper[3];    // Defines bounding box
		uint16_t firstFace, faceCount; // Index and count into BSPFACES array
	};

	// Leafs lump contains leaf structures
	struct Leaf {
		int32_t content;                             // Contents enumeration, see #defines
		int32_t visOffset;                           // Offset into the compressed visibility lump
		int16_t lower[3], upper[3];                  // Defines bounding box
		uint16_t firstMarkSurface, markSurfaceCount; // Index and count into MarkSurface array
		uint8_t ambientLevels[4];                    // Ambient sound levels
	};

	// Leaves index into marksurfaces, which index into faces
	using MarkSurface = uint16_t;

	// Planes lump contains plane structures
	struct Plane {
		glm::vec3 normal; // The planes normal vector
		float dist{};     // Plane equation is: normal * X = dist
		int32_t type{};   // Plane type, see #defines
	};

	// Vertex lump is an array of float triples (glm::vec3)
	using Vertex = glm::vec3;

	// Edge struct contains the begining and end vertex for each edge
	struct Edge {
		uint16_t vertexIndex[2]; // Indices into vertex array
	};

	// Faces are equal to the polygons that make up the world
	struct Face {
		uint16_t planeIndex;     // Index of the plane the face is parallel to
		uint16_t planeSide;      // Set if different normals orientation
		uint32_t firstEdgeIndex; // Index of the first edge (in the surfedge array)
		uint16_t edgeCount;      // Number of consecutive surfedges
		uint16_t textureInfo;    // Index of the texture info structure
		uint8_t styles[4];       // Specify lighting styles
		//       styles[0]             // type of lighting, for the face
		//       styles[1]             // from 0xFF (dark) to 0 (bright)
		//       styles[2], styles[3] // two additional light models
		uint32_t lightmapOffset; // Offsets into the raw lightmap data
	};

	// Surfedges lump is array of signed int indices into edge lump, where a negative index indicates
	// using the referenced edge in the opposite direction. Faces index into surfEdges, which index
	// into edges, which finally index into vertices.
	using SurfEdge = int32_t;

	// Textures lump begins with a header, followed by offsets to MipTex structures, then MipTex structures
	struct TextureHeader {
		uint32_t mipTextureCount; // Number of MipTex structures
	};

	// 32-bit offsets (within texture lump) to (mipTextureCount) MipTex structures
	using MipTexOffset = int32_t;

	// MipTex structures which defines a Texture
	constexpr auto MAXTEXTURENAME = 16;
	constexpr auto MIPLEVELS = 4;
	struct MipTex {
		char name[MAXTEXTURENAME];   // Name of texture, for reference from external WAD file
		uint32_t width, height;      // Extends of the texture
		uint32_t offsets[MIPLEVELS]; // Offsets to MIPLEVELS texture mipmaps, if 0 texture data is stored in an external WAD file
	};

	// Texinfo lump contains texinfo structures
	struct TextureInfo {
		glm::vec3 s;            // 1st row of texture matrix
		float sShift{};         // Texture shift in s direction
		glm::vec3 t;            // 2nd row of texture matrix - multiply 1st and 2nd by vertex to get texture coordinates
		float tShift{};         // Texture shift in t direction
		uint32_t miptexIndex{}; // Index into textures array
		uint32_t flags{};       // Texture flags, seems to always be 0
	};

	struct Model {
		glm::vec3 lower, upper;                  // Defines bounding box
		glm::vec3 origin;                        // Coordinates to move the coordinate system before drawing the model
		int32_t headNodesIndex[MAX_MAP_HULLS]{}; // Index into nodes array
		int32_t visLeaves{};                     // No idea, sometimes called numleafs in HLDS
		int32_t firstFace{}, faceCount{};        // Index and count into face array
	};

	struct ClipNode {
		int32_t planeIndex;    // Index into planes
		int16_t childIndex[2]; // negative numbers are contents behind and in front of the plane
	};
}
