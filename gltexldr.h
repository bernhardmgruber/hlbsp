#ifndef GLTEXTURELOADER_H_INCLUDED
#define GLTEXTURELOADER_H_INCLUDED

#include <windows.h>
#include <gl/gl.h>
#include "imageio.h"

#define CT_NEAREST  0x01
#define CT_LINEAR   0x02
#define CT_MIPMAP   0x04
#define LT_RESOURCE 0x08

bool LoadTexture(GLuint* texture, const char* fileName, unsigned char mode); // Loads texture form file to OpenGL
bool CreateTexture(GLuint* texture, IMAGE* img, unsigned char mode); // Loads texture from IMAGE to OpenGL

#endif // GLTEXTURELOADER_H_INCLUDED
