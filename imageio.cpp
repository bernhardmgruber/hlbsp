#include "imageio.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TGA_RGB		 2		// This tells us it's a normal RGB (really BGR) file
#define TGA_A		 3		// This tells us it's a ALPHA file
#define TGA_RLE		10		// This tells us that the targa is Run-Length Encoded (RLE)

IMAGE* CreateImage(BYTE nChannels, WORD nWidth, WORD nHeight)
{
    if(nChannels != 3 && nChannels != 4)
        return NULL;

    IMAGE* pImg = (IMAGE*) malloc(sizeof(IMAGE));
    if(pImg == NULL)
    {
        fprintf(stderr, "Memory allocation failed");
        return NULL;
    }
    pImg->nChannels = nChannels;
    pImg->nWidth = nWidth;
    pImg->nHeight = nHeight;
    pImg->pData = (BYTE*) malloc(nWidth * nHeight * nChannels * sizeof(BYTE));
    if(pImg->pData == NULL)
    {
        fprintf(stderr, "Memory allocation failed");
        return NULL;
    }
    memset(pImg->pData, 0, nWidth * nHeight * nChannels * sizeof(BYTE));
    return pImg;
}

bool GetPixel(IMAGE* pImg, WORD x, WORD y, BYTE* pRed, BYTE* pGreen, BYTE* pBlue, BYTE* pAlpha)
{
    if (x > pImg->nWidth-1 || y > pImg->nHeight-1)
        return false;

    if (pImg->nChannels >= 3)
    {
        *pRed   = pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 0];
        *pGreen = pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 1];
        *pBlue  = pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 2];
    }
    if (pImg->nChannels == 4)
        *pAlpha = pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 3];

    return true;
}

bool SetPixel(IMAGE* pImg, WORD x, WORD y, BYTE red, BYTE green, BYTE blue, BYTE alpha)
{
    if (x > pImg->nWidth-1 || y > pImg->nHeight-1)
        return false;

    if (pImg->nChannels >= 3)
    {
        pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 0] = red;
        pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 1] = green;
        pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 2] = blue;
    }
    if (pImg->nChannels == 4)
        pImg->pData[(y * pImg->nWidth + x) * pImg->nChannels + 3] = alpha;

    return true;
}

