#include "hlbsp.h"
#include "main.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>


#define WAD_DIR "data/wads"
#define SKY_DIR "data/textures/sky"

#define DECAL_WAD_COUNT 2

#define RENDER_MODE_NORMAL   0
#define RENDER_MODE_COLOR	1
#define RENDER_MODE_TEXTURE  2
#define RENDER_MODE_GLOW	 3
#define RENDER_MODE_SOLID	4
#define RENDER_MODE_ADDITIVE 5

/**
 *============================================================================================
 *									   PRIVATE
 *============================================================================================
**/

void CBSP::AdjustTextureToPowerOfTwo(IMAGE* pImg)
{
    if (g_bTexNPO2Support)
        return;

    if (((pImg->nWidth & (pImg->nWidth - 1)) == 0) && ((pImg->nHeight & (pImg->nHeight - 1)) == 0))
        return;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int nPOT = 1;
    while (nPOT < pImg->nHeight || nPOT < pImg->nWidth)
        nPOT *= 2;

    // Scale image
    unsigned char* pNewData = (unsigned char*) MALLOC(nPOT * nPOT * pImg->nChannels * sizeof(unsigned char));

    gluScaleImage(pImg->nChannels == 4 ? GL_RGBA : GL_RGB, pImg->nWidth, pImg->nHeight, GL_UNSIGNED_BYTE, pImg->pData, nPOT, nPOT, GL_UNSIGNED_BYTE, pNewData);

    free(pImg->pData);
    pImg->nWidth = nPOT;
    pImg->nHeight = nPOT;
    pImg->pData = pNewData;
}

bool CBSP::LoadSkyTextures()
{
    LOG("Loading sky textures ...\n");

    pdlSkyBox = NULL;

    const char* pszSkyName = FindEntity("worldspawn")->FindProperty("skyname");
    if (pszSkyName == NULL)
        return true; //we don't have a sky texture

    char aszSide[6][3] = {"ft", "bk", "rt", "lf", "up", "dn"};

    GLuint nSkyTex[6];

    glGenTextures(6, nSkyTex);

    char szFileName[64];
    IMAGE* pImg;

    for (int i=0; i<6; i++)
    {
        sprintf(szFileName, SKY_DIR "/%s%s.tga", pszSkyName, aszSide[i]);

        pImg = LoadTGA(szFileName);
        if (pImg == NULL)
        {
            MSGBOX_ERROR("Failed to load skytexture from file: %s", szFileName);
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, nSkyTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pImg->nWidth, pImg->nHeight, 0, pImg->nChannels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, pImg->pData);
    }

    //Create Displaylist
    pdlSkyBox = (GLuint*) MALLOC(sizeof(GLuint));

    *pdlSkyBox = glGenLists(1);
    glNewList(*pdlSkyBox, GL_COMPILE);



    //http://enter.diehlsworld.de/ogl/skyboxartikel/skybox.htm
    glDepthMask(0); // prevent writing depth coords

    float fAHalf = 100; //half length of the edge of the cube

    //front
    glBindTexture(GL_TEXTURE_2D, nSkyTex[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(fAHalf, -fAHalf, -fAHalf);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(fAHalf, -fAHalf, fAHalf);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-fAHalf, -fAHalf, fAHalf);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-fAHalf, -fAHalf, -fAHalf);
    glEnd();

    //back
    glBindTexture(GL_TEXTURE_2D, nSkyTex[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-fAHalf, fAHalf, -fAHalf);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-fAHalf, fAHalf, fAHalf);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(fAHalf, fAHalf, fAHalf);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(fAHalf, fAHalf, -fAHalf);
    glEnd();

    //right
    glBindTexture(GL_TEXTURE_2D, nSkyTex[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(fAHalf, fAHalf, -fAHalf);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(fAHalf, fAHalf, fAHalf);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(fAHalf, -fAHalf, fAHalf);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(fAHalf, -fAHalf, -fAHalf);
    glEnd();

    //left
    glBindTexture(GL_TEXTURE_2D, nSkyTex[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-fAHalf, -fAHalf, -fAHalf);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-fAHalf, -fAHalf, fAHalf);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-fAHalf, fAHalf, fAHalf);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-fAHalf, fAHalf, -fAHalf);
    glEnd();

    //up
    glBindTexture(GL_TEXTURE_2D, nSkyTex[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(fAHalf, fAHalf, fAHalf);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-fAHalf, fAHalf, fAHalf);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-fAHalf, -fAHalf, fAHalf);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(fAHalf, -fAHalf, fAHalf);
    glEnd();

    //down
    glBindTexture(GL_TEXTURE_2D, nSkyTex[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-fAHalf, fAHalf, -fAHalf);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(fAHalf, fAHalf, -fAHalf);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(fAHalf, -fAHalf, -fAHalf);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-fAHalf, -fAHalf, -fAHalf);
    glEnd();

    glDepthMask(1);

    glEndList();

    return true;
}

void strchrrp(char* str, char search, char replace)
{
    for(char* c=str; *c!=0; c++)
    {
        if(*c == search)
            *c = replace;
    }
}

bool CBSP::LoadWadFiles(const char* pszWadstr)
{
    nWadFiles = 0;
    for (unsigned int i=0; i<strlen(pszWadstr); i++)
    {
        if (pszWadstr[i] == ';')
            nWadFiles++;
    }

    pWadFiles = new CWAD[nWadFiles]; // new to execute ctor

    char* pszWadFiles = (char*) MALLOC((strlen(pszWadstr) + 1) * sizeof(char));
    strcpy(pszWadFiles, pszWadstr);
    strchrrp(pszWadFiles, '\\', '/');

    char* pch = strtok(pszWadFiles, ";");

    int nWadCount = 0;
    int nWadErrors = 0;

    while (pch != NULL)
    {
        char path[256];
        strcpy(path, WAD_DIR);

        bool bFirst = true;
        for(unsigned int i=strlen(pch)-1; i<strlen(pch); i--)
        {
            if(pch[i] == '/')
            {
                if(bFirst)
                    bFirst = false;
                else
                {
                    pch = pch + i;
                    break;
                }
            }
        }

        strcat(path, pch);

        if (!pWadFiles[nWadCount++].Open(path))
        {
            LOG("#%2d ERROR loading %s\n", nWadCount, path);
            nWadErrors++;
        }
        else
            LOG("#%2d Loaded %s\n", nWadCount, path);

        pch = strtok(NULL, ";");
    }

    free(pszWadFiles);

    LOG("Loaded %d WADs, %d failed ", nWadCount, nWadErrors);
    if (nWadErrors == 0)
        LOG("OK\n");
    else
        LOG("ERRORS\n");

    return true;
}

void CBSP::UnloadWadFiles()
{
    for (int i=0; i<nWadFiles; i++)
        pWadFiles[i].Close();

    delete[] pWadFiles;
}

