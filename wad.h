#ifndef WAD_H_INCLUDED
#define WAD_H_INCLUDED

#include <stdio.h>
#include "imageio.h"
#include "bspdef.h"

typedef struct
{
    char    szMagic[4]; // should be WAD2/WAD3
    int32_t nDir;       // number of directory entries
    int32_t nDirOffset; // offset into directory
} WADHEADER;

// Directory entry structure
typedef struct _WADDIRENTRY
{
    int32_t nFilePos;               // offset in WAD
    int32_t nDiskSize;              // size in file
    int32_t nSize;                  // uncompressed size
    int8_t  nType;                  // type of entry
    bool    bCompression;           // 0 if none
    int16_t nDummy;                 // not used
    char    szName[MAXTEXTURENAME]; // must be null terminated
} WADDIRENTRY;

// Structure for holding data for mipmap textires
typedef struct _MIPTEXTURE
{
    IMAGE Img[MIPLEVELS]; // Stores MIPLEVELS images for textures
} MIPTEXTURE;

class CWAD
{
    public:
        CWAD();  // ctor
        ~CWAD(); // dtor

        bool Open(const char* pszFilename);                                           // Opens a WAD File and loads it's directory for texture searching
        MIPTEXTURE* LoadTexture(const char * pszTexName);                             // Finds and loads a texture by the given name
        MIPTEXTURE* LoadDecalTexture(const char* pszTexName);
        void CreateMipTexture(const unsigned char* pRawTexture, MIPTEXTURE* pMipTex); // Creates a Miptexture out of the raw texture data
        void Close();                                                                 // Closes the WAD file and frees all allocated memory

    private:
        FILE*        pWadFile;    // Pointer to the opened WAD file on the disk
        WADDIRENTRY* pDirEntries; // Pointer to an array of directory entries
        int          nDirEntries; // Count of the directory entries

        bool LoadDirectory();                  // Loads the directory of the WAD file for further texture finding
        int  FindTexture(const char* pszName); // Finds a texture in the WAD file and returns the directory index
        unsigned char* GetTexture(const char* pszTexName);
        void CreateDecalTexture(const unsigned char* pRawTexture, MIPTEXTURE* pMipTex);
        void ApplyAlphaSections(IMAGE* pTex);  // Sets the blue pixels to alpha and colors them a bit for later linear filtering
};

void FreeMipTexturePointer(MIPTEXTURE* pMipTex); // Frees the allocated memory of a pointer to a MIPTEXTURE structure
void FreeMipTexture(MIPTEXTURE MipTex);          // Frees the allocated memory of a MIPTEXTURE structure

#endif // WAD_H_INCLUDED