IMAGE *LoadTGA(const char *szFileName)
{
    IMAGE *pImg = NULL;			// This stores our important IMAGE data
    WORD nWidth = 0, nHeight = 0;			// The dimensions of the IMAGE
    BYTE length = 0;					// The length in bytes to the pixels
    BYTE imageType = 0;					// The IMAGE type (RLE, RGB, Alpha...)
    BYTE bits = 0;						// The bits per pixel for the IMAGE (16, 24, 32)
    FILE *pFile = NULL;					// The file pointer
    int channels = 0;					// The channels of the IMAGE (3 = RGA : 4 = RGBA)
    int stride = 0;						// The stride (channels * nWidth)
    int i = 0;							// A counter

    // Open a file pointer to the targa file and check if it was found and opened
    if ((pFile = fopen(szFileName, "rb")) == NULL)
        return NULL;

    // Allocate the structure that will hold our eventual IMAGE data (must free it!)
    pImg = (IMAGE*)malloc(sizeof(IMAGE));
    if(pImg == NULL)
    {
        fprintf(stderr, "Memory allocation failed");
        return NULL;
    }

    // Read in the length in bytes from the header to the pixel data
    fread(&length, sizeof(BYTE), 1, pFile);

    // Jump over one byte
    fseek(pFile,1,SEEK_CUR);

    // Read in the imageType (RLE, RGB, etc...)
    fread(&imageType, sizeof(BYTE), 1, pFile);

    // Skip past general information we don't care about
    fseek(pFile, 9, SEEK_CUR);

    // Read the nWidth, nHeight and bits per pixel (16, 24 or 32)
    fread(&nWidth,  sizeof(WORD), 1, pFile);
    fread(&nHeight, sizeof(WORD), 1, pFile);
    fread(&bits,   sizeof(BYTE), 1, pFile);

    // Now we move the file pointer to the pixel data
    fseek(pFile, length + 1, SEEK_CUR);

    // Check if the IMAGE is RLE compressed or not
    if (imageType != TGA_RLE)
    {
        // Check if the IMAGE is a 24 or 32-bit IMAGE
        if (bits == 24 || bits == 32)
        {
            // Calculate the channels (3 or 4) - (use bits >> 3 for more speed).
            // Next, we calculate the stride and allocate enough memory for the pixels.
            channels = bits / 8;
            stride = channels * nWidth;
            pImg->pData = ((unsigned char*)malloc(sizeof(unsigned char)*stride*nHeight));
            if(pImg->pData == NULL)
            {
                fprintf(stderr, "Memory allocation failed");
                return NULL;
            }

            // Load in all the pixel data line by line
            for (int y = 0; y < nHeight; y++)
            {
                // Store a pointer to the current line of pixels
                unsigned char *pLine = &(pImg->pData[stride * y]);

                // Read in the current line of pixels
                fread(pLine, stride, 1, pFile);

                // Go through all of the pixels and swap the B and R values since TGA
                // files are stored as BGR instead of RGB (or use GL_BGR_EXT verses GL_RGB)
                for (i = 0; i < stride; i += channels)
                {
                    int temp     = pLine[i];
                    pLine[i]     = pLine[i + 2];
                    pLine[i + 2] = temp;
                }
            }
        }
        // Check if the IMAGE is a 16 bit IMAGE (RGB stored in 1 unsigned short)
        else if (bits == 16)
        {
            unsigned short pixels = 0;
            int r=0, g=0, b=0;

            // Since we convert 16-bit images to 24 bit, we hardcode the channels to 3.
            // We then calculate the stride and allocate memory for the pixels.
            channels = 3;
            stride = channels * nWidth;
            pImg->pData = ((unsigned char*)malloc(sizeof(unsigned char)*stride*nHeight));
            if(pImg->pData == NULL)
            {
                fprintf(stderr, "Memory allocation failed");
                return NULL;
            }

            // Load in all the pixel data pixel by pixel
            for (int i = 0; i < nWidth*nHeight; i++)
            {
                // Read in the current pixel
                fread(&pixels, sizeof(unsigned short), 1, pFile);

                // Convert the 16-bit pixel into an RGB
                b = (pixels & 0x1f) << 3;
                g = ((pixels >> 5) & 0x1f) << 3;
                r = ((pixels >> 10) & 0x1f) << 3;

                // This essentially assigns the color to our array and swaps the
                // B and R values at the same time.
                pImg->pData[i * 3 + 0] = r;
                pImg->pData[i * 3 + 1] = g;
                pImg->pData[i * 3 + 2] = b;
            }
        }
        // Else return a NULL for a bad or unsupported pixel format
        else
            return NULL;
    }
    // Else, it must be Run-Length Encoded (RLE)
    else
    {
        // Create some variables to hold the rleID, current colors read, channels, & stride.
        BYTE rleID = 0;
        int colorsRead = 0;
        channels = bits / 8;
        stride = channels * nWidth;

        // Next we want to allocate the memory for the pixels and create an array,
        // depending on the channel count, to read in for each pixel.
        pImg->pData = ((unsigned char*)malloc(sizeof(unsigned char)*stride*nHeight));
        if(pImg->pData == NULL)
        {
            fprintf(stderr, "Memory allocation failed");
            return NULL;
        }
        BYTE *pColors = (BYTE*)malloc(sizeof(BYTE) * channels);
        if(pColors == NULL)
        {
            fprintf(stderr, "Memory allocation failed");
            return NULL;
        }

        // Load in all the pixel data
        while (i < nWidth*nHeight)
        {
            // Read in the current color count + 1
            fread(&rleID, sizeof(BYTE), 1, pFile);

            // Check if we don't have an encoded string of colors
            if (rleID < 128)
            {
                // Increase the count by 1
                rleID++;

                // Go through and read all the unique colors found
                while (rleID)
                {
                    // Read in the current color
                    fread(pColors, sizeof(BYTE) * channels, 1, pFile);

                    // Store the current pixel in our IMAGE array
                    pImg->pData[colorsRead + 0] = pColors[2];
                    pImg->pData[colorsRead + 1] = pColors[1];
                    pImg->pData[colorsRead + 2] = pColors[0];

                    // If we have a 4 channel 32-bit IMAGE, assign one more for the alpha
                    if (bits == 32)
                        pImg->pData[colorsRead + 3] = pColors[3];

                    // Increase the current pixels read, decrease the amount
                    // of pixels left, and increase the starting index for the next pixel.
                    i++;
                    rleID--;
                    colorsRead += channels;
                }
            }
            // Else, let's read in a string of the same character
            else
            {
                // Minus the 128 ID + 1 (127) to get the color count that needs to be read
                rleID -= 127;

                // Read in the current color, which is the same for a while
                fread(pColors, sizeof(BYTE) * channels, 1, pFile);

                // Go and read as many pixels as are the same
                while (rleID)
                {
                    // Assign the current pixel to the current index in our pixel array
                    pImg->pData[colorsRead + 0] = pColors[2];
                    pImg->pData[colorsRead + 1] = pColors[1];
                    pImg->pData[colorsRead + 2] = pColors[0];

                    // If we have a 4 channel 32-bit IMAGE, assign one more for the alpha
                    if (bits == 32)
                        pImg->pData[colorsRead + 3] = pColors[3];

                    // Increase the current pixels read, decrease the amount
                    // of pixels left, and increase the starting index for the next pixel.
                    i++;
                    rleID--;
                    colorsRead += channels;
                }
            }
        }
    }

    // Close the file pointer that opened the file
    fclose(pFile);

    // Fill in our IMAGE structure to pass back
    pImg->nChannels = channels;
    pImg->nWidth    = nWidth;
    pImg->nHeight   = nHeight;

    // Return the TGA data (remember, you must free this data after you are done)
    return pImg;
}