void CBSP::LoadTextures(FILE* pFile)
{
    CEntity* worldSpawn = FindEntity("worldspawn");

    LOG("Loading WADs ...\n");
    LoadWadFiles(worldSpawn->FindProperty("wad"));

    LOG("Loading textures ...\n");
    // Find unbound texture slots
    glGenTextures(textureHeader.nMipTextures, (GLuint*)pnTextureLookUp);

    int nTexError = 0;

    for (unsigned int i=0; i<textureHeader.nMipTextures; i++)
    {
        if (pMipTextures[i].nOffsets[0] == 0) //miptexture is stored externally
        {
            MIPTEXTURE* pMipTex = LoadTextureFromWad(pMipTextures[i].szName);
            if (!pMipTex)
            {
                nTexError++;
                LOG("#%3d ERROR loading texture %s\n", i + 1, pMipTextures[i].szName);
                continue;
            }

            // Bind the texture
            glBindTexture(GL_TEXTURE_2D, pnTextureLookUp[i]);

            // Set up Texture Filtering Parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPLEVELS - 1);

            for (int j=0; j<MIPLEVELS; j++)
            {
                AdjustTextureToPowerOfTwo(&pMipTex->Img[j]);
                glTexImage2D(GL_TEXTURE_2D, j, GL_RGBA, pMipTex->Img[j].nWidth, pMipTex->Img[j].nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pMipTex->Img[j].pData);
            }

            FreeMipTexturePointer(pMipTex);

            //LOG("#%3d Loaded texture %15s from WAD file\n", i + 1, pMipTextures[i].szName);
        }
        else //Internal texture
        {
            int nDataSize = sizeof(unsigned char) * (pMipTextures[i].nOffsets[3] + (pMipTextures[i].nHeight / 8) * (pMipTextures[i].nWidth / 8) + 2 + 768);
            unsigned char* pImgData = (unsigned char*) MALLOC(nDataSize);

            fseek(pFile, header.lump[LUMP_TEXTURES].nOffset + pMipTextureOffsets[i], SEEK_SET);
            fread(pImgData, sizeof(unsigned char), nDataSize, pFile);

            MIPTEXTURE MipTex;

            CWAD wad;
            wad.CreateMipTexture(pImgData, &MipTex);

            free(pImgData);

            // Bind the texture
            glBindTexture(GL_TEXTURE_2D, pnTextureLookUp[i]);

            // Set up Texture Filtering Parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPLEVELS - 1);

            for (int j=0; j<MIPLEVELS; j++)
            {
                AdjustTextureToPowerOfTwo(&MipTex.Img[j]);
                glTexImage2D(GL_TEXTURE_2D, j, GL_RGBA, MipTex.Img[j].nWidth, MipTex.Img[j].nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, MipTex.Img[j].pData);
            }
            FreeMipTexture(MipTex);

            //LOG("#%3d Loaded texture %15s from bsp file\n", i + 1, pMipTextures[i].szName);
        }
    }

    UnloadWadFiles();

    LOG("Loaded %d textures, %d failed ", textureHeader.nMipTextures, nTexError);
    if (nTexError == 0)
        LOG("OK\n");
    else
        LOG("ERRORS\n");

    // Calculate Texture Coordinates
    pFaceTexCoords = (BSPFACETEXCOORDS*) MALLOC(nFaces * sizeof(BSPFACETEXCOORDS));
    for (int i=0; i<nFaces; ++i)
    {
        pFaceTexCoords[i].pTexCoords = (BSPTEXCOORDS*) MALLOC(pFaces[i].nEdges * sizeof(BSPTEXCOORDS));

        BSPTEXTUREINFO curTexInfo = pTextureInfos[pFaces[i].iTextureInfo];

        for (int j=0; j<pFaces[i].nEdges; j++)
        {
            int iEdge = pSurfEdges[pFaces[i].iFirstEdge+j]; // This gives the index into the edge lump

            if (iEdge > 0)
            {
                pFaceTexCoords[i].pTexCoords[j].fS = (DotProduct(pVertices[pEdges[iEdge].iVertex[0]], curTexInfo.vS) + curTexInfo.fSShift) / pMipTextures[curTexInfo.iMiptex].nWidth;
                pFaceTexCoords[i].pTexCoords[j].fT = (DotProduct(pVertices[pEdges[iEdge].iVertex[0]], curTexInfo.vT) + curTexInfo.fTShift) / pMipTextures[curTexInfo.iMiptex].nHeight;
            }
            else
            {
                iEdge *= -1;
                pFaceTexCoords[i].pTexCoords[j].fS = (DotProduct(pVertices[pEdges[iEdge].iVertex[1]], curTexInfo.vS) + curTexInfo.fSShift) / pMipTextures[curTexInfo.iMiptex].nWidth;
                pFaceTexCoords[i].pTexCoords[j].fT = (DotProduct(pVertices[pEdges[iEdge].iVertex[1]], curTexInfo.vT) + curTexInfo.fTShift) / pMipTextures[curTexInfo.iMiptex].nHeight;
            }
        }
    }
}

MIPTEXTURE* CBSP::LoadTextureFromWad(const char* pszName)
{
    for (int i=0; i<nWadFiles; i++)
    {
        MIPTEXTURE* pMipMapTex = pWadFiles[i].LoadTexture(pszName);
        if (pMipMapTex != NULL)
            return pMipMapTex;
    }

    return NULL;
}

MIPTEXTURE* CBSP::LoadDecalTexture(const char* pszName)
{
    for (int i=0; i<DECAL_WAD_COUNT; i++)
    {
        MIPTEXTURE* pMipMapTex = pDecalWads[i].LoadDecalTexture(pszName);
        if (pMipMapTex != NULL)
            return pMipMapTex;
    }

    return NULL;
}

void CBSP::LoadDecals()
{
    // Load Decal WADs
    pDecalWads = new CWAD[DECAL_WAD_COUNT];

    pDecalWads[0].Open(WAD_DIR "/valve/decals.wad");
    pDecalWads[1].Open(WAD_DIR "/cstrike/decals.wad");

    // Count decals
    nDecals = 0;

    CEntity* pEnt = FindEntity("infodecal");
    do
        nDecals++;
    while((pEnt = FindEntity(NULL)));

    // Texture name table for texture loading
    int nLoadedTex = 0;
    struct
    {
        char szName[MAXTEXTURENAME];
        GLuint texID;
        int nWidth;
        int nHeight;
    } aLoadedTex[nDecals];

    // Allocate new decals
    pDecals = (DECAL*) MALLOC(sizeof(DECAL) * nDecals);

    // Process each decal
    pEnt = FindEntity("infodecal");
    if(!pEnt)
    {
        // There are no decals
        printf("(no decals)\n");
        return;
    }

    for(int i=0; i<nDecals; i++)
    {
        const char* pszOrigin = pEnt->FindProperty("origin");
        if(pszOrigin != NULL)
        {
            int x, y, z;
            sscanf(pszOrigin, "%d %d %d", &x, &y, &z);

            VECTOR3D vOrigin;
            vOrigin.x = x;
            vOrigin.y = y;
            vOrigin.z = z;

            // Find leaf
            int iLeaf = TraverseBSPTree(vOrigin, 0);
            if(iLeaf == -1)
            {
                LOG("ERROR finding decal leaf\n");
                continue;
            }

            // Loop through each face in this leaf
            for (int j=0; j<pLeafs[iLeaf].nMarkSurfaces; j++)
            {
                // Find face
                int iFace = pMarkSurfaces[pLeafs[iLeaf].iFirstMarkSurface + j];

                // Find normal
                VECTOR3D normal = pPlanes[pFaces[iFace].iPlane].vNormal;

                // Find a vertex on the face
                VECTOR3D vertex;
                int iEdge = pSurfEdges[pFaces[iFace].iFirstEdge]; // This gives the index into the edge lump

                if (iEdge > 0)
                {
                    vertex = pVertices[pEdges[iEdge].iVertex[0]];
                }
                else
                {
                    iEdge *= -1;
                    vertex = pVertices[pEdges[iEdge].iVertex[1]];
                }

                // Check if decal origin is in this face
                if(PointInPlane(vOrigin, normal, DotProduct(normal, vertex)))
                {
                    // TEXTURE
                    GLuint texID = 0;
                    int width = 0;
                    int height = 0;

                    const char* pszTexName = pEnt->FindProperty("texture");
                    if(!pszTexName)
                    {
                        LOG("ERROR retrieving texture name from decal\n");
                        continue;
                    }

                    // Check if texture has already been loaded
                    for(int k=0; k<nLoadedTex; k++)
                    {
                        if(!strcmp(pszTexName, aLoadedTex[k].szName))
                        {
                            // Found already loaded texture
                            texID = aLoadedTex[k].texID;
                            width = aLoadedTex[k].nWidth;
                            height = aLoadedTex[k].nHeight;
                            break;
                        }
                    }

                    if(!texID)
                    {
                        // Load new texture
                        MIPTEXTURE* pMipTex = LoadDecalTexture(pszTexName);
                        if (!pMipTex)
                        {
                            LOG("#%3d ERROR loading texture %s\n", i + 1, pszTexName);
                            continue;
                        }

                        glGenTextures(1, &texID);

                        // Bind the texture
                        glBindTexture(GL_TEXTURE_2D, texID);

                        // Set up Texture Filtering Parameters
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPLEVELS - 1);

                        for (int k=0; k<MIPLEVELS; k++)
                        {
                            AdjustTextureToPowerOfTwo(&pMipTex->Img[k]);
                            glTexImage2D(GL_TEXTURE_2D, k, GL_RGBA, pMipTex->Img[k].nWidth, pMipTex->Img[k].nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pMipTex->Img[k].pData);
                        }

                        // Decal size
                        width = pMipTex->Img[0].nWidth;
                        height = pMipTex->Img[0].nHeight;

                        FreeMipTexturePointer(pMipTex);

                        // Add to loaded textures
                        strcpy(aLoadedTex[nLoadedTex].szName, pszTexName);
                        aLoadedTex[nLoadedTex].texID = texID;
                        aLoadedTex[nLoadedTex].nWidth = width;
                        aLoadedTex[nLoadedTex].nHeight = height;
                        nLoadedTex++;

                        //LOG("#%3d Loaded texture %15s from WAD file\n", i + 1, pszTexName);
                    }

                    int h2 = height / 2;
                    int w2 = width / 2;

                    VECTOR3D vS = pTextureInfos[pFaces[iFace].iTextureInfo].vS;
                    VECTOR3D vT = pTextureInfos[pFaces[iFace].iTextureInfo].vT;

                    pDecals[i].vNormal = normal;
                    pDecals[i].nTex = texID;

                    pDecals[i].vec[0] = vOrigin - vT * h2 - vS * w2;
                    pDecals[i].vec[1] = vOrigin - vT * h2 + vS * w2;
                    pDecals[i].vec[2] = vOrigin + vT * h2 + vS * w2;
                    pDecals[i].vec[3] = vOrigin + vT * h2 - vS * w2;

                    break;
                }
            }

        }

        pEnt = FindEntity(NULL);
    }

    LOG("Loaded %d decals, %d decal textures\n", nDecals, nLoadedTex);
}

