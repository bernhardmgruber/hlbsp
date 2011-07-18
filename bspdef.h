#ifndef BSPV32FILEDEFS_H_INCLUDED
#define BSPV32FILEDEFS_H_INCLUDED

#include <stdint.h>
#include "mathlib.h"

#define	MAX_MAP_HULLS		4

#define	MAX_MAP_MODELS		400
#define	MAX_MAP_BRUSHES		4096
#define	MAX_MAP_ENTITIES	1024
#define	MAX_MAP_ENTSTRING	(128*1024)

#define	MAX_MAP_PLANES		32767
#define	MAX_MAP_NODES		32767		// because negative shorts are leaves
#define	MAX_MAP_CLIPNODES	32767		//
#define	MAX_MAP_LEAFS		8192
#define	MAX_MAP_VERTS		65535
#define	MAX_MAP_FACES		65535
#define	MAX_MAP_MARKSURFACES 65535
#define	MAX_MAP_TEXINFO		8192
#define	MAX_MAP_EDGES		256000
#define	MAX_MAP_SURFEDGES	512000
#define	MAX_MAP_TEXTURES	512
#define	MAX_MAP_MIPTEX		0x200000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_VISIBILITY	0x200000

#define	MAX_MAP_PORTALS		65536

#define	MAX_KEY		32
#define	MAX_VALUE	1024

// BSP-30 files contain these lumps
#define	LUMP_ENTITIES	  0
#define	LUMP_PLANES		  1
#define	LUMP_TEXTURES	  2
#define	LUMP_VERTEXES	  3
#define	LUMP_VISIBILITY	  4
#define	LUMP_NODES		  5
#define	LUMP_TEXINFO	  6
#define	LUMP_FACES		  7
#define	LUMP_LIGHTING	  8
#define	LUMP_CLIPNODES	  9
#define	LUMP_LEAFS		  10
#define	LUMP_MARKSURFACES 11
#define	LUMP_EDGES		  12
#define	LUMP_SURFEDGES	  13
#define	LUMP_MODELS		  14
#define	HEADER_LUMPS	  15

// Leaf content values
#define	CONTENTS_EMPTY        -1
#define	CONTENTS_SOLID        -2
#define	CONTENTS_WATER        -3
#define	CONTENTS_SLIME        -4
#define	CONTENTS_LAVA         -5
#define	CONTENTS_SKY          -6
#define	CONTENTS_ORIGIN       -7
#define	CONTENTS_CLIP         -8
#define	CONTENTS_CURRENT_0    -9
#define	CONTENTS_CURRENT_90   -10
#define	CONTENTS_CURRENT_180  -11
#define	CONTENTS_CURRENT_270  -12
#define	CONTENTS_CURRENT_UP   -13
#define	CONTENTS_CURRENT_DOWN -14
#define	CONTENTS_TRANSLUCENT  -15

//Plane types
#define PLANE_X    0 // Plane is perpendicular to given axis
#define PLANE_Y    1
#define PLANE_Z    2
#define PLANE_ANYX 3 // Non-axial plane is snapped to the nearest
#define PLANE_ANYY 4
#define PLANE_ANYZ 5

// Render modes
#define RENDER_MODE_NORMAL   0
#define RENDER_MODE_COLOR	 1
#define RENDER_MODE_TEXTURE  2
#define RENDER_MODE_GLOW	 3
#define RENDER_MODE_SOLID	 4
#define RENDER_MODE_ADDITIVE 5

/**
 *  @brief Describes a lump in the BSP file
 *  To read the different lumps from the given BSP file, every lump entry file states the beginning of each lump as an offset relativly to the beginning of the file. Additionally, the lump entry also gives the length of the addressed lump in bytes.
 */
typedef struct _BSPLUMP
{
	int32_t nOffset; ///< File offset to data
	int32_t nLength; ///< Length of data
} BSPLUMP;

/**
 *  @brief The BSP file header
 *  The file header begins with an 32bit integer containing the file version of the BSP file (the magic number). This should be 30 for a valid BSP file used by the Half-Life Engine.
 *  Subseqently, there is an array of entries for the so-called lumps. A lump is more or less a section of the file containing a specific type of data. The lump entries in the file header address these lumps, accessed by the 15 predefined indexes.
 */
typedef struct _BSPHEADER
{
	int32_t nVersion;			///< Version number, must be 30 for a valid HL BSP file
	BSPLUMP lump[HEADER_LUMPS]; ///< Stores the directory of lumps.
} BSPHEADER;

/**
 *  @brief Describes a node of the BSP Tree
 *
 */
