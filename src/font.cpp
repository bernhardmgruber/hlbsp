#include "font.h"

#include <SDL2/SDL.h>

#ifdef __WIN32__
#include <windows.h>
#else
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

GLuint createFont(const char* name, int height) {
	GLuint uBase = glGenLists(96); // Storage For 96 Characters

#ifdef __WIN32__
	HFONT hFont = CreateFont(-height, // Height Of Font
		0,                                 // Width Of Font
		0,                                 // Angle Of Escapement
		0,                                 // Orientation Angle
		FW_MEDIUM,                         // Font Weight
		FALSE,                             // Italic
		FALSE,                             // Underline
		FALSE,                             // Strikeout
		ANSI_CHARSET,                      // Character Set Identifier
		OUT_TT_PRECIS,                     // Output Precision
		CLIP_DEFAULT_PRECIS,               // Clipping Precision
		ANTIALIASED_QUALITY,               // Output Quality
		FF_DONTCARE | DEFAULT_PITCH,       // Family And Pitch
		name);                      // Font Name

	if (hFont == nullptr) {
		printf("Could not load font %s\n", name);
		return uBase;
	}

	HDC hDC = GetDC(nullptr);

	HFONT hOldFont = (HFONT)SelectObject(hDC, hFont); // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, uBase);            // Builds 96 Characters Starting At Character 32

	SelectObject(hDC, hOldFont); // restore font
	DeleteObject(hFont);         // Delete The Font
#else
	Display* pDpl = XOpenDisplay(nullptr);

	char szFont[256];
	sprintf(szFont, "-*-%s-medium-r-*-*-%d-*-*-*-*-*-*-*", name, height);

	XFontStruct* fontInfo = XLoadQueryFont(pDpl, szFont);

	if (fontInfo == nullptr) {
		printf("Could not load font %s\n", name);
		return uBase;
	}

	glXUseXFont(fontInfo->fid, 32, 96, uBase);

	XFreeFont(pDpl, fontInfo);
#endif

	return uBase;
}

void deleteFont(GLuint font) {
	glDeleteLists(font, 96); // Delete All 96 Characters
}

void glPuts(int x, int y, GLuint font, const std::string& text) {
	glRasterPos2i(x, y);

	glPushAttrib(GL_LIST_BIT);
	glListBase(font - 32);
	glCallLists(text.size(), GL_UNSIGNED_BYTE, text.data());
	glPopAttrib();
}