void CBSP::LoadLightMaps(unsigned char* pLightMapData)
{
    int nLoadedData = 0;
    int nLoadedLightmaps = 0;
    //int nErrors = 0;

    for (int i=0; i<nFaces; i++)
    {
        if (pFaces[i].nStyles[0] == 0 && (signed)pFaces[i].nLightmapOffset >= -1)
        {
            //Allocate pLightmapCoords
            pFaceTexCoords[i].pLightmapCoords = (BSPTEXCOORDS*) MALLOC(sizeof(BSPTEXCOORDS) * pFaces[i].nEdges);

            /************ QRAD ***********/

            float fMinU = 999999;
            float fMinV = 999999;
            float fMaxU = -99999;
            float fMaxV = -99999;

            BSPTEXTUREINFO* pTexInfo = &pTextureInfos[pFaces[i].iTextureInfo];
            for (int j=0; j<pFaces[i].nEdges; j++)
            {
                int iEdge = pSurfEdges[pFaces[i].iFirstEdge+j];
                BSPVERTEX* pVertex;
                if (iEdge >= 0)
                    pVertex = &pVertices[pEdges[iEdge].iVertex[0]];
                else
                    pVertex = &pVertices[pEdges[-iEdge].iVertex[1]];

                float fU = DotProduct(pTexInfo->vS, *pVertex) + pTexInfo->fSShift;
                if (fU < fMinU)
                    fMinU = fU;
                if (fU > fMaxU)
                    fMaxU = fU;

                float fV = DotProduct(pTexInfo->vT, *pVertex) + pTexInfo->fTShift;
                if (fV < fMinV)
                    fMinV = fV;
                if (fV > fMaxV)
                    fMaxV = fV;
            }

            float fTexMinU = (float)floor(fMinU / 16.0f);
            float fTexMinV = (float)floor(fMinV / 16.0f);
            float fTexMaxU = (float)ceil(fMaxU / 16.0f);
            float fTexMaxV = (float)ceil(fMaxV / 16.0f);

            int nWidth = (int)(fTexMaxU - fTexMinU) + 1;
            int nHeight = (int)(fTexMaxV - fTexMinV) + 1;

            /************ end QRAD ***********/

            /*********** http://www.gamedev.net/community/forums/topic.asp?topic_id=538713 (last refresh: 20.02.2010) ***********/

            float fMidPolyU = (fMinU + fMaxU) / 2.0;
            float fMidPolyV = (fMinV + fMaxV) / 2.0;
            float fMidTexU = (float)(nWidth) / 2.0;
            float fMidTexV = (float)(nHeight) / 2.0;

            for (int j=0; j<pFaces[i].nEdges; ++j)
            {
                int iEdge = pSurfEdges[pFaces[i].iFirstEdge+j];
                BSPVERTEX* pVertex;
                if (iEdge >= 0)
                    pVertex = pVertices + pEdges[iEdge].iVertex[0];
                else
                    pVertex = pVertices + pEdges[-iEdge].iVertex[1];

                float fU = DotProduct(pTexInfo->vS, *pVertex) + pTexInfo->fSShift;
                float fV = DotProduct(pTexInfo->vT, *pVertex) + pTexInfo->fTShift;

                float fLightMapU = fMidTexU + (fU - fMidPolyU) / 16.0f;
                float fLightMapV = fMidTexV + (fV - fMidPolyV) / 16.0f;

                pFaceTexCoords[i].pLightmapCoords[j].fS = fLightMapU / (float)nWidth;
                pFaceTexCoords[i].pLightmapCoords[j].fT = fLightMapV / (float)nHeight;
            }

            /*********** end http://www.gamedev.net/community/forums/topic.asp?topic_id=538713 ***********/

            // Find unbound texture slots
            glGenTextures(1, &pnLightmapLookUp[i]);

            IMAGE* pImg = CreateImage(3, nWidth, nHeight);
            memcpy(pImg->pData, &pLightMapData[pFaces[i].nLightmapOffset], nWidth * nHeight * 3 * sizeof(unsigned char));

            AdjustTextureToPowerOfTwo(pImg);

            // Bind the texture
            glBindTexture(GL_TEXTURE_2D, pnLightmapLookUp[i]);

            // Set up Texture Filtering Parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pImg->nWidth, pImg->nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pImg->pData);

            FreeImagePointer(pImg);

            nLoadedLightmaps++;
            //LOG("#%4d Loaded lightmap %2d x %2d\n", nLoadedLightmaps, nWidth, nHeight);

            //lm data diff
            /*unsigned int nNextOffset = 0xFFFFFFFF;
            for (int j=0;j<nFaces;j++)
            {
            	if (pFaces[j].nLightmapOffset > pFaces[i].nLightmapOffset && pFaces[j].nLightmapOffset < nNextOffset)
            		nNextOffset = pFaces[j].nLightmapOffset;
            }

            if (nNextOffset == 0xFFFFFFFF)
            	nNextOffset = header.lump[LUMP_LIGHTING].nLength;

            int nDataDiff = nWidth * nHeight * 3 - (nNextOffset - pFaces[i].nLightmapOffset);

            LOG("DataDiff: %d ", nDataDiff);

            if (nDataDiff == 0)
            	LOG("OK\n");
            else
            {
            	LOG("ERROR\n");
            	nErrors++;
            }*/

            nLoadedData += nWidth * nHeight * 3;
        }
        else
        {
            pnLightmapLookUp[i] = 0;
        }
    }
    //LOG("Loaded %d lightmaps, %d Errors, lightmapdatadiff: %d Bytes ", nFaces, nErrors, nLoadedData - header.lump[LUMP_LIGHTING].nLength);
    LOG("Loaded %d lightmaps, lightmapdatadiff: %d Bytes ", nLoadedLightmaps, nLoadedData - header.lump[LUMP_LIGHTING].nLength);
    //if (((nLoadedData - header.lump[LUMP_LIGHTING].nLength) == 0) && (nErrors == 0))
    if ((nLoadedData - header.lump[LUMP_LIGHTING].nLength) == 0)
        LOG("OK\n");
    else
        LOG("ERRORS\n");
}

// Compare function for brush entity sorting
int BrushEntityCmp(const void* a, const void* b)
{
    const char* szRenderMode1 = (*((CEntity**)(a)))->FindProperty("rendermode");
    const char* szRenderMode2 = (*((CEntity**)(b)))->FindProperty("rendermode");

    unsigned char nRenderMode1;
    unsigned char nRenderMode2;

    if(szRenderMode1)
        nRenderMode1 = atoi(szRenderMode1);
    else
        nRenderMode1 = RENDER_MODE_NORMAL;

    if(szRenderMode2)
        nRenderMode2 = atoi(szRenderMode2);
    else
        nRenderMode2 = RENDER_MODE_NORMAL;

    if (nRenderMode1 == RENDER_MODE_TEXTURE)
        return 1;

    if (nRenderMode2 == RENDER_MODE_TEXTURE)
        return -1;

    return 0;
}

