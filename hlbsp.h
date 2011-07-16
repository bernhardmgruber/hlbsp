#ifndef BSPLOADER_H_INCLUDED
#define BSPLOADER_H_INCLUDED

//#include <windows.h>
#include <GL/gl.h>
#include "bspv30filedefs.h"
#include "entity.h"
#include "wad.h"

// Stores texture coordinates for each vertex of a face
typedef struct _BSPTEXCOORDS
{
    float fS; // S coordinate
    float fT; // T coordinate
} BSPTEXCOORDS;

// One for each face, holds texture and lightmap coordinates for each vertex
typedef struct _BSPFACETEXCOORDS
{
    BSPTEXCOORDS* pTexCoords;      // Stores texture coordinates
    BSPTEXCOORDS* pLightmapCoords; // Stores lightmap coordinates
} BSPFACETEXCOORDS;

typedef struct _DECAL
{
    GLuint nTex;
    VECTOR3D vNormal;
    VECTOR3D vec[4];
} DECAL;

class CBSP
{
public:

    CBSP();  //ctor
    ~CBSP(); //dtor

    bool LoadBSPFile(const char* pszFileName);                  // Loads the entire BSP file into memory

    CEntity* FindEntity(const char* pszNewClassName);        // Returns the entity with the given name

    /** Collision detection **/
    VECTOR3D Move(VECTOR3D vStart, VECTOR3D vEnd, int hull);
    //VECTOR3D TraceSphere(VECTOR3D vStart, VECTOR3D vEnd, float radius);
    //VECTOR3D TraceBox   (VECTOR3D vStart, VECTOR3D vEnd, VECTOR3D vMin, VECTOR3D vMax);

    /** Rendering **/
    void RenderLevel(VECTOR3D vPos);                      // Renders the complete BSP tree
    void RenderLeavesOutlines();
    void RenderLeafOutlines(int iLeaf);

    void Destroy();                                       // Unloads the complete BSP tree and frees all allocated memory

//private:
    int nNodes;           // Number of nodes
    int nLeafs;           // Number of leafs
    int nMarkSurfaces;    // Number of marksurfaces
    int nPlanes;          // Number of planes
    int nVertices;        // Number of vertices
    int nEdges;           // Number of edges
    int nFaces;           // Number of faces
    int nClipNodes;
    int nSurfEdges;       // Number of surface edges
    int nModels;          // Number of models
    int nTextureInfos;    // Number of texture infos
    int nEntities;        // Number of entities
    int nBrushEntities;   // Number of brush entities
    int nSpecialEntities; // Number of special entities
    int nWadFiles;        // Number of WAD files
    int nDecals;

    BSPHEADER         header;             // Stores the header
    BSPVERTEX*        pVertices;          // Stores the vertices
    BSPEDGE*          pEdges;             // Stores the edges
    BSPSURFEDGE*      pSurfEdges;         // Stores the surface edges
    BSPNODE*          pNodes;             // Stores the nodes
    BSPLEAF*          pLeafs;             // Stores the leafs
    BSPMARKSURFACE*   pMarkSurfaces;      // Stores the marksurfaces
    BSPPLANE*         pPlanes;            // Stores the planes
    BSPFACE*          pFaces;             // Stores the faces
    BSPCLIPNODE*      pClipNodes;
    BSPMODEL*         pModels;            // Stores the models
    BSPTEXTUREHEADER  textureHeader;      // Stores the texture header
    BSPMIPTEX*        pMipTextures;       // Stores the miptextures
    BSPMIPTEXOFFSET*  pMipTextureOffsets; // Stores the miptexture offsets
    BSPTEXTUREINFO*   pTextureInfos;      // Stores the texture infos
    BSPFACETEXCOORDS* pFaceTexCoords;     // Stores precalculated texture and lightmap coordinates for every vertex


    CEntity*                   pEntities;          // Stores the entities
    CEntity**                  ppBrushEntities;    // Pointers to brush entities in *pEntities
    CEntity**                  ppSpecialEntities;  // Pointers to special entities in *pEntities
    CWAD*                      pWadFiles;          // Wad files used for texture loading
    CWAD*                      pDecalWads;
    DECAL*                     pDecals;
    bool**                     ppbVisLists;        // Stores the vis lists for all faces
    int                        nVisLeafs;          // Number of Leafs the player can walk in (needs PVS)

    GLuint* pnLightmapLookUp; // Stores a lookup table where pFaces use their index to find the index of their lightmap texture
    GLuint* pnTextureLookUp;  // Stores a lookup table where pFaces use their index to find the index of their texture
    GLuint* pdlSkyBox;        // Pointer to a displaylist index which stores the skybox, if NULL, there is no skybox.
    bool*   pbFacesDrawn;     // Boolarray which avoids drawing pFaces twice by marking each drawn face's index in the array

    void AdjustTextureToPowerOfTwo(IMAGE* pImg);         // Checks whether or not a texture has power of two extends and scales it if neccessary
    bool LoadSkyTextures();                              // Loads the sky textures from disk and creates a display list for faster rendering
    bool LoadWadFiles(const char* pszWadstr);            // Loads and prepares the wad files for further texture loading
    void UnloadWadFiles();                               // Unloads all wad files and frees allocated memory
    void LoadTextures(FILE* pFile);                      // Loads the textures either from the wad file or directly from the bsp file
    MIPTEXTURE* LoadTextureFromWad(const char* pszName); // Finds and loads a texture from a wad file by the given name
    MIPTEXTURE* LoadDecalTexture(const char* pszName);
    void LoadDecals();
    void LoadLightMaps(unsigned char* pLightMapData);    // Loads lightmaps and calculates extends and coordinates

    void ParseEntities(const char* pszEntities); // Parses the entity lump of the bsp file into single entity classes
    bool IsBrushEntity(CEntity* pEnt);           // Checks if an entity is a valid brush entity (has a model)

    void CountVisLeafs(int iNode);                    // Counts the number of nVisLeafs recursively
    bool* GetPVS(int iLeaf, unsigned char* pVisList); // Get the PVS for a given leaf and return it in the form of a pointer to a bool array

    int TraverseBSPTree(VECTOR3D vPos, int iNode); // Recursivly walks through the BSP tree to find the leaf where the camera is in

	/** Collision detection **/
	VECTOR3D TryToStep(VECTOR3D vStart, VECTOR3D vEnd);
    VECTOR3D Trace(VECTOR3D vStart, VECTOR3D vEnd);
    int HullPointContents(int iNode, VECTOR3D p);
    bool RecursiveHullCheck (int iNode, float p1f, float p2f, VECTOR3D p1, VECTOR3D p2);

	/** Rendering **/
    void RenderSkybox(VECTOR3D vPos);                       // Calls the display list, which draws the skybox to the screen
    void RenderFace(int iFace);                             // Renders a face (polygon) by the given index
    void RenderLeaf(int iLeaf);                             // Renders a leaf of the BSP tree by rendering each face of the leaf by the given index
    void RenderBSP(int iNode, int iLeaf, VECTOR3D vPos);    // Recursively walks through the BSP tree and draws it
    void RenderBrushEntity(int iEntity, VECTOR3D vPos);     // Renders a brush entity by rendering each face of the associated model by the given index
    void RenderDecals();
};
#endif // BSPLOADER_H_INCLUDED
