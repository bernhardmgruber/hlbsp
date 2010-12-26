#include "wad.h"
#include "main.h"

#include <stdlib.h>
#include <math.h>
#include <ctype.h>

CWAD::CWAD()
{
    pWadFile = NULL;
    pDirEntries = NULL;
    nDirEntries = 0;
}

CWAD::~CWAD()
{
    Close();
}

bool CWAD::Open(const char * pszFilename)
{
    // WAD file should be in current working directory
    pWadFile = fopen(pszFilename, "rb");

    if (!pWadFile)
        return false;

    // Load file header and WAD directory
    return LoadDirectory();
}

void CWAD::Close()
{
    if (pWadFile)
        fclose(pWadFile);

    pWadFile = NULL;

    if (pDirEntries)
    {
        free(pDirEntries);
        pDirEntries = NULL;
        nDirEntries = 0;
    }
}

MIPTEXTURE* CWAD::LoadTexture(const char * pszTexName)
{
    MIPTEXTURE* pMipMapTexture = (MIPTEXTURE*) MALLOC(sizeof(MIPTEXTURE));

    unsigned char* pRawTex = GetTexture(pszTexName);
    if(!pRawTex)
        return NULL;

    CreateMipTexture(pRawTex, pMipMapTexture);

    free(pRawTex);

    return pMipMapTexture;
}

MIPTEXTURE* CWAD::LoadDecalTexture(const char* pszTexName)
{
    MIPTEXTURE* pMipMapTexture = (MIPTEXTURE*) MALLOC(sizeof(MIPTEXTURE));

    unsigned char* pRawTex = GetTexture(pszTexName);
    if(!pRawTex)
        return NULL;

    CreateDecalTexture(pRawTex, pMipMapTexture);

    free(pRawTex);

    return pMipMapTexture;
}

int DirCompare(const void* pA, const void* pB)
{
    return strcasecmp(((WADDIRENTRY*)pA)->szName, ((WADDIRENTRY*)pB)->szName);
}

bool CWAD::LoadDirectory()
{
    WADHEADER header;

    // Get file header information
    fseek(pWadFile, 0, SEEK_SET);
    fread(&header, sizeof(WADHEADER), 1, pWadFile);

    // Check magic
    if(header.szMagic[0] != 'W' ||
       header.szMagic[1] != 'A' ||
       header.szMagic[2] != 'D' ||
       (header.szMagic[3] != '2' && header.szMagic[3] != '3'))
        return false;

    // Read in directory
    pDirEntries = (WADDIRENTRY*) MALLOC(sizeof(WADDIRENTRY) * header.nDir);
    fseek(pWadFile, header.nDirOffset, SEEK_SET);
    fread(pDirEntries, sizeof(WADDIRENTRY), header.nDir, pWadFile);
    nDirEntries = header.nDir;

    // Sort directory by texture name
    qsort(pDirEntries, nDirEntries, sizeof(WADDIRENTRY), DirCompare);

    return true;
}

int CWAD::FindTexture(const char * pszName)
{
    int nDirIndex = -1;

    // Use binary search to quickly find WAD texture
    int nMin = 0;
    int nMax = nDirEntries - 1;

    while (nMin <= nMax)
    {
        int nMid = ((nMax - nMin) / 2) + nMin;

        int nResult = strcasecmp(pszName, pDirEntries[nMid].szName);
        if (nResult > 0)
        {
            nMin = nMid + 1;
        }
        else if (nResult < 0)
        {
            nMax = nMid - 1;
        }
        else
        {
            nDirIndex = nMid;
            break;
        }
    }

    return nDirIndex;
}

unsigned char* CWAD::GetTexture(const char* pszTexName)
{
    // Get texture offset from the WAD directory
    int iDir = FindTexture(pszTexName);
    if (iDir < 0)
        return NULL;

    // We are assuming all WAD textures are not compressed
    if (pDirEntries[iDir].bCompression != 0)
    {
        LOG("WAD texture cannot be loaded. Cannot read compressed items.\n");
        return NULL;
    }

    size_t nTexSize = pDirEntries[iDir].nSize;
    unsigned char * wadtexdata = (unsigned char*) MALLOC(nTexSize * sizeof(unsigned char));

    fseek(pWadFile, pDirEntries[iDir].nFilePos, SEEK_SET);
    fread(wadtexdata, sizeof(unsigned char), nTexSize, pWadFile);

    return wadtexdata;
}