void CBSP::ParseEntities(const char* pszEntities)
{
    // Count entities
    char* pchPos = (char*) pszEntities;
    nEntities = 0;

    while (true)
    {
        pchPos = strchr(pchPos, '{');
        if (pchPos == NULL)
            break;
        else
        {
            nEntities++;
            pchPos++;
        }
    }

    // Allocate memory for the pEntities
    pEntities = new CEntity[nEntities]; // new to execute ctor

    nBrushEntities = 0;
    nSpecialEntities = 0;

    // Start over
    pchPos = (char*) pszEntities;
    // Loop for each entity and parse it. count number of solid and special pEntities
    for (int i=0; i<nEntities; i++)
    {
        pchPos = strchr(pchPos, '{');
        char* pchClose = strchr(pchPos, '}');

        int nLen = pchClose - pchPos - 1;
        char* szIndividualEntity = (char*) MALLOC(sizeof(char) * (nLen + 1));
        strncpy(szIndividualEntity, pchPos + 1, nLen);
        szIndividualEntity[nLen] = 0;

        pEntities[i].ParseProperties(szIndividualEntity);

        free(szIndividualEntity);

        if (IsBrushEntity(&pEntities[i]))
            nBrushEntities++;
        else
            nSpecialEntities++;

        pchPos++;
    }

    ppBrushEntities = (CEntity**) MALLOC(nBrushEntities * sizeof(CEntity*));
    ppSpecialEntities = (CEntity**) MALLOC(nSpecialEntities * sizeof(CEntity*));

    int iBrush = 0;
    int iSpecial = 0;
    for (int i=0; i<nEntities; i++)
    {
        if (IsBrushEntity(&pEntities[i]))
        {
            ppBrushEntities[iBrush] = &pEntities[i];

            //if CEntity has property "origin" apply to model struct for rendering
            const char* szOrigin;
            if ((szOrigin = pEntities[i].FindProperty("origin")) != NULL)
            {
                int iModel = atoi(&pEntities[i].FindProperty("model")[1]);
                sscanf(szOrigin, "%f %f %f", &pModels[iModel].vOrigin.x, &pModels[iModel].vOrigin.y, &pModels[iModel].vOrigin.z);
            }

            iBrush++;
        }
        else
        {
            ppSpecialEntities[iSpecial] = &pEntities[i];
            iSpecial++;
        }
    }

    // sort brush entities so that these with rendermode texture are at the back
    qsort(ppBrushEntities, nBrushEntities, sizeof(CEntity*), BrushEntityCmp);
}

bool CBSP::IsBrushEntity(CEntity* pEnt)
{
    if (pEnt->FindProperty("model") == NULL)
        return false;

    const char* pzsClassName = pEnt->FindProperty("classname");
    if (!strcmp(pzsClassName, "func_door_rotating") ||
            !strcmp(pzsClassName, "func_door") ||
            !strcmp(pzsClassName, "func_illusionary") ||
            !strcmp(pzsClassName, "func_wall") ||
            !strcmp(pzsClassName, "func_breakable") ||
            !strcmp(pzsClassName, "func_button"))
        return true;
    else
        return false;
}

void CBSP::CountVisLeafs(int iNode)
{
    if (iNode < 0)
    {
        // decision node
        if(iNode == -1)
            return;

        if(pLeafs[~iNode].nContents == CONTENTS_SOLID)
            return;

        nVisLeafs++;
        return;
    }

    CountVisLeafs(pNodes[iNode].iChildren[0]);
    CountVisLeafs(pNodes[iNode].iChildren[1]);

    return;
}

bool* CBSP::GetPVS(int iLeaf, unsigned char* pVisList)
{
    bool* pbPVS = (bool*) MALLOC((nLeafs - 1) * sizeof(bool));

    memset(pbPVS, false, (nLeafs - 1) * sizeof(bool));

    unsigned char* pCurVisList = &pVisList[pLeafs[iLeaf].nVisOffset]; // Pointer to the begin of the current vis list

    bool* pbWriter = pbPVS; // Pointer that moves through the destination bool array (pbPVS)

    for (unsigned int iCurByte = 0; pbWriter - pbPVS < nVisLeafs; iCurByte++)
    {
        // Check for a run of 0s
        if (pCurVisList[iCurByte] == 0)
        {
            // Advance past this run of 0s
            iCurByte++;
            // Move the write pointer the number of compressed 0s
            pbWriter += 8 * pCurVisList[iCurByte];
        }
        else
        {
            // Iterate through this byte with bit shifting till the one of the bit has moved beyond the 8th digit (bit == 0)
            for (unsigned char bit = 1; bit != 0; pbWriter++, bit <<= 1)
            {
                // Test a bit of the compressed PVS with the bit mask
                if ((pCurVisList[iCurByte] & bit) && (pbWriter - pbPVS < nLeafs))
                {
                    *pbWriter = true;
                }
            }
        }
    }

    return pbPVS;
}

int CBSP::TraverseBSPTree(VECTOR3D vPos, int iNode)
{
    // Run once for each child
    for (int i=0; i<2; i++)
    {
        // If the index is positive  it is an index into the nodes array
        if ((pNodes[iNode].iChildren[i]) >= 0)
        {
            if(PointInBox(vPos, pNodes[pNodes[iNode].iChildren[i]].nMins, pNodes[pNodes[iNode].iChildren[i]].nMaxs))
                return TraverseBSPTree(vPos, pNodes[iNode].iChildren[i]);
        }
        // Else, bitwise inversed, it is an index into the leaf array
        // Do not test solid leaf 0
        else if (~pNodes[iNode].iChildren[i] != 0)
        {
            if(PointInBox(vPos, pLeafs[~(pNodes[iNode].iChildren[i])].nMins, pLeafs[~(pNodes[iNode].iChildren[i])].nMaxs))
                return ~(pNodes[iNode].iChildren[i]);
        }
    }

    return -1;
}

void CBSP::RenderSkybox(VECTOR3D vPos)
{
    glPushMatrix();
    glTranslatef(vPos.x, vPos.y, vPos.z);

    glCallList(*pdlSkyBox);

    glPopMatrix();
}

void CBSP::RenderFace(int iFace)
{
    if (pbFacesDrawn[iFace])
        return;
    else
        pbFacesDrawn[iFace] = true;

    if (pFaces[iFace].nStyles[0] == 0xFF)
        return;

    // if the light map offset is not -1 and the lightmap lump is not empty, there are lightmaps
    bool bLightmapAvail = (signed)pFaces[iFace].nLightmapOffset != -1 && header.lump[LUMP_LIGHTING].nLength > 0;

    if (bLightmapAvail && g_bLightmaps && g_bTextures)
    {
        // We need both texture units for textures and lightmaps

        // base texture
        glActiveTexture(GL_TEXTURE0_ARB);
        glBindTexture(GL_TEXTURE_2D, pnTextureLookUp[pTextureInfos[pFaces[iFace].iTextureInfo].iMiptex]);

        // light map
        glActiveTexture(GL_TEXTURE1_ARB);
        glBindTexture(GL_TEXTURE_2D, pnLightmapLookUp[iFace]);

        glBegin(GL_TRIANGLE_FAN);
        for (int i=0; i<pFaces[iFace].nEdges; i++)
        {
            glMultiTexCoord2f(GL_TEXTURE0_ARB, pFaceTexCoords[iFace].pTexCoords[i].fS, pFaceTexCoords[iFace].pTexCoords[i].fT);
            glMultiTexCoord2f(GL_TEXTURE1_ARB, pFaceTexCoords[iFace].pLightmapCoords[i].fS, pFaceTexCoords[iFace].pLightmapCoords[i].fT);

            // normal
            VECTOR3D vNormal = pPlanes[pFaces[iFace].iPlane].vNormal;
            if (pFaces[iFace].nPlaneSide)
                vNormal = vNormal * -1;
            glNormal3f(vNormal.x, vNormal.y, vNormal.z);

            int iEdge = pSurfEdges[pFaces[iFace].iFirstEdge + i]; // This gives the index into the edge lump

            if (iEdge > 0)
            {
                glVertex3f(pVertices[pEdges[iEdge].iVertex[0]].x, pVertices[pEdges[iEdge].iVertex[0]].y, pVertices[pEdges[iEdge].iVertex[0]].z);
            }
            else
            {
                iEdge *= -1;
                glVertex3f(pVertices[pEdges[iEdge].iVertex[1]].x, pVertices[pEdges[iEdge].iVertex[1]].y, pVertices[pEdges[iEdge].iVertex[1]].z);
            }
        }
        glEnd();
    }
    else
    {
        // We need one texture unit for either textures or lightmaps
        glActiveTexture(GL_TEXTURE0_ARB);

        if(g_bLightmaps)
            glBindTexture(GL_TEXTURE_2D, pnLightmapLookUp[iFace]);
        else
            glBindTexture(GL_TEXTURE_2D, pnTextureLookUp[pTextureInfos[pFaces[iFace].iTextureInfo].iMiptex]);

        glBegin(GL_TRIANGLE_FAN);
        for (int i=0; i<pFaces[iFace].nEdges; i++)
        {
            if(g_bLightmaps)
                glTexCoord2f(pFaceTexCoords[iFace].pLightmapCoords[i].fS, pFaceTexCoords[iFace].pLightmapCoords[i].fT);
            else
                glTexCoord2f(pFaceTexCoords[iFace].pTexCoords[i].fS, pFaceTexCoords[iFace].pTexCoords[i].fT);

            // normal
            VECTOR3D vNormal = pPlanes[pFaces[iFace].iPlane].vNormal;
            if (pFaces[iFace].nPlaneSide)
                vNormal = vNormal * -1;
            glNormal3f(vNormal.x, vNormal.y, vNormal.z);

            int iEdge = pSurfEdges[pFaces[iFace].iFirstEdge + i]; // This gives the index into the edge lump

            if (iEdge > 0)
            {
                glVertex3f(pVertices[pEdges[iEdge].iVertex[0]].x, pVertices[pEdges[iEdge].iVertex[0]].y, pVertices[pEdges[iEdge].iVertex[0]].z);
            }
            else
            {
                iEdge *= -1;
                glVertex3f(pVertices[pEdges[iEdge].iVertex[1]].x, pVertices[pEdges[iEdge].iVertex[1]].y, pVertices[pEdges[iEdge].iVertex[1]].z);
            }
        }
        glEnd();

        // Debug edge direction
        /*glPointSize(3.0f);
        glBegin(GL_POLYGON);
        for (int i=0; i<pFaces[iFace].nEdges; i++)
        {
        	VECTOR3D v;
        	int iEdge = pSurfEdges[pFaces[iFace].iFirstEdge + i]; // This gives the index into the edge lump
        	if(iEdge > 0)
        		v = pVertices[pEdges[iEdge].iVertex[0]] +
        					 0.75 * (pVertices[pEdges[iEdge].iVertex[1]] - pVertices[pEdges[iEdge].iVertex[0]]); // From [0] to [1] at 0.75
        	else
        	{
        		iEdge *= -1;
        		v = pVertices[pEdges[iEdge].iVertex[1]] +
        					 0.75 * (pVertices[pEdges[iEdge].iVertex[0]] - pVertices[pEdges[iEdge].iVertex[1]]); // From [0] to [1] at 0.75
        	}

        	glVertex3f(v.x, v.y,v.z);
        }
        glEnd();*/
    }
}

