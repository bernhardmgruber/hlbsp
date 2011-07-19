#include "hud.h"
#include "main.h"

#include <stdarg.h>
#include <stdio.h>

#define FONT_HUD_HEIGHT 12
#define FONT_HUD_SPACE 5
#define FONT_HUD_COLOR 1.0f ,0.0f, 0.0f

#define CONSOLE_WIDTH 400
#define CONSOLE_HEIGHT 300

extern CTimer g_timer;
extern CCamera g_camera;

extern unsigned int g_nWinWidth;
extern unsigned int g_nWinHeight;

GLuint g_nFontHUD;

void CHUD::Init()
{
    // Load fonts
    LOG("Creating font ...\n");
    #ifdef __WIN32__
    g_nFontHUD = CreateFont("System", FONT_HUD_HEIGHT);
    #else
    g_nFontHUD = CreateFont("helvetica", FONT_HUD_HEIGHT);
    #endif
}

void CHUD::Render()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0, g_nWinWidth, 0, g_nWinHeight, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int nCurrentY = g_nWinHeight;

    glColor3f(FONT_HUD_COLOR);
    glPrintf(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), g_nFontHUD, "FPS: %.1f", g_timer.fTPS);
    VECTOR3D pos = g_camera.GetPosition();
    glPrintf(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), g_nFontHUD, "Cam pos: %.1fx %.1fy %.1fz", pos.x, pos.y, pos.z);
    VECTOR2D view = g_camera.GetViewAngles();
    VECTOR3D viewVec = g_camera.GetViewVector();
    glPrintf(FONT_HUD_SPACE, nCurrentY -= (FONT_HUD_SPACE + FONT_HUD_HEIGHT), g_nFontHUD, "Cam view: %.1f°pitch %.1f°yaw (vec: %.1fx %.1fy %.1fz)", view.x, view.y, viewVec.x, viewVec.y, viewVec.z);

    /** console **/
    nCurrentY = FONT_HUD_SPACE;
    for(vector<string>::reverse_iterator it = console.rbegin(); it != console.rend() && nCurrentY + FONT_HUD_HEIGHT < CONSOLE_HEIGHT; ++it)
    {
        glPuts(FONT_HUD_SPACE, nCurrentY, g_nFontHUD, it->c_str());
        nCurrentY += FONT_HUD_HEIGHT + FONT_HUD_SPACE;
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void CHUD::Printf(const char* pszFormat, ...)
{
    if (pszFormat == NULL)
        return;

    char szText[512];								// Holds Our String
    va_list aParam;								    // Pointer To List Of Arguments
    // Do Nothing

    va_start(aParam, pszFormat);									// Parses The String For Variables
    vsprintf(szText, pszFormat, aParam);						// And Converts Symbols To Actual Numbers
    va_end(aParam);											// Results Are Stored In Text

    console.push_back(szText);
}
