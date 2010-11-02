#ifndef IMAGEIO_H_INCLUDED
#define IMAGEIO_H_INCLUDED

#include <windows.h> // optional, for portability, need for gltexldr.h

#ifndef _WINDOWS_H
#ifndef BYTE
#define BYTE  unsigned char  // 8 bit memory, unsigned
#endif
#ifndef WORD
#define WORD  unsigned short //16 bit memory, unsigned
#endif
#ifndef DWORD
#define DWORD unsigned long //32 bit memory, unsigned
#endif
#ifndef LONG
#define LONG  long  //32 bit memory, signed
#endif
#endif // _WINDOWS_H

#ifndef STRUCT_IMAGE
#define STRUCT_IMAGE
typedef struct _IMAGE
{
	BYTE nChannels; // The channels in the image (3 = RGB, 4 = RGBA)
	WORD nWidth;    // The width of the image in pixels
	WORD nHeight;   // The height of the image in pixels
	BYTE* pData;   // The image pixel data
} IMAGE;
#endif

IMAGE* CreateImage(BYTE nChannels, WORD nWidth, WORD nHeight); //Creates an empty image

bool GetPixel(IMAGE* pImg, WORD x, WORD y, BYTE* pRed, BYTE* pGreen, BYTE* pBlue, BYTE* pAlpha); //if channels is 3, alpha is ignored
bool SetPixel(IMAGE* pImg, WORD x, WORD y, BYTE red, BYTE green, BYTE blue, BYTE alpha); //if channels is 3, alpha is ignored

IMAGE* LoadTGA(const char* szFileName);
#ifdef _WINDOWS_H
IMAGE* LoadBMP(const char* szFileName, bool bFromResource);
#endif // _WINDOWS_H

bool SaveBMP(IMAGE* pImg, const char* szFileName); //Saves an image to a BMP File
bool SaveTGA(IMAGE* pImg, const char* szFileName); //Saves an image to a TGA File

void FreeImage(IMAGE Img);
void FreeImagePointer(IMAGE* pImg);

/* Pixel stored: R G B A
 *
 *    y
 *
 *  3 |x x x x
 *  2 |x x x x
 *  1 |x x x x
 *  0 |x x x x
 *     - - - -  x
 *     0 1 2 3
 */


#endif // IMAGEIO_H_INCLUDED