void CBSP::RenderLeaf(int iLeaf)
{
    // Loop through each face in this leaf
    for (int i=0; i<pLeafs[iLeaf].nMarkSurfaces; i++)
        RenderFace(pMarkSurfaces[pLeafs[iLeaf].iFirstMarkSurface + i]);
}

void CBSP::RenderBSP(int iNode, int iCurrentLeaf, VECTOR3D vPos)
{
    if (iNode < 0)
    {
        if (iNode == -1)
            return;

        if (iCurrentLeaf > 0)
            if (header.lump[LUMP_VISIBILITY].nLength != 0 && ppbVisLists != NULL && ppbVisLists[iCurrentLeaf - 1] != NULL && !ppbVisLists[iCurrentLeaf - 1][~iNode - 1])
                return;

        RenderLeaf(~iNode);

        return;
    }

    float location;

    switch (pPlanes[pNodes[iNode].iPlane].nType)
    {
    case PLANE_X:
        location = vPos.x - pPlanes[pNodes[iNode].iPlane].fDist;
    case PLANE_Y:
        location = vPos.y - pPlanes[pNodes[iNode].iPlane].fDist;
    case PLANE_Z:
        location = vPos.z - pPlanes[pNodes[iNode].iPlane].fDist;
    default:
        location = DotProduct(pPlanes[pNodes[iNode].iPlane].vNormal, vPos) - pPlanes[pNodes[iNode].iPlane].fDist;
    }

    if (location > 0.0f)
    {
        RenderBSP(pNodes[iNode].iChildren[1], iCurrentLeaf, vPos);
        RenderBSP(pNodes[iNode].iChildren[0], iCurrentLeaf, vPos);
    }
    else
    {
        RenderBSP(pNodes[iNode].iChildren[0], iCurrentLeaf, vPos);
        RenderBSP(pNodes[iNode].iChildren[1], iCurrentLeaf, vPos);
    }
}

