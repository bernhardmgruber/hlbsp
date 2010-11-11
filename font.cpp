#include "font.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

GLuint CreateFont(const char* pszFontName, int nFontHeight)
{
    GLuint uBase = glGenLists(96);						// Storage For 96 Characters

	HFONT hFont = CreateFont(-nFontHeight,				// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_NORMAL,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE | DEFAULT_PITCH,	// Family And Pitch
						pszFontName);					// Font Name

    HDC hDC = GetDC(NULL);

	HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, uBase); // Builds 96 Characters Starting At Character 32

	SelectObject(hDC, hOldFont);							// restore font
	DeleteObject(hFont);									// Delete The Font

    return uBase;
}

void DeleteFont(GLuint uFont)
{
	glDeleteLists(uFont, 96);							// Delete All 96 Characters
}

void glPrintf(int nX, int nY, GLuint uFont, const char* pszFormat, ...)
{
    glRasterPos2i(nX, nY);

	char szText[256];								// Holds Our String
	va_list aParam;								    // Pointer To List Of Arguments

	if (pszFormat == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(aParam, pszFormat);									// Parses The String For Variables
	    vsprintf(szText, pszFormat, aParam);						// And Converts Symbols To Actual Numbers
	va_end(aParam);											// Results Are Stored In Text

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(uFont - 32); // Sets The Base Character to 32
	glCallLists(strlen(szText), GL_UNSIGNED_BYTE, szText);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}

void glPuts(int nX, int nY, GLuint uFont, const char* pszText)
{
    glRasterPos2i(nX, nY);

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(uFont - 32); // Sets The Base Character to 32
	glCallLists(strlen(pszText), GL_UNSIGNED_BYTE, pszText);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
}