typedef struct _BSPNODE
{
	uint32_t iPlane;			 // Index into pPlanes lump
	int16_t  iChildren[2];		 // If > 0, then indices into Nodes otherwise bitwise inverse indices into Leafs
	int16_t  nMins[3], nMaxs[3]; // Defines bounding box
	uint16_t iFirstFace, nFaces;  // Index and count into BSPFACES array
} BSPNODE;

// Leafs lump contains leaf structures
typedef struct _BSPLEAF
{
	int32_t  nContents;			               // Contents enumeration, see #defines
	int32_t  nVisOffset;		               // Offset into the compressed visibility lump
	int16_t  nMins[3], nMaxs[3];               // Defines bounding box
	uint16_t iFirstMarkSurface, nMarkSurfaces; // Index and count into BSPMARKSURFACE array
	uint8_t  nAmbientLevels[4];	               // Ambient sound levels
} BSPLEAF;

// Leaves index into marksurfaces, which index into pFaces
typedef uint16_t BSPMARKSURFACE;

// Planes lump contains plane structures
typedef struct _BSPPLANE
{
	VECTOR3D vNormal; // The planes normal vector
	float    fDist;   // Plane equation is: vNormal * X = fDist
	int32_t  nType;   // Plane type, see #defines
} BSPPLANE;

// Vertex lump is an array of float triples (VECTOR3D)
typedef VECTOR3D BSPVERTEX;

// Edge struct contains the begining and end vertex for each edge
typedef struct _BSPEDGE
{
	uint16_t iVertex[2];		// Indices into vertex array
} BSPEDGE;

// Faces are equal to the polygons that make up the world
typedef struct _BSPFACE
{
    uint16_t iPlane;                // Index of the plane the face is parallel to
    uint16_t nPlaneSide;            // Set if different normals orientation
    uint32_t iFirstEdge;            // Index of the first edge (in the surfedge array)
    uint16_t nEdges;                // Number of consecutive surfedges
    uint16_t iTextureInfo;          // Index of the texture info structure
    uint8_t  nStyles[4];            // Specify lighting styles
    //       nStyles[0]             // type of lighting, for the face
    //       nStyles[1]             // from 0xFF (dark) to 0 (bright)
    //       nStyles[2], nStyles[3] // two additional light models
    uint32_t nLightmapOffset;    // Offsets into the raw lightmap data
} BSPFACE;

// Surfedges lump is array of signed int indices into edge lump, where a negative index indicates
// using the referenced edge in the opposite direction. Faces index into pSurfEdges, which index
// into pEdges, which finally index into pVertices.
typedef int32_t BSPSURFEDGE;

// Textures lump begins with a header, followed by offsets to BSPMIPTEX structures, then BSPMIPTEX structures
typedef struct _BSPTEXTUREHEADER
{
	uint32_t nMipTextures; // Number of BSPMIPTEX structures
} BSPTEXTUREHEADER;

// 32-bit offsets (within texture lump) to (nMipTextures) BSPMIPTEX structures
typedef int32_t BSPMIPTEXOFFSET;

// BSPMIPTEX structures which defines a Texture
#define MAXTEXTURENAME 16
#define	MIPLEVELS 4
typedef struct _BSPMIPTEX
{
	char     szName[MAXTEXTURENAME]; // Name of texture, for reference from external WAD file
	uint32_t nWidth, nHeight;        // Extends of the texture
	uint32_t nOffsets[MIPLEVELS];    // Offsets to MIPLEVELS texture mipmaps, if 0 texture data is stored in an external WAD file
} BSPMIPTEX;

// Texinfo lump contains texinfo structures
typedef struct _BSPTEXTUREINFO
{
	VECTOR3D vS;      // 1st row of texture matrix
	float    fSShift; // Texture shift in s direction
    VECTOR3D vT;      // 2nd row of texture matrix - multiply 1st and 2nd by vertex to get texture coordinates
    float    fTShift; // Texture shift in t direction
	uint32_t iMiptex; // Index into textures array
	uint32_t nFlags;  // Texture flags, seems to always be 0
} BSPTEXTUREINFO;

typedef struct _BSPMODEL
{
    float    nMins[3], nMaxs[3];        // Defines bounding box
    VECTOR3D vOrigin;                   // Coordinates to move the coordinate system before drawing the model
    int32_t  iHeadNodes[MAX_MAP_HULLS]; // Index into nodes array
    int32_t  nVisLeafs;                 // No idea
    int32_t  iFirstFace, nFaces;        // Index and count into face array
} BSPMODEL;

typedef struct _BSPCLIPNODE
{
    int32_t iPlane;       // Index into planes
    int16_t iChildren[2]; // negative numbers are contents behind and in front of the plane
} BSPCLIPNODE;

#endif // BSPV32FILEDEFS_H_INCLUDED