void CBSP::RenderBrushEntity(int iEntity, VECTOR3D vPos)
{
    CEntity* pCurEnt = ppBrushEntities[iEntity];

    // Model
    int iModel = atoi(pCurEnt->FindProperty("model") + 1);

    // Alpha value
    unsigned char nAlpha;
    const char* pszRenderAmt = pCurEnt->FindProperty("renderamt");
    if(pszRenderAmt != NULL)
        nAlpha = atoi(pszRenderAmt);
    else
        nAlpha = 255;

    // Rendermode
    unsigned char nRenderMode;
    const char* pszRenderMode = pCurEnt->FindProperty("rendermode");
    if(pszRenderMode != NULL)
        nRenderMode = atoi(pszRenderMode);
    else
        nRenderMode = RENDER_MODE_NORMAL;

    glPushMatrix();
    glTranslatef(pModels[iModel].vOrigin.x, pModels[iModel].vOrigin.y, pModels[iModel].vOrigin.z);

    switch (nRenderMode)
    {
    case RENDER_MODE_NORMAL:
        break;
    case RENDER_MODE_TEXTURE:
        glColor4f(1.0f, 1.0f, 1.0f, (float)nAlpha / 255.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(false);

        glActiveTexture(GL_TEXTURE0_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        break;
    case RENDER_MODE_SOLID:
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.25);
        break;
    case RENDER_MODE_ADDITIVE:
        glColor4f(1.0f, 1.0f, 1.0f, (float)nAlpha / 255.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(false);

        glActiveTexture(GL_TEXTURE0_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        break;
    }

    RenderBSP(pModels[iModel].iHeadNodes[0], -1, vPos);

    switch (nRenderMode)
    {
    case RENDER_MODE_NORMAL:
        break;
    case RENDER_MODE_TEXTURE:
    case RENDER_MODE_ADDITIVE:
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glDisable(GL_BLEND);
        glDepthMask(true);

        glActiveTexture(GL_TEXTURE0_ARB);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        break;
    case RENDER_MODE_SOLID:
        glDisable(GL_ALPHA_TEST);
        break;
    }

    glPopMatrix();
}

void CBSP::RenderDecals()
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0.0f,-2.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(int i=0; i<nDecals; i++)
    {
        glBindTexture(GL_TEXTURE_2D, pDecals[i].nTex);

        glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(0,0);
        glNormal3f(pDecals[i].vNormal.x, pDecals[i].vNormal.y, pDecals[i].vNormal.z);
        glVertex3f(pDecals[i].vec[0].x, pDecals[i].vec[0].y, pDecals[i].vec[0].z);
        glTexCoord2f(1,0);
        glNormal3f(pDecals[i].vNormal.x, pDecals[i].vNormal.y, pDecals[i].vNormal.z);
        glVertex3f(pDecals[i].vec[1].x, pDecals[i].vec[1].y, pDecals[i].vec[1].z);
        glTexCoord2f(1,1);
        glNormal3f(pDecals[i].vNormal.x, pDecals[i].vNormal.y, pDecals[i].vNormal.z);
        glVertex3f(pDecals[i].vec[2].x, pDecals[i].vec[2].y, pDecals[i].vec[2].z);
        glTexCoord2f(0,1);
        glNormal3f(pDecals[i].vNormal.x, pDecals[i].vNormal.y, pDecals[i].vNormal.z);
        glVertex3f(pDecals[i].vec[3].x, pDecals[i].vec[3].y, pDecals[i].vec[3].z);
        glEnd();
    }

    glDisable(GL_BLEND);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

/**
 *============================================================================================
 *										PUBLIC
 *============================================================================================
**/

CBSP::CBSP()
{
    pVertices = NULL;
    pEdges = NULL;
    pSurfEdges = NULL;
    pNodes = NULL;
    pLeafs = NULL;
    pMarkSurfaces = NULL;
    pPlanes = NULL;
    pFaces = NULL;
    pModels = NULL;
    pMipTextures = NULL;
    pMipTextureOffsets = NULL;
    pTextureInfos = NULL;
    pFaceTexCoords = NULL;

    pEntities = NULL;
    ppBrushEntities = NULL;
    ppSpecialEntities = NULL;
    pWadFiles = NULL;
    ppbVisLists = NULL;

    pnLightmapLookUp = NULL;
    pnTextureLookUp = NULL;
    pdlSkyBox = NULL;
    pbFacesDrawn = NULL;
}

CBSP::~CBSP()
{
    Destroy();
}

bool CBSP::LoadBSPFile(const char* pszFileName)
{
    LOG("LOADING BSP FILE: %s\n", strrchr(pszFileName, '/') + 1);

    FILE* pfile = fopen(pszFileName, "rb");
    if (pfile == NULL)
    {
        MSGBOX_ERROR("Map file %s not found", pszFileName);
        return false;
    }

    // Read in the header
    fread(&header, sizeof(BSPHEADER), 1, pfile);

    if (header.nVersion != 30)
    {
        MSGBOX_ERROR("Invalid BSP version (%d instead of 30)", header.nVersion);
        return false;
    }

    // =================================================================
    // Get number of elements and allocate memory for them
    // =================================================================

    // Get the number of pNodes and allocate memory
    nNodes = header.lump[LUMP_NODES].nLength / sizeof(BSPNODE);
    pNodes = (BSPNODE*) MALLOC(nNodes * sizeof(BSPNODE));

    // Get the number of pLeafs and allocate memory
    nLeafs = header.lump[LUMP_LEAFS].nLength / sizeof(BSPLEAF);
    pLeafs = (BSPLEAF*) MALLOC(nLeafs * sizeof(BSPLEAF));

    // Get the number of marksurfaces and allocate memory
    nMarkSurfaces = header.lump[LUMP_MARKSURFACES].nLength / sizeof(BSPMARKSURFACE);
    pMarkSurfaces = (BSPMARKSURFACE*) MALLOC(nMarkSurfaces * sizeof(BSPMARKSURFACE));

    // Get the number of pFaces and allocate memory
    nFaces = header.lump[LUMP_FACES].nLength / sizeof(BSPFACE);
    pFaces = (BSPFACE*) MALLOC(nFaces * sizeof(BSPFACE));

    nClipNodes = header.lump[LUMP_CLIPNODES].nLength / sizeof(BSPCLIPNODE);
    pClipNodes = (BSPCLIPNODE*) MALLOC(nClipNodes * sizeof(BSPCLIPNODE));

    // Get the number of surface pEdges and allocate memory
    nSurfEdges = header.lump[LUMP_SURFEDGES].nLength / sizeof(BSPSURFEDGE);
    pSurfEdges = (BSPSURFEDGE*) MALLOC(nSurfEdges * sizeof(BSPSURFEDGE));

    // Get the number of pEdges and allocate memory
    nEdges = header.lump[LUMP_EDGES].nLength / sizeof(BSPEDGE);
    pEdges = (BSPEDGE*) MALLOC(nEdges * sizeof(BSPEDGE));

    // Get the number of pVertices and allocate memory
    nVertices = header.lump[LUMP_VERTEXES].nLength / sizeof(BSPVERTEX);
    pVertices = (BSPVERTEX*) MALLOC (nVertices * sizeof(BSPVERTEX));

    // Get the number of pPlanes and allocate memory
    nPlanes = header.lump[LUMP_PLANES].nLength / sizeof(BSPPLANE);
    pPlanes = (BSPPLANE*) MALLOC(nPlanes * sizeof(BSPPLANE));

    // Get the number of pModels and allocate memory
    nModels = header.lump[LUMP_MODELS].nLength / sizeof(BSPMODEL);
    pModels = (BSPMODEL*) MALLOC(nModels * sizeof(BSPMODEL));

    // =================================================================
    // Seek to and read in the data
    // =================================================================

    // Seek to the position in the file that stores the node information
    fseek(pfile, header.lump[LUMP_NODES].nOffset, SEEK_SET);
    // Read in the pNodes
    fread(pNodes, sizeof(BSPNODE), nNodes, pfile);

    // Seek to the position in the file that stores the leaf information
    fseek(pfile, header.lump[LUMP_LEAFS].nOffset, SEEK_SET);
    // Read in the pLeafs
    fread(pLeafs, sizeof(BSPLEAF), nLeafs, pfile);

    // Seek to the position in the file that stores the leaf information
    fseek(pfile, header.lump[LUMP_MARKSURFACES].nOffset, SEEK_SET);
    // Read in the pLeafs
    fread(pMarkSurfaces, sizeof(BSPMARKSURFACE), nMarkSurfaces, pfile);

    // Seek to the position in the file that stores the face information
    fseek(pfile, header.lump[LUMP_FACES].nOffset, SEEK_SET);
    // Read in the pFaces
    fread(pFaces, sizeof(BSPFACE), nFaces, pfile);

    // Seek to the position in the file that stores the clipnodes information
    fseek(pfile, header.lump[LUMP_CLIPNODES].nOffset, SEEK_SET);
    // Read in the clipnodes
    fread(pClipNodes, sizeof(BSPCLIPNODE), nClipNodes, pfile);

    // Seek to the position in the file that stores the surface edge information
    fseek(pfile, header.lump[LUMP_SURFEDGES].nOffset, SEEK_SET);
    // Read in the surface pEdges
    fread(pSurfEdges, sizeof(BSPSURFEDGE), nSurfEdges, pfile);

    // Seek to the position in the file that stores the edge information
    fseek(pfile, header.lump[LUMP_EDGES].nOffset, SEEK_SET);
    // Read in the pEdges
    fread(pEdges, sizeof(BSPEDGE), nEdges, pfile);

    // Seek to the position in the file that stores the vertex information
    fseek(pfile, header.lump[LUMP_VERTEXES].nOffset, SEEK_SET);
    // Read in the vertexs
    fread(pVertices, sizeof(BSPVERTEX), nVertices, pfile);

    // Seek to the position in the file that stores the plane information
    fseek(pfile, header.lump[LUMP_PLANES].nOffset, SEEK_SET);
    // Read in the planes
    fread(pPlanes, sizeof(BSPPLANE), nPlanes, pfile);

    // Seek to the position in the file that stores the model information
    fseek(pfile, header.lump[LUMP_MODELS].nOffset, SEEK_SET);
    // Read in the models
    fread(pModels, sizeof(BSPMODEL), nModels, pfile);

    // ===========================
    // Entity Operations
    // ===========================

    char* pszEntityBuffer = (char*) MALLOC(header.lump[LUMP_ENTITIES].nLength * sizeof(char));
    // Seek to the position in the file that stores the entity information
    fseek(pfile, header.lump[LUMP_ENTITIES].nOffset, SEEK_SET);
    // Read in the pEntities
    fread(pszEntityBuffer, sizeof(char), header.lump[LUMP_ENTITIES].nLength, pfile);

    // Parse the string and create the pEntities
    ParseEntities(pszEntityBuffer);

    // Delete the buffer
    free(pszEntityBuffer);

    // ===========================
    // Texture Operations
    // ===========================

    // Get the number of textures and allocate memory
    nTextureInfos = header.lump[LUMP_TEXINFO].nLength / sizeof(BSPTEXTUREINFO);
    pTextureInfos = (BSPTEXTUREINFO*) MALLOC(nTextureInfos * sizeof(BSPTEXTUREINFO));

    // Seek to the position in the file that stores the texture info information
    fseek(pfile, header.lump[LUMP_TEXINFO].nOffset, SEEK_SET);
    // Read in the texture infos
    fread(pTextureInfos, sizeof(BSPTEXTUREINFO), nTextureInfos, pfile);

    // Seek to the position in the file that stores the mip texture header info
    fseek(pfile, header.lump[LUMP_TEXTURES].nOffset, SEEK_SET);

    // Read in the texture header
    fread(&textureHeader, sizeof(BSPTEXTUREHEADER), 1, pfile);

    // Allocate the mip texture memory
    pMipTextures = (BSPMIPTEX*) MALLOC(textureHeader.nMipTextures * sizeof(BSPMIPTEX));

    // Allocate the mip texture offset memory

    pMipTextureOffsets = (BSPMIPTEXOFFSET*) MALLOC(textureHeader.nMipTextures * sizeof(BSPMIPTEXOFFSET));

    // Read in the mip texture offsets
    fread(pMipTextureOffsets, sizeof(int32_t), (textureHeader.nMipTextures), pfile);

    // Read in all the mip texture structs
    for (unsigned int i=0; i<textureHeader.nMipTextures; i++)
    {
        // Seek to the position in the file that stores the mip texture header info
        fseek(pfile, header.lump[LUMP_TEXTURES].nOffset + pMipTextureOffsets[i], SEEK_SET);
        // Read in the mip textures
        fread(&pMipTextures[i], sizeof(BSPMIPTEX), 1, pfile);
    }

    // Allocate the memory for the OpenGL texture unit IDs
    pnTextureLookUp = (GLuint*) MALLOC(textureHeader.nMipTextures * sizeof(GLuint));

    // Load in the texture images and calculate the texture cordinates for each vertex to save render time
    LoadTextures(pfile);

    // ===========================
    // Lightmap Operations
    // ===========================

    LOG("Loading lightmaps ...\n");
    if (header.lump[LUMP_LIGHTING].nLength == 0)
        LOG("No lightmapdata found\n");
    else
    {
        // Allocate the memory for the OpenGL texture unit IDs
        pnLightmapLookUp = (GLuint*) MALLOC(sizeof(GLuint) * nFaces);

        unsigned char* pLightMapData = (unsigned char*) MALLOC(header.lump[LUMP_LIGHTING].nLength * sizeof(unsigned char));

        // Seek to the position in the file that stores the light map information
        fseek(pfile, header.lump[LUMP_LIGHTING].nOffset, SEEK_SET);
        // Read in the light map data
        fread(pLightMapData, sizeof(unsigned char), header.lump[LUMP_LIGHTING].nLength, pfile);

        LoadLightMaps(pLightMapData);

        free(pLightMapData);
    }

    // ===========================
    // Decal Operations
    // ===========================

    LOG("Loading decals ...\n");
    LoadDecals();

    // ===========================
    // Skybox Operations
    // ===========================

    if (!LoadSkyTextures())
        return false;

    // ===========================
    // Visibility List Operations
    // ===========================

    if(header.lump[LUMP_VISIBILITY].nLength > 0)
    {
        // Allocate memory for the compressed vis lists
        unsigned char* pVisList = (unsigned char*) MALLOC(header.lump[LUMP_VISIBILITY].nLength * sizeof(unsigned char));

        // Seek to the beggining of the vis lump
        fseek(pfile, header.lump[LUMP_VISIBILITY].nOffset, SEEK_SET);
        // Read in the compressed visibility lists
        fread(pVisList, sizeof(unsigned char), header.lump[LUMP_VISIBILITY].nLength, pfile);

        LOG("Decompressing VIS ...\n");

        nVisLeafs = 0;
        CountVisLeafs(0);

        ppbVisLists = (bool**) MALLOC(sizeof(bool*) * nVisLeafs); // DO NOT MOVE THIS LINE Memory Bug
        //memset(ppbVisLists, 0, sizeof(bool*) * nVisLeafs);

        for (int i=0; i<nVisLeafs; i++)
        {
            if (pLeafs[i+1].nVisOffset >= 0)
                ppbVisLists[i] = GetPVS(i+1, pVisList);
            else
                ppbVisLists[i] = NULL;
        }

        free(pVisList);
    }
    else
        LOG("No VIS found\n");

    // Close the file
    fclose(pfile);

    // misc
    LOG("Miscellaneous ...\n");
    pbFacesDrawn = (bool*) MALLOC(sizeof(bool) * nFaces);

    LOG("FINISHED LOADING BSP\n");

    return true;
}

void CBSP::RenderLevel(VECTOR3D vPos)
{
    /** RENDER SKY BOX **/
    if ((pdlSkyBox != NULL) && g_bRenderSkybox)
    {
        if(g_bUseShader)
            glUniform1i(glGetUniformLocation(g_shpMain, "bUnit1Enabled"), 1);
        else
        {
            glActiveTexture(GL_TEXTURE0_ARB);
            glEnable(GL_TEXTURE_2D);
        }

        RenderSkybox(vPos);

        if(g_bUseShader)
            glUniform1i(glGetUniformLocation(g_shpMain, "bUnit1Enabled"), 0);
        else
            glDisable(GL_TEXTURE_2D);
    }

    /** PREPARE **/
    // turn on needed texture units
    if(g_bUseShader)
    {
        glUniform1i(glGetUniformLocation(g_shpMain, "bUnit1Enabled"), g_bTextures || g_bLightmaps);
        glUniform1i(glGetUniformLocation(g_shpMain, "bUnit2Enabled"), g_bTextures && g_bLightmaps);
    }
    else
    {
        if (g_bTextures || g_bLightmaps)
        {
            glActiveTexture(GL_TEXTURE0_ARB);
            glEnable(GL_TEXTURE_2D);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        }

        if (g_bLightmaps && g_bTextures)
        {
            glActiveTexture(GL_TEXTURE1_ARB);
            glEnable(GL_TEXTURE_2D);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }
    }

    glEnable(GL_DEPTH_TEST);

    memset(pbFacesDrawn, false, sizeof(bool) * nFaces);

    int iLeaf = TraverseBSPTree(vPos, 0); //Get the leaf where the camera is in
    //printf("Current Leaf %d\n", iLeaf);
    /** RENDER STATIC GEOMETRY **/
    if (g_bRenderStaticBSP)
        RenderBSP(0, iLeaf, vPos);

    /** RENDER BRUSH ENTITIES **/
    if (g_bRenderBrushEntities)
        for (int i=0; i<nBrushEntities; i++) //TODO: implement PVS for pEntities
            RenderBrushEntity(i, vPos);

    // Turn off second unit, if it was enabled
    if(g_bUseShader)
        glUniform1i(glGetUniformLocation(g_shpMain, "bUnit2Enabled"), 0);
    else
    {
        if (g_bLightmaps && g_bTextures)
        {
            glActiveTexture(GL_TEXTURE1_ARB);
            glDisable(GL_TEXTURE_2D);
        }
    }

    /** RENDER DECALS **/
    if(g_bRenderDecals)
    {
        glActiveTexture(GL_TEXTURE0_ARB);
        RenderDecals();
    }

    // Turn off first unit, if it was enabled
    if(g_bUseShader)
        glUniform1i(glGetUniformLocation(g_shpMain, "bUnit1Enabled"), 0);
    else
    {
        if (g_bLightmaps || g_bTextures)
        {
            glActiveTexture(GL_TEXTURE0_ARB);
            glDisable(GL_TEXTURE_2D);
        }
    }

    glDisable(GL_DEPTH_TEST);
}

void CBSP::RenderLeafOutlines(int iLeaf)
{
    srand(iLeaf);
    glColor3ub(rand()%255, rand()%255, rand()%255);
    BSPLEAF *curLeaf = &pLeafs[iLeaf];

    glBegin(GL_LINES);
    // Draw right face of bounding box
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMaxs[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMins[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMins[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMins[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMins[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMaxs[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMaxs[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMaxs[1], curLeaf->nMaxs[2]);

    // Draw left face of bounding box
    glVertex3f(curLeaf->nMins[0], curLeaf->nMins[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMaxs[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMaxs[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMaxs[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMaxs[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMins[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMins[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMins[1], curLeaf->nMins[2]);

    // Connect the faces
    glVertex3f(curLeaf->nMins[0], curLeaf->nMaxs[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMaxs[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMaxs[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMaxs[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMins[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMins[1], curLeaf->nMins[2]);
    glVertex3f(curLeaf->nMins[0], curLeaf->nMins[1], curLeaf->nMaxs[2]);
    glVertex3f(curLeaf->nMaxs[0], curLeaf->nMins[1], curLeaf->nMaxs[2]);
    glEnd();
}

void CBSP::RenderLeavesOutlines()
{
    glLineWidth(1.0f);
    glLineStipple(1, 0xF0F0);
    glEnable(GL_LINE_STIPPLE);
    for(int i=0; i<nLeafs; i++)
    {
        RenderLeafOutlines(i);
    }
    glDisable(GL_LINE_STIPPLE);
    glColor3f(1,1,1);
}

CEntity* CBSP::FindEntity(const char* pszNewClassName)
{
    static int i;
    static const char* pszClassName;
    if(pszNewClassName == NULL)
        i++;
    else
    {
        i = 0;
        pszClassName = pszNewClassName;
    }

    for (; i<nEntities; i++)
    {
        const char* szClass = pEntities[i].FindProperty("classname");
        if(szClass == NULL)
            continue;

        if (!strcmp(szClass, pszClassName))
            return &pEntities[i];
    }

    return NULL;
}

/**************** FROM QUAKE 1 ***********/

int hull;
bool allsolid;
float traceRatio;
VECTOR3D vCollisionNormal;

VECTOR3D CBSP::Trace(VECTOR3D vStart, VECTOR3D vEnd)
{
    if(vStart == vEnd)
        return vEnd; // no move

    printf("begin trace start %.1f/%.1f/%.1f end %.1f/%.1f/%.1f \n", vStart.x, vStart.y, vStart.z, vEnd.x, vEnd.y, vEnd.z);
    // Initially we set our trace ratio to 1.0f, which means that we don't have
    // a collision or intersection point, so we can move freely.
    traceRatio = 1.0f;

    allsolid = true;

    // We start out with the first node (0), setting our start and end ratio to 0 and 1.
    // We will recursively go through all of the nodes to see which brushes we should check.
    //CheckNode(pModels[0].iHeadNodes[hull], 0.0f, 1.0f, vStart, vEnd);
    RecursiveHullCheck(pModels[0].iHeadNodes[hull], 0, 1, vStart, vEnd);

    printf("end trace %f\n", traceRatio);

    // If the traceRatio is STILL 1.0f, then we never collided and just return our end position
    if(traceRatio == 1.0f)
    {
        return vEnd;
    }
    else	// Else COLLISION!!!!
    {
        printf("coll normal %.1f/%.1f/%.1f\n", vCollisionNormal.x, vCollisionNormal.y, vCollisionNormal.z);

        // Set our new position to a position that is right up to the brush we collided with
        VECTOR3D vNewPosition = vStart + ((vEnd - vStart) * traceRatio);

        // Get the distance from the end point to the new position we just got
        VECTOR3D vMove = vEnd - vNewPosition;

        // Get the distance we need to travel backwards to the new slide position.
        // This is the distance of course along the normal of the plane we collided with.
        float distance = DotProduct(vMove, vCollisionNormal);

        // Get the new end position that we will end up (the slide position).
        VECTOR3D vEndPosition = vEnd - vCollisionNormal * distance;

        // Since we got a new position for our sliding vector, we need to check
        // to make sure that new sliding position doesn't collide with anything else.
        vNewPosition = Trace(vNewPosition, vEndPosition);

        // Return the new position to be used by our camera (or player)
        return vNewPosition;
    }
}

VECTOR3D CBSP::Move(VECTOR3D vStart, VECTOR3D vEnd, int h)
{
    hull = h;
    return Trace(vStart, vEnd);
}

int CBSP::HullPointContents(int iNode, VECTOR3D p)
{
    float       d;
    BSPCLIPNODE *node;
    BSPPLANE    *plane;

    while (iNode >= 0)
    {
        //if (iNode < hull->firstclipnode || iNode > hull->lastclipnode)
        //Sys_Error ("PM_HullPointContents: bad node number");
        assert(iNode >= 0 && iNode < nClipNodes);

        node = pClipNodes + iNode;
        plane = pPlanes + node->iPlane;

        //if (plane->nType < 3)
        //    d = p[plane->nType] - plane->fDist;
        //else
        d = DotProduct (plane->vNormal, p) - plane->fDist;
        if (d < 0)
            iNode = node->iChildren[1];
        else
            iNode = node->iChildren[0];
    }

    return iNode;
}

bool CBSP::RecursiveHullCheck (int iNode, float p1f, float p2f, VECTOR3D p1, VECTOR3D p2)
{
    BSPCLIPNODE* node;
    BSPPLANE    *plane;
    float       t1, t2;
    float       frac;
    //int         i;
    VECTOR3D      mid;
    int         side;
    float       midf;

// check for empty
    if (iNode < 0)
    {
        if (iNode != CONTENTS_SOLID)
        {
            allsolid = false;
            //if (iNode == CONTENTS_EMPTY)
            //    trace->inopen = true;
            //else
            //    trace->inwater = true;
        }
        //else
            //trace->startsolid = true;
        return true;        // empty
    }

    //if (iNode < hull->firstclipnode || iNode > hull->lastclipnode)
    //    Sys_Error ("RecursiveHullCheck: bad node number");
    assert(iNode >= 0 && iNode < nClipNodes);

//
// find the point distances
//
    node = pClipNodes + iNode;
    plane = pPlanes + node->iPlane;

    /*if (plane->type < 3)
    {
        t1 = p1[plane->type] - plane->dist;
        t2 = p2[plane->type] - plane->dist;
    }
    else*/
    {
        t1 = DotProduct (plane->vNormal, p1) - plane->fDist;
        t2 = DotProduct (plane->vNormal, p2) - plane->fDist;
    }

    if (t1 >= 0 && t2 >= 0)
        return RecursiveHullCheck ( node->iChildren[0], p1f, p2f, p1, p2);
    if (t1 < 0 && t2 < 0)
        return RecursiveHullCheck ( node->iChildren[1], p1f, p2f, p1, p2);

    // put the crosspoint DIST_EPSILON pixels on the near side
    if (t1 < 0)
        frac = (t1 + EPSILON)/(t1-t2);
    else
        frac = (t1 - EPSILON)/(t1-t2);
    if (frac < 0)
        frac = 0;
    if (frac > 1)
        frac = 1;

    midf = p1f + (p2f - p1f)*frac;
    //for (i=0 ; i<3 ; i++)
    mid = p1 + frac * (p2 - p1);

    side = (t1 < 0);

    // move up to the node
    if (!RecursiveHullCheck (node->iChildren[side], p1f, midf, p1, mid) )
        return false;

    /*#ifdef PARANOID
        if (PM_HullPointContents (pm_hullmodel, mid, node->children[side])
        == CONTENTS_SOLID)
        {
            Con_Printf ("mid PointInHullSolid\n");
            return false;
        }
    #endif*/

    if (HullPointContents (node->iChildren[side^1], mid) != CONTENTS_SOLID)
        // go past the node
        return RecursiveHullCheck (node->iChildren[side^1], midf, p2f, mid, p2);

    if (allsolid)
        return false;       // never got out of the solid area

//==================
// the other side of the node is solid, this is the impact point
//==================
    if (!side)
    {
        //VectorCopy (plane->normal, trace->plane.normal);
        //trace->plane.dist = plane->dist;
        vCollisionNormal = plane->vNormal;
    }
    else
    {
        //VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
        //trace->plane.dist = -plane->dist;
        vCollisionNormal = -1 * plane->vNormal;
    }

    while (HullPointContents (pModels[0].iHeadNodes[hull], mid) == CONTENTS_SOLID)
    {
        // shouldn't really happen, but does occasionally
        frac -= 0.1;
        if (frac < 0)
        {
            //trace->fraction = midf;
            traceRatio = midf;
            //VectorCopy (mid, trace->endpos);
            printf ("backup past 0\n");
            return false;
        }
        midf = p1f + (p2f - p1f)*frac;
        //for (i=0 ; i<3 ; i++)
            mid = p1 + frac*(p2 - p1);
    }

    //trace->fraction = midf;
    traceRatio = midf;
    //VectorCopy (mid, trace->endpos);

    return false;
}

/**************** END FROM QUAKE 1 **************/

void CBSP::Destroy()
{
    // Entities
    if (ppBrushEntities)
    {
        free(ppBrushEntities);
        ppBrushEntities = NULL;
    }

    if (ppSpecialEntities)
    {
        free(ppSpecialEntities);
        ppSpecialEntities = NULL;
    }

    //textures
    if (pnTextureLookUp)
    {
        glDeleteTextures(textureHeader.nMipTextures, pnTextureLookUp);

        free(pnTextureLookUp);
        pnTextureLookUp = NULL;
    }

    //lightmaps
    if (pnLightmapLookUp)
    {
        for (int i=0; i<nFaces; i++)
        {
            if (pnLightmapLookUp[i]!=0)
                glDeleteTextures(1, &pnLightmapLookUp[i]);
        }

        free(pnLightmapLookUp);
        pnLightmapLookUp = NULL;
    }

    // decals
    if(pDecalWads)
    {
        for(int i=0; i<DECAL_WAD_COUNT; i++)
            pDecalWads[i].Close();

        delete[] pDecalWads;
        pDecalWads = NULL;
    }

    //displaylists
    if (pdlSkyBox)
    {
        glDeleteLists(*pdlSkyBox, 1);

        free(pdlSkyBox);
        pdlSkyBox = NULL;
    }

    //visLists
    if (ppbVisLists)
    {
        for (int i=0; i<nVisLeafs; i++)
        {
            if (ppbVisLists[i])
            {
                free(ppbVisLists[i]);
                ppbVisLists[i] = NULL;
            }
        }

        free(ppbVisLists);
        ppbVisLists = NULL;
    }

    //facesdrawn
    if (pbFacesDrawn)
    {
        free(pbFacesDrawn);
        pbFacesDrawn = NULL;
    }

    // If we still have valid memory for our pEntities, free them
    if (pEntities)
    {
        delete [] pEntities; // pointer to array of classes
        pEntities = NULL;
    }

    // If we still have valid memory for our pNodes, free them
    if (pNodes)
    {
        free(pNodes);
        pNodes = NULL;
    }

    // If we still have valid memory for our pLeafs, free them
    if (pLeafs)
    {
        free(pLeafs);
        pLeafs = NULL;
    }

    // If we still have valid memory for our pMarkSurfaces, free them
    if (pMarkSurfaces)
    {
        free(pMarkSurfaces);
        pMarkSurfaces = NULL;
    }

    // If we still have valid memory for our pFaces, free them
    if (pFaces)
    {
        free(pFaces);
        pFaces = NULL;
    }

    // If we still have valid memory for our surfedges, free them
    if (pSurfEdges)
    {
        free(pSurfEdges);
        pSurfEdges = NULL;
    }

    // If we still have valid memory for our pEdges, free them
    if (pEdges)
    {
        free(pEdges);
        pEdges = NULL;
    }

    // If we still have valid memory for our pVertices, free them
    if (pVertices)
    {
        free(pVertices);
        pVertices = NULL;
    }

    // If we still have valid memory for our pPlanes, free them
    if (pVertices)
    {
        free(pPlanes);
        pPlanes = NULL;
    }

    // If we still have valid memory for our pModels, free them
    if (pModels)
    {
        free(pModels);
        pModels = NULL;
    }
}