void CWAD::CreateMipTexture(const unsigned char* pRawTexture, MIPTEXTURE* pMipTex)
{
    BSPMIPTEX * pRawMipTex = (BSPMIPTEX*) pRawTexture;

    int nWidth = pRawMipTex->nWidth;
    int nHeight = pRawMipTex->nHeight;
    int nRGBPalIndex = pRawMipTex->nOffsets[3] + ((nWidth / 8) * (nHeight / 8)) + 2;
    const unsigned char * pRGBPalette = &(pRawTexture[nRGBPalIndex]);

    for (int i=0;i<MIPLEVELS;i++)
    {
        const unsigned char * pPalTexture = &(pRawTexture[pRawMipTex->nOffsets[i]]);

        unsigned char * pRGBATexture = (unsigned char*) MALLOC(nWidth * nHeight * 4 * sizeof(unsigned char));

        for (int j=0; j<nHeight * nWidth; j++)
        {
                int nPalIndex = pPalTexture[j] * 3;

                pRGBATexture[j * 4]     = pRGBPalette[nPalIndex];
                pRGBATexture[j * 4 + 1] = pRGBPalette[nPalIndex + 1];
                pRGBATexture[j * 4 + 2] = pRGBPalette[nPalIndex + 2];
                pRGBATexture[j * 4 + 3] = 255; //every pixel is totally opaque
        }

        pMipTex->Img[i].nChannels = 4;
        pMipTex->Img[i].nWidth =  nWidth;
        pMipTex->Img[i].nHeight = nHeight;
        pMipTex->Img[i].pData = pRGBATexture;

        ApplyAlphaSections(&pMipTex->Img[i]);

        nWidth >>= 1;
        nHeight >>= 1;
    }
}

void CWAD::CreateDecalTexture(const unsigned char* pRawTexture, MIPTEXTURE* pMipTex)
{
    BSPMIPTEX * pRawMipTex = (BSPMIPTEX*) pRawTexture;

    int nWidth = pRawMipTex->nWidth;
    int nHeight = pRawMipTex->nHeight;
    int nRGBPalIndex = pRawMipTex->nOffsets[3] + ((nWidth / 8) * (nHeight / 8)) + 2;
    const unsigned char * pRGBPalette = &(pRawTexture[nRGBPalIndex]);

    for (int i=0;i<MIPLEVELS;i++)
    {
        const unsigned char * pPalTexture = &(pRawTexture[pRawMipTex->nOffsets[i]]);

        const unsigned char* pColor = pRGBPalette + 255 * 3;

        unsigned char * pRGBATexture = (unsigned char*) MALLOC(nWidth * nHeight * 4 * sizeof(unsigned char));

        for (int j=0; j<nHeight * nWidth; j++)
        {
                int nPalIndex = pPalTexture[j] * 3;

                pRGBATexture[j * 4]     = pColor[0];
                pRGBATexture[j * 4 + 1] = pColor[1];
                pRGBATexture[j * 4 + 2] = pColor[2];
                pRGBATexture[j * 4 + 3] = 255 - pRGBPalette[nPalIndex];
        }

        pMipTex->Img[i].nChannels = 4;
        pMipTex->Img[i].nWidth =  nWidth;
        pMipTex->Img[i].nHeight = nHeight;
        pMipTex->Img[i].pData = pRGBATexture;

        ApplyAlphaSections(&pMipTex->Img[i]);

        nWidth >>= 1;
        nHeight >>= 1;
    }
}

