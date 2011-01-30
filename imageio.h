#ifndef IMAGEIO_H_INCLUDED
#define IMAGEIO_H_INCLUDED

//#include <windows.h>
#include <stdint.h>

#ifndef STRUCT_IMAGE
#define STRUCT_IMAGE
typedef struct _IMAGE
{
	unsigned char nChannels; // The channels in the image (3 = RGB, 4 = RGBA)
	unsigned short nWidth;    // The width of the image in pixels
	unsigned short nHeight;   // The height of the image in pixels
	unsigned char* pData;   // The image pixel data
} IMAGE;
#endif

IMAGE* CreateImage(unsigned char nChannels, unsigned short nWidth, unsigned short nHeight); //Creates an empty image

//bool GetPixel(IMAGE* pImg, unsigned short x, unsigned short y, unsigned char* pRed, unsigned char* pGreen, unsigned char* pBlue, unsigned char* pAlpha); //if channels is 3, alpha is ignored
//bool SetPixel(IMAGE* pImg, unsigned short x, unsigned short y, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha); //if channels is 3, alpha is ignored

IMAGE* LoadTGA(const char* szFileName);
#ifdef __WIN32__
IMAGE* LoadBMP(const char* szFileName, bool bFromResource);
#endif

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