#ifdef _WINDOWS_H
IMAGE* LoadBMP(const char* szFileName, bool bFromResource)
{
    IMAGE* pImg = NULL;			// This stores our important IMAGE data
    HBITMAP hBMP;														// Handle Of The Bitmap
    BITMAP bmp;

    hBMP = (HBITMAP)LoadImage(GetModuleHandle(NULL), szFileName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | (bFromResource?0:LR_LOADFROMFILE));

    if (!hBMP)															// Does The Bitmap Exist?
        return NULL;

    GetObject(hBMP, sizeof(BITMAP), &bmp);									// Get The Object

    pImg = (IMAGE*)malloc(sizeof(IMAGE));
    if(pImg == NULL)
    {
        fprintf(stderr, "Memory allocation failed");
        return NULL;
    }
    pImg->nChannels = bmp.bmBitsPixel / 8;
    pImg->nWidth = bmp.bmWidth;
    pImg->nHeight = bmp.bmHeight;
    int bytescount = bmp.bmWidth * bmp.bmHeight * bmp.bmBitsPixel / 8;
    pImg->pData =  (unsigned char*) malloc(bytescount * sizeof(unsigned char));
    if(pImg->pData == NULL)
    {
        fprintf(stderr, "Memory allocation failed");
        return NULL;
    }
    memmove(pImg->pData, bmp.bmBits, bytescount);

    DeleteObject(hBMP);

    return pImg;
}
#endif // _WINDOWS_H

bool SaveBMP(IMAGE* pImg, const char* szFileName)
{
    FILE* pfile = fopen(szFileName, "wb");					// Open The BMP File
    if (pfile == NULL)										// Did The File Even Exist?
        return false;										// Return False

    int rowZeros = 4 - ((pImg->nWidth * pImg->nChannels) % 4);
    if(rowZeros==4)
        rowZeros = 0;

    BITMAPFILEHEADER bfh;
    bfh.bfType = 0x4d42; // "BM"
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + pImg->nWidth * pImg->nHeight * pImg->nChannels + rowZeros * pImg->nHeight;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bih;
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = pImg->nWidth;
    bih.biHeight = pImg->nHeight;
    bih.biPlanes = 1;
    bih.biBitCount = pImg->nChannels * 8;
    bih.biCompression = 0; //BI_RGB
    bih.biSizeImage = pImg->nWidth * pImg->nHeight * pImg->nChannels + rowZeros * pImg->nHeight;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, pfile);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, pfile);

    for (int i=0;i<pImg->nWidth * pImg->nHeight;i++)
    {
        if (pImg->nChannels >= 3)
        {
            fwrite(&pImg->pData[i * pImg->nChannels + 2], 1, 1, pfile); // swap red and blue
            fwrite(&pImg->pData[i * pImg->nChannels + 1], 1, 1, pfile);
            fwrite(&pImg->pData[i * pImg->nChannels + 0], 1, 1, pfile); // swap red and blue
        }
        if (pImg->nChannels == 4)
            fwrite(&pImg->pData[i * pImg->nChannels + 3], 1, 1, pfile);

        if((i+1)%pImg->nWidth == 0)
        {
            BYTE zero = 0;
            for(int i=0;i<rowZeros;i++)
                fwrite(&zero, 1, 1, pfile);
        }
    }

    fclose(pfile);

    return true;
}

bool SaveTGA(IMAGE* pImg, const char* szFileName)
{
    FILE* pfile = fopen(szFileName, "wb");					// Open The TGA File
    if (pfile == NULL)										// Did The File Even Exist?
        return false;										// Return False

    BYTE TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};		// Uncompressed TGA Header
    fwrite(TGAheader, 1, sizeof(TGAheader), pfile);

    BYTE header[6];										// First 6 Useful Bytes From The Header
    header[0] = LOBYTE(pImg->nWidth);
    header[1] = HIBYTE(pImg->nWidth);
    header[2] = LOBYTE(pImg->nHeight);
    header[3] = HIBYTE(pImg->nHeight);
    header[4] = pImg->nChannels * 8;
    header[5] = 0x08;
    fwrite(header,1,sizeof(header),pfile);

    for (int i=0;i<pImg->nWidth * pImg->nHeight;i++)
    {
        if (pImg->nChannels >= 3)
        {
            fwrite(&pImg->pData[i * pImg->nChannels + 2], 1, 1, pfile); // swap red and blue
            fwrite(&pImg->pData[i * pImg->nChannels + 1], 1, 1, pfile);
            fwrite(&pImg->pData[i * pImg->nChannels + 0], 1, 1, pfile); // swap red and blue
        }
        if (pImg->nChannels == 4)
            fwrite(&pImg->pData[i * pImg->nChannels + 3], 1, 1, pfile);
    }
    BYTE TGAfoot[8]={0,0,0,0,0,0,0,0};
    fwrite(TGAfoot,1,sizeof(TGAfoot),pfile);

    fwrite("TRUEVISION-XFILE.",1,18,pfile);

    fclose(pfile);

    return true;
}

void FreeImage(IMAGE Img)
{
    free(Img.pData);
    Img.pData = NULL;
}

void FreeImagePointer(IMAGE* pImg)
{
    free(pImg->pData);
    free(pImg);
    pImg = NULL;
}
