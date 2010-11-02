#include "gltexldr.h"

#include <gl/glu.h>
#include <gl/glext.h>
#include <stdio.h>

bool LoadTexture(GLuint* texture, const char* fileName, unsigned char mode)
{
    if (!fileName)
    {
        char errtext[512];
        sprintf(errtext, "Could not load texture %s. Invalid filename.", fileName);
        MessageBox(NULL, errtext, "ERROR", MB_OK | MB_ICONERROR);
        return false;
    }

    // Define a pointer to a tImage
    IMAGE *pImage = NULL;

#ifdef _WINDOWS_H
    if(mode & LT_RESOURCE) //Resource
    {
        pImage = LoadBMP(fileName, true);
        if (pImage==NULL)
        {
            char errtext[512];
            sprintf(errtext, "Could not load texture %s. Loading BMP from resource failed.", fileName);
            MessageBox(NULL, errtext, "ERROR", MB_OK | MB_ICONERROR);
            return false;
        }

        int resID = LOWORD((DWORD)fileName);
        fileName = (char*) malloc(sizeof(char) * 64);
        if(fileName == NULL)
        {
            printf("Memory allocation failed\n");
            return false;
        }
        sprintf((char*)fileName, "#%d (resource)", resID);
    }
    else
    #endif // _WINDOWS_H
    {
        if (strstr(fileName, ".tga")) //TGA File
        {
            pImage = LoadTGA(fileName);
            if (pImage==NULL)
            {
                char errtext[512];
                sprintf(errtext, "Could not load texture %s. Loading TGA file failed.", fileName);
                MessageBox(NULL, errtext, "ERROR", MB_OK | MB_ICONERROR);
                return false;
            }
        }
        else if (strstr(fileName, ".bmp")) //BMP File
        {
            pImage = LoadBMP(fileName, false);
            if (pImage==NULL)
            {
                char errtext[512];
                sprintf(errtext, "Could not load texture %s. Loading BMP file failed.", fileName);
                MessageBox(NULL, errtext, "ERROR", MB_OK | MB_ICONERROR);
                return false;
            }
        }
        else // Else we don't support the file format
        {
            char errtext[512];
            sprintf(errtext, "Could not load texture %s. Unknown file extension.", fileName);
            MessageBox(NULL, errtext, "ERROR", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    if(!CreateTexture(texture, pImage, mode))
    {
        char errtext[512];
        sprintf(errtext, "Could not load texture %s.", fileName);
        MessageBox(NULL, errtext, "ERROR", MB_OK | MB_ICONERROR);
        return false;
    }
    else
    {
        printf("Loaded Texture %s width ID %d\n", fileName, *texture);
        return true;
    }
}

bool CreateTexture(GLuint* texture, IMAGE* pImage, unsigned char mode)
{
    int textureType;

    if (pImage->nChannels == 4)
        textureType = GL_BGRA_EXT;
    else
        textureType = GL_BGR_EXT;

    // Make sure valid IMAGE data was given to pImage, otherwise return false
    if (pImage == NULL)
        return false;

    // Generate a texture with the associative texture ID stored in the array
    int texCount = 0;
    if (mode &CT_NEAREST)
        texCount++;
    if (mode & CT_LINEAR)
        texCount++;
    if (mode & CT_MIPMAP)
        texCount++;

    glGenTextures(texCount, texture);

    // This sets the alignment requirements for the start of each pixel row in memory.
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    // Bind the texture to the texture arrays index and init the texture
    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    texCount = 0;

    if (mode & CT_NEAREST)
    {
        // Create Nearest Filtered Texture
        glBindTexture(GL_TEXTURE_2D, texture[texCount]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, pImage->nChannels, pImage->nWidth, pImage->nHeight, 0, textureType, GL_UNSIGNED_BYTE, pImage->pData);
        texCount++;
    }

    if (mode & CT_LINEAR)
    {
        // Create Linear Filtered Texture
        glBindTexture(GL_TEXTURE_2D, texture[texCount]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, pImage->nChannels, pImage->nWidth, pImage->nHeight, 0, textureType, GL_UNSIGNED_BYTE, pImage->pData);
        texCount++;
    }

    if (mode & CT_MIPMAP)
    {
        // Create MipMapped Texture
        glBindTexture(GL_TEXTURE_2D, texture[texCount]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
        gluBuild2DMipmaps(GL_TEXTURE_2D, pImage->nChannels, pImage->nWidth, pImage->nHeight, textureType, GL_UNSIGNED_BYTE, pImage->pData);
        texCount++;
    }

    if (pImage)										// If we loaded the IMAGE
    {
        if (pImage->pData)							// If there is texture data
        {
            free(pImage->pData);						// Free the texture data, we don't need it anymore
        }

        free(pImage);								// Free the IMAGE structure
    }

    return true;
}
