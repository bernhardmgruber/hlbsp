#ifndef UNFONT_H_INCLUDED
#define UNFONT_H_INCLUDED

#include <gl/gl.h>

GLuint CreateFont(const char* pszFontName, int nFontHeight);
void DeleteFont(GLuint uFont);

void glPrintf(int nX, int nY, GLuint uFont, const char* pszFormat, ...);
void glPuts(int nX, int nY, GLuint uFont, const char* pszText);

#endif // UNFONT_H_INCLUDED