void CWAD::ApplyAlphaSections(IMAGE* pTex)
{
    unsigned char* pRGBTexture = (unsigned char*) MALLOC(pTex->nWidth * pTex->nHeight * 4 * sizeof(unsigned char));

    memset(pRGBTexture, 0, pTex->nWidth * pTex->nHeight * 4 * sizeof(unsigned char));

    // Color pRGBTexture totally blue
    for(int i=0;i<pTex->nHeight * pTex->nWidth;i++)
        pRGBTexture[i * 4 + 2] = 255;

    for (int y=0;y<pTex->nHeight;y++)
    {
        for (int x=0;x<pTex->nWidth;x++)
        {
            int index = y * pTex->nWidth + x;

            if ((pTex->pData[index * 4] == 0) && (pTex->pData[index * 4 + 1] == 0) && (pTex->pData[index * 4 + 2] == 255))
            {
                // Blue color signifies a transparent portion of the texture. zero alpha for blending and
                // to get rid of blue edges choose the average color of the nearest non blue pixels

                //First set pixel black and transparent
                pTex->pData[index * 4 + 2] = 0;
                pTex->pData[index * 4 + 3] = 0;

                int nCount = 0;
                unsigned int RGBColorSum[3] = {0,0,0};

                //left above pixel
                if((x > 0) && (y > 0))
                {
                    int iPixel = ((y - 1) * pTex->nWidth + (x - 1)) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += (unsigned int)((float)pTex->pData[iPixel] * M_SQRT2);
                        RGBColorSum[1] += (unsigned int)((float)pTex->pData[iPixel + 1] * M_SQRT2);
                        RGBColorSum[2] += (unsigned int)((float)pTex->pData[iPixel + 2] * M_SQRT2);
                        nCount++;
                    }
                }

                //above pixel
                if((x >= 0) && (y > 0))
                {
                    int iPixel = ((y - 1) * pTex->nWidth + x) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += pTex->pData[iPixel];
                        RGBColorSum[1] += pTex->pData[iPixel + 1];
                        RGBColorSum[2] += pTex->pData[iPixel + 2];
                        nCount++;
                    }
                }

                //right above pixel
                if((x < pTex->nWidth - 1) && (y > 0))
                {
                    int iPixel = ((y - 1) * pTex->nWidth + (x + 1)) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += (unsigned int)((float)pTex->pData[iPixel] * M_SQRT2);
                        RGBColorSum[1] += (unsigned int)((float)pTex->pData[iPixel + 1] * M_SQRT2);
                        RGBColorSum[2] += (unsigned int)((float)pTex->pData[iPixel + 2] * M_SQRT2);
                        nCount++;
                    }
                }

                //left pixel
                if(x > 0)
                {
                    int iPixel = (y * pTex->nWidth + (x - 1)) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += pTex->pData[iPixel];
                        RGBColorSum[1] += pTex->pData[iPixel + 1];
                        RGBColorSum[2] += pTex->pData[iPixel + 2];
                        nCount++;
                    }
                }

                //right pixel
                if(x < pTex->nWidth - 1)
                {
                    int iPixel = (y * pTex->nWidth + (x + 1)) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += pTex->pData[iPixel];
                        RGBColorSum[1] += pTex->pData[iPixel + 1];
                        RGBColorSum[2] += pTex->pData[iPixel + 2];
                        nCount++;
                    }
                }

                //left underneath pixel
                if((x > 0) && (y < pTex->nHeight - 1))
                {
                    int iPixel = ((y + 1) * pTex->nWidth + (x - 1)) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += (unsigned int)((float)pTex->pData[iPixel] * M_SQRT2);
                        RGBColorSum[1] += (unsigned int)((float)pTex->pData[iPixel + 1] * M_SQRT2);
                        RGBColorSum[2] += (unsigned int)((float)pTex->pData[iPixel + 2] * M_SQRT2);
                        nCount++;
                    }
                }

                //underneath pixel
                if((x >= 0) && (y < pTex->nHeight - 1))
                {
                    int iPixel = ((y + 1) * pTex->nWidth + x) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += pTex->pData[iPixel];
                        RGBColorSum[1] += pTex->pData[iPixel + 1];
                        RGBColorSum[2] += pTex->pData[iPixel + 2];
                        nCount++;
                    }
                }

                //right underneath pixel
                if((x < pTex->nWidth - 1) && (y < pTex->nHeight - 1))
                {
                    int iPixel = ((y + 1) * pTex->nWidth + (x + 1)) * 4;
                    if (!((pTex->pData[iPixel] == 0) && (pTex->pData[iPixel + 1] == 0) && (pTex->pData[iPixel + 2] == 255)))
                    {
                        RGBColorSum[0] += (unsigned int)((float)pTex->pData[iPixel] * M_SQRT2);
                        RGBColorSum[1] += (unsigned int)((float)pTex->pData[iPixel + 1] * M_SQRT2);
                        RGBColorSum[2] += (unsigned int)((float)pTex->pData[iPixel + 2] * M_SQRT2);
                        nCount++;
                    }
                }

                if (nCount > 0)
                {
                    RGBColorSum[0] /= nCount;
                    RGBColorSum[1] /= nCount;
                    RGBColorSum[2] /= nCount;

                    pRGBTexture[index * 4] = RGBColorSum[0];
                    pRGBTexture[index * 4 + 1] = RGBColorSum[1];
                    pRGBTexture[index * 4 + 2] = RGBColorSum[2];
                }
            }
        }
    }

    //Merge pTex and pRGBTexture
    for (int y=0;y<pTex->nHeight;y++)
    {
        for (int x=0;x<pTex->nWidth;x++)
        {
            int index = y * pTex->nWidth + x;

            if ((pRGBTexture[index * 4] != 0) || (pRGBTexture[index * 4 + 1] != 0) || (pRGBTexture[index * 4 + 2] != 255) || (pRGBTexture[index * 4 + 3] != 0))
            {
                memcpy(&pTex->pData[index * 4], &pRGBTexture[index * 4], sizeof(unsigned char) * 4);
            }
        }
    }

    free(pRGBTexture);
}

void FreeMipTexturePointer(MIPTEXTURE* pMipTex)
{
    for (int i=0;i<MIPLEVELS;i++)
        FreeImage(pMipTex->Img[i]);
    free(pMipTex);
    pMipTex = NULL;
}

void FreeMipTexture(MIPTEXTURE MipTex)
{
    for (int i=0;i<MIPLEVELS;i++)
        FreeImage(MipTex.Img[i]);
}
