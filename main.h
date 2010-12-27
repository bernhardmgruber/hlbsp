#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <windows.h>  // Header for windows
#include <gl/gl.h>    // Header for OpenGL core functions
#include <gl/glu.h>	  // Header for OpenGl utility functions
#include <gl/glext.h> // Header for OpenGL extension definitions
#include "glsl.h"

#include "hlbsp.h"
#include "camera.h"
#include "hud.h"
#include "timer.h"
#include "3dmath.h"
#include "font.h"

extern PFNGLACTIVETEXTUREPROC   glActiveTexture;
extern PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f;

extern CBSP    g_bsp;
extern CCamera g_camera;
extern CHUD    g_hud;
extern CTimer  g_timer;

extern HDC       g_hDC;		 // Handle to device context from windows
extern HGLRC     g_hRC;		 // OpenGL rendering context
extern HWND      g_hWnd;		 // Handle to our window
extern HINSTANCE g_hInstance; // Handle to our application instance

extern bool g_abKeys[256];			// Array Used For The Keyboard Routine
extern bool g_bActive;		// Window Active Flag Set To true By Default
extern bool g_bFullscreen;	// Fullscreen Flag Set To Fullscreen Mode By Default

extern bool g_bTextures;
extern bool g_bLightmaps;
extern bool g_bPolygons;

extern bool g_bRenderStaticBSP;
extern bool g_bRenderBrushEntities;
extern bool g_bRenderSkybox;
extern bool g_bRenderDecals;
extern bool g_bRenderCoords;
extern bool g_bRenderHUD;

extern bool g_bShaderSupport;
extern bool g_bUseShader;
extern bool g_bNightvision;
extern bool g_bFlashlight;

extern bool g_bCaptureMouse;
extern VECTOR2D g_windowCenter;

extern bool g_bTexNPO2Support;

extern unsigned int g_nWinWidth;
extern unsigned int g_nWinHeight;

extern GLuint g_shpMain;

int MSGBOX_WARNING(const char* pszFormat, ...);
int MSGBOX_ERROR(const char* pszFormat, ...);
int LOG(const char* pszFormat, ...);
void* MALLOC(size_t nSize);

#endif // MAIN_H_INCLUDED
