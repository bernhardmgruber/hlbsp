/*
    Link against: opengl32, glu32
*/
#include "main.h"

#include <math.h>
#include "hlbsp.h"
#include "camera.h"
#include "timer.h"
#include "font.h"
#include "glsl.h"
#include "hud.h"

#define WINDOW_CLASS_NAME "hlbsp"
#define WINDOW_CAPTION "HL BSP"

#define BSP_DIR "data/maps"
#define BSP_FILE_NAME "test.bsp"

//#define FULLSCREEN

#define WINDOW_HEIGHT 768
#define WINDOW_WIDTH 1024

#ifdef __WIN32__
PFNGLACTIVETEXTUREPROC   glActiveTexture   = NULL;
PFNGLMULTITEXCOORD2FPROC glMultiTexCoord2f = NULL;
#endif

CBSP    g_bsp;
CCamera g_camera;
CHUD    g_hud;
CTimer  g_timer;
CPlayer g_player;

//HDC	        g_hDC;		 // Handle to device context from windows
//HGLRC		g_hRC;		 // OpenGL rendering context
//HWND		g_hWnd;		 // Handle to our window
//HINSTANCE	g_hInstance; // Handle to our application instance

//bool g_abKeys[256];			// Array Used For The Keyboard Routine
bool g_abKeys[SDLK_LAST];
bool g_bActive = true;		// Window Active Flag Set To true By Default
bool g_bFullscreen = true;	// Fullscreen Flag Set To Fullscreen Mode By Default

bool g_bTextures = true;
bool g_bLightmaps = true;
bool g_bPolygons = false;

bool g_bRenderStaticBSP = true;
bool g_bRenderBrushEntities = true;
bool g_bRenderSkybox = true;
bool g_bRenderDecals = true;
bool g_bRenderCoords = false;
bool g_bRenderLeafOutlines = false;
bool g_bRenderHUD = true;

bool g_bShaderSupport;
bool g_bUseShader = false;
bool g_bNightvision = false;
bool g_bFlashlight = false;

bool g_bCaptureMouse = false;
VECTOR2D g_windowCenter;

bool g_bTexNPO2Support;

unsigned int g_nWinWidth = WINDOW_WIDTH;
unsigned int g_nWinHeight = WINDOW_HEIGHT;

GLuint g_shpMain;

//LRESULT CALLBACK WndProc(HWND g_hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int MSGBOX_WARNING(const char* pszFormat, ...)
{
    if (pszFormat == NULL)
        return 0;

    #ifdef __WIN32__
    char szText[1024];

    va_list aParam;
    va_start(aParam, pszFormat);
    vsprintf(szText, pszFormat, aParam);
    va_end(aParam);

    return MessageBox(NULL, szText, "WARNING", MB_OK | MB_ICONWARNING);
    #else
    int ret;
    va_list aParam;
    va_start(aParam, pszFormat);
    ret = vprintf(pszFormat, aParam);
    va_end(aParam);

    return ret;
    #endif
}

int MSGBOX_ERROR(const char* pszFormat, ...)
{
    if (pszFormat == NULL)
        return 0;

    #ifdef __WIN32__
    char szText[1024];

    va_list aParam;
    va_start(aParam, pszFormat);
    vsprintf(szText, pszFormat, aParam);
    va_end(aParam);

    return MessageBox(NULL, szText, "ERROR", MB_OK | MB_ICONERROR);
    #else
    int ret;
    va_list aParam;
    va_start(aParam, pszFormat);
    ret = vprintf(pszFormat, aParam);
    va_end(aParam);

    return ret;
    #endif
}

int LOG(const char* pszFormat, ...)
{
    int ret;
    va_list args;
    va_start(args, pszFormat);
    ret = vprintf(pszFormat, args);
    va_end(args);
    return ret;
}

void* MALLOC(size_t nSize)
{
    void* pMemory = malloc(nSize);
    if (pMemory == NULL)
        MSGBOX_ERROR("Memory allocation of %d bytes failed", nSize);

    return pMemory;
}

void ResizeGLScene(int nWidth, int nHeight)		// Resize And Initialize The GL Window
{
    g_nWinWidth = nWidth;
    g_nWinHeight = nHeight;
    LOG("Window dimensions changed to %d x %d\n", nWidth, nHeight);

    if (nHeight == 0)										// Prevent A Divide By Zero By
    {
        nHeight = 1;										// Making nHeight Equal One
    }

    glViewport(0, 0, nWidth, nHeight);						// Reset The Current Viewport

    glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
    glLoadIdentity();									// Reset The Projection Matrix

    // Calculate The Aspect Ratio Of The Window
    gluPerspective(60.0f, (GLfloat)nWidth/(GLfloat)nHeight, 8.0f, 4000.0f);

    glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
    glLoadIdentity();									// Reset The Modelview Matrix
}

bool CheckExtension(const char* pszExtensionName)
{
    // get the list of supported extensions
    char* pszExtensionList = (char*) glGetString(GL_EXTENSIONS);

    if (!pszExtensionName || !pszExtensionList)
        return false;

    while (*pszExtensionList)
    {
        // find the length of the first extension substring
        unsigned int nCurExtensionLength = strcspn(pszExtensionList, " ");

        if (strlen(pszExtensionName) == nCurExtensionLength && strncmp(pszExtensionName, pszExtensionList, nCurExtensionLength) == 0)
            return true;

        // move to the next substring
        pszExtensionList += nCurExtensionLength + 1;
    }

    return false;
}

int InitGL()										// All Setup For OpenGL Goes Here
{
    LOG("Setting rendering states ...\n");
    glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
    glClearDepth(1.0f);									// Depth Buffer Setup
    glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glEnable(GL_MULTISAMPLE);

    // Extensions
    #ifdef __WIN32__
    LOG("Getting multitexture extension function pointers ...\n");
    if (CheckExtension("GL_ARB_multitexture"))
    {
        // Obtain the functions entry point
        if ((glActiveTexture = (PFNGLACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress("glActiveTexture")) == NULL)
        {
            MSGBOX_ERROR("Error retrieving function pointer. glActiveTexture is not supported");
            return false;
        }
        if ((glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FARBPROC)SDL_GL_GetProcAddress("glMultiTexCoord2f")) == NULL)
        {
            MSGBOX_ERROR("Error retrieving function pointer. glMultiTexCoord2f is not supported");
            return false;
        }
    }
    else
    {
        MSGBOX_ERROR("GL_ARB_multitexture is not supported. Please upgrade your video driver.");
        return false;
    }
    #endif

    LOG("Checking ARB_texture_non_power_of_two extension ...\n");
    g_bTexNPO2Support = CheckExtension("GL_ARB_texture_non_power_of_two");
    if(g_bTexNPO2Support)
        LOG("Supported, no image scaling needed\n");
    else
        LOG("Not supported, lightmaps will be scaled to 16 x 16\n");

    // BSP file
    if (!g_bsp.LoadBSPFile(BSP_DIR "/" BSP_FILE_NAME))
        return false;

    // Shader
    LOG("Checking GLSL shader support ...\n");
    if (!glslCheckSupport())
    {
        MSGBOX_WARNING("GLSL shaders are not supported. Several features will not be available.");
        g_bShaderSupport = false;
    }
    else
    {
        if (!glslInitProcs())
        {
            MSGBOX_ERROR("Error retrieving shader function pointers. Several features will not be available.");
            g_bShaderSupport = false;
        }
        g_bShaderSupport = true;
    }

    if (g_bShaderSupport)
    {
        LOG("Loading shaders ...\n");
        GLuint vsMain = glCreateShader(GL_VERTEX_SHADER);
        GLuint fsMain = glCreateShader(GL_FRAGMENT_SHADER);

        if (!glslShaderSourceFile(vsMain, "shader/main.vert"))
            return false;
        if (!glslShaderSourceFile(fsMain, "shader/main.frag"))
            return false;

        glCompileShader(vsMain);
        glCompileShader(fsMain);

        //PROGRAM
        g_shpMain = glCreateProgram();
        glAttachShader(g_shpMain, vsMain);
        glAttachShader(g_shpMain, fsMain);

        glLinkProgram(g_shpMain);

        //LOG("Program Info Log:\n");
        glslPrintProgramInfoLog(g_shpMain);
    }

    // turn off shader if not supported
    if(!g_bShaderSupport)
        g_bUseShader = false;

    // lighting for compelex flashlight
    glEnable(GL_LIGHT0);

    // set light position
    GLfloat lightPos[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0,GL_POSITION,lightPos);

    // set spot light parameters
    GLfloat spotDir[] = {0.0f, 0.0f, -1.0f};            	// define spot direction
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDir);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 25.0f);    	// set cutoff angle
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.0f);   	// set focusing strength
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.01f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.01f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0001f);

    // Create local player
    LOG("Creating local player ...\n");
    CEntity* info_player_start = g_bsp.FindEntity("info_player_start");

    const char* pszOrigin = info_player_start->FindProperty("origin");
    if(pszOrigin != NULL)
    {
        int x, y, z;
        sscanf(pszOrigin, "%d %d %d", &x, &y, &z);
        g_player.SetPosition(x, y, z);
    }

    const char* pszAngle = info_player_start->FindProperty("angle");
    if(pszAngle != NULL)
    {
        float fZAngle;
        sscanf(pszAngle, "%f", &fZAngle);
        g_player.SetViewAngles(0.0f, fZAngle);
    }

    g_camera.BindPlayer(&g_player);

    // HUD
    g_hud.Init();

    return true;										// Initialization Went OK
}

void DrawCube(float a)
{
    float a2 = a / 2.0f;

    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
		// Front Face
		glVertex3f(-a2, -a2,  a2);	// Bottom Left Of The Texture and Quad
		glVertex3f( a2, -a2,  a2);	// Bottom Right Of The Texture and Quad
		glVertex3f( a2,  a2,  a2);	// Top Right Of The Texture and Quad
		glVertex3f(-a2,  a2,  a2);	// Top Left Of The Texture and Quad
		// Back Face
		glVertex3f(-a2, -a2, -a2);	// Bottom Right Of The Texture and Quad
		glVertex3f(-a2,  a2, -a2);	// Top Right Of The Texture and Quad
		glVertex3f( a2,  a2, -a2);	// Top Left Of The Texture and Quad
		glVertex3f( a2, -a2, -a2);	// Bottom Left Of The Texture and Quad
		// Top Face
		glVertex3f(-a2,  a2, -a2);	// Top Left Of The Texture and Quad
		glVertex3f(-a2,  a2,  a2);	// Bottom Left Of The Texture and Quad
		glVertex3f( a2,  a2,  a2);	// Bottom Right Of The Texture and Quad
		glVertex3f( a2,  a2, -a2);	// Top Right Of The Texture and Quad
		// Bottom Face
		glVertex3f(-a2, -a2, -a2);	// Top Right Of The Texture and Quad
		glVertex3f( a2, -a2, -a2);	// Top Left Of The Texture and Quad
		glVertex3f( a2, -a2,  a2);	// Bottom Left Of The Texture and Quad
		glVertex3f(-a2, -a2,  a2);	// Bottom Right Of The Texture and Quad
		// Right face
		glVertex3f( a2, -a2, -a2);	// Bottom Right Of The Texture and Quad
		glVertex3f( a2,  a2, -a2);	// Top Right Of The Texture and Quad
		glVertex3f( a2,  a2,  a2);	// Top Left Of The Texture and Quad
		glVertex3f( a2, -a2,  a2);	// Bottom Left Of The Texture and Quad
		// Left Face
		glVertex3f(-a2, -a2, -a2);	// Bottom Left Of The Texture and Quad
		glVertex3f(-a2, -a2,  a2);	// Bottom Right Of The Texture and Quad
		glVertex3f(-a2,  a2,  a2);	// Top Right Of The Texture and Quad
		glVertex3f(-a2,  a2, -a2);	// Top Left Of The Texture and Quad
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
}

int DrawGLScene()									// Here's Where We Do All The Drawing
{
    /** UPDATE SCENE **/
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
    glLoadIdentity();									// Reset The Current Modelview Matrix

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f); //Reset Color

    g_timer.Tick();

    char szWindowText[256];
    sprintf(szWindowText, "%s - %.1f FPS", WINDOW_CAPTION, g_timer.fTPS);
    SDL_WM_SetCaption(szWindowText, szWindowText);

    // Player
    g_player.UpdateFromInput(g_timer.dInterval);

    // Camera
    g_camera.UpdateFromInput(g_timer.dInterval);
    g_camera.Look();

    /** DRAW SCENE **/
    // Enable Shader
    if (g_bUseShader)
    {
        glUseProgram(g_shpMain);

        glUniform1i(glGetUniformLocation(g_shpMain, "tex1"), 0);
        glUniform1i(glGetUniformLocation(g_shpMain, "tex2"), 1);
        glUniform1i(glGetUniformLocation(g_shpMain, "bNightvision"), g_bNightvision);
        glUniform1i(glGetUniformLocation(g_shpMain, "bFlashlight"), g_bFlashlight);
    }

    g_bsp.RenderLevel(g_camera.GetPosition());

    // Disable Shader
    if (g_bUseShader)
        glUseProgram(0);

    /// Brightness
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ONE);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(-15.0f, -15.0f, -17.0f);
    glVertex3f(-15.0f,  15.0f, -17.0f);
    glVertex3f( 15.0f,  15.0f, -17.0f);
    glVertex3f( 15.0f, -15.0f, -17.0f);
    glEnd();
    glDisable(GL_BLEND);
    glPopMatrix();

    if (g_bRenderCoords)
    {
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glColor3f(1.0f,0.0f,0.0f); //red X+
        glVertex3i(4000,0,0);
        glVertex3i(0,0,0);
        glColor3f(0.0f,1.0f,0.0f); //green Y+
        glVertex3i(0,4000,0);
        glVertex3i(0,0,0);
        glColor3f(0.0f,0.0f,1.0f); //blue Z+
        glVertex3i(0,0,4000);
        glVertex3i(0,0,0);
        glEnd();

        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glColor3f(0.0f,0.4f,0.0f); //green Y-
        glVertex3i(0,0,0);
        glVertex3i(0,-4000,0);
        glColor3f(0.4f,0.0f,0.0f); //red X-
        glVertex3i(0,0,0);
        glVertex3i(-4000,0,0);
        glColor3f(0.0f,0.0f,0.4f); //blue Z-
        glVertex3i(0,0,0);
        glVertex3i(0,0,-4000);
        glEnd();
    }

    /// Leaf outlines
    if(g_bRenderLeafOutlines)
        g_bsp.RenderLeafOutlines();


    glBegin(GL_LINE_LOOP);
    glVertex3f(156.1,298.7,-48.0);
    glColor3f(1.0f, 0, 0);
    glVertex3f(157.1,306.6,-56.0);
    glEnd();

    glPushMatrix();
    glTranslatef(156.1,298.7,-48.0);
    glColor3f(1, 1, 1);
    DrawCube(32.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(157.1,306.6,-56.0);
    glColor3f(1.0f, 0, 0);
    DrawCube(32.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(157.1,306.6,-48.0);
    glColor3f(0.0f, 1, 0);
    DrawCube(32.0f);
    glPopMatrix();

    /// HUD
    if (g_bRenderHUD)
    {
        g_hud.Render();
    }

    return true;										// Everything Went OK
}

bool CreateSDLWindow(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        MSGBOX_ERROR("Could not initialize SDL");
        return false;
    };

    SDL_WM_SetCaption(WINDOW_CAPTION, WINDOW_CAPTION);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // enable 2x anti-aliasing
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    unsigned int flags = SDL_OPENGL | SDL_RESIZABLE;
    if(g_bFullscreen)
        flags |= SDL_FULLSCREEN;

    SDL_SetVideoMode(width, height, 0, flags);

    InitGL();
    ResizeGLScene(width, height);

    return true;
}

void ProcessEvent(SDL_Event event)
{
    switch(event.type)
    {
    case SDL_APPACTIVE:
        if((event.active.state & SDL_APPINPUTFOCUS) && (event.active.gain == 0))
            g_bActive = false;
        else
            g_bActive = true;
        return;

    case SDL_KEYDOWN:
        g_abKeys[event.key.keysym.sym] = true;

        switch(event.key.keysym.sym)
        {
        case SDLK_TAB:
            g_camera.SetMoveSens(CAMERA_MOVE_SENS * 3.0f);
            break;

        case SDLK_LSHIFT:
            g_camera.SetMoveSens(CAMERA_MOVE_SENS / 3.0f);
            break;

        case SDLK_F1:
            SDL_Quit();						// Kill Our Current Window
            g_bFullscreen =! g_bFullscreen;				// Toggle Fullscreen / Windowed Mode
            if(g_bFullscreen)
                LOG("Changing to fullscreen ...\n");
            else
                LOG("Changing to windowed mode ...\n");

            // Recreate Our OpenGL Window
            if (!CreateSDLWindow(WINDOW_WIDTH, WINDOW_HEIGHT))
                SDL_Quit();
            break;

        case SDLK_F2:
            if(g_bShaderSupport)
            {
                g_bUseShader = !g_bUseShader;
                if(g_bUseShader)
                    g_hud.Printf("shaders enabled");
                else
                    g_hud.Printf("shaders disabled");
            }
            else
                g_hud.Printf("shaders are not supported");
            break;

        case SDLK_F5:
        {
            IMAGE* pImg = CreateImage(3, g_nWinWidth, g_nWinHeight);
            glReadPixels(0, 0, pImg->nWidth, pImg->nHeight, GL_RGB, GL_UNSIGNED_BYTE, pImg->pData);

            //get filename
            char szFileName[512];

            for (int i=1;; i++)
            {
                sprintf(szFileName, "screenshots/Screenshot%d.bmp", i);
                FILE* pFile;
                if ((pFile = fopen(szFileName, "rb")) == NULL)
                    break;
                fclose(pFile);
            }

            SaveBMP(pImg, szFileName);
            FreeImagePointer(pImg);
            break;
        }

        case SDLK_c:
            g_bRenderCoords = !g_bRenderCoords;
            if(g_bRenderCoords)
                g_hud.Printf("coords enabled");
            else
                g_hud.Printf("coords disabled");
            break;

        case SDLK_h:
            g_bRenderHUD = !g_bRenderHUD;
            if(g_bRenderHUD)
                g_hud.Printf("hud enabled");
            else
                g_hud.Printf("hud disabled");
            break;

        case SDLK_l:
            g_bLightmaps = !g_bLightmaps;
            if(g_bLightmaps)
                g_hud.Printf("lightmaps enabled");
            else
                g_hud.Printf("lightmaps disabled");
            break;

        case SDLK_n:
            g_bNightvision = !g_bNightvision;
            if(g_bNightvision)
                g_hud.Printf("nightvision enabled");
            else
                g_hud.Printf("nightvision disabled");
            break;

        case SDLK_t:
            g_bTextures = !g_bTextures;
            if(g_bTextures)
                g_hud.Printf("textures enabled");
            else
                g_hud.Printf("textures disabled");
            break;

        case SDLK_o:
            g_bRenderLeafOutlines = !g_bRenderLeafOutlines;
            if(g_bRenderLeafOutlines)
                g_hud.Printf("leaf outlines enabled");
            else
                g_hud.Printf("leaf outlines disabled");
            break;

        case SDLK_p:
            g_bPolygons = !g_bPolygons;
            if (g_bPolygons)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                g_hud.Printf("polygon mode set to line");
            }
            else
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                g_hud.Printf("polygon mode set to fill");
            }
            break;

        case SDLK_v:
            g_bFlashlight = !g_bFlashlight;
            if(g_bFlashlight)
                g_hud.Printf("flashlight enabled");
            else
                g_hud.Printf("flashlight disabled");
            break;

        case SDLK_1:
            g_bRenderSkybox = !g_bRenderSkybox;
            if(g_bRenderSkybox)
                g_hud.Printf("skybox enabled");
            else
                g_hud.Printf("skybox disabled");
            break;

        case SDLK_2:
            g_bRenderStaticBSP = !g_bRenderStaticBSP;
            if(g_bRenderStaticBSP)
                g_hud.Printf("static geometry enabled");
            else
                g_hud.Printf("static geometry  disabled");
            break;

        case SDLK_3:
            g_bRenderBrushEntities = !g_bRenderBrushEntities;
            if(g_bRenderBrushEntities)
                g_hud.Printf("entities enabled");
            else
                g_hud.Printf("entities disabled");
            break;

        case SDLK_4:
            g_bRenderDecals = !g_bRenderDecals;
            if(g_bRenderDecals)
                g_hud.Printf("decals enabled");
            else
                g_hud.Printf("decals disabled");
            break;

        default:
            break;
        }
        return;

    case SDL_KEYUP:
        g_abKeys[event.key.keysym.sym] = false;

        switch(event.key.keysym.sym)
        {
        case SDLK_TAB:
        case SDLK_LSHIFT:
            g_camera.SetMoveSens(CAMERA_MOVE_SENS);
            break;

        default:
            break;
        }
        return;

    case SDL_VIDEORESIZE:
        ResizeGLScene(event.resize.w, event.resize.h);
        return;

    case SDL_MOUSEBUTTONDOWN:
        switch(event.button.button)
        {
            case SDL_BUTTON_RIGHT:
                g_bCaptureMouse = true;
                //POINT pt;
                //GetCursorPos(&pt);
                int x, y;
                SDL_GetMouseState(&x, &y);

                g_windowCenter.x = x;
                g_windowCenter.y = y;
                //ShowCursor(false);
                SDL_ShowCursor(SDL_DISABLE);
                break;
        }
        return;

    case SDL_MOUSEBUTTONUP:
        switch(event.button.button)
        {
            case SDL_BUTTON_RIGHT:
                g_bCaptureMouse = false;
                //ShowCursor(true);
                SDL_ShowCursor(SDL_ENABLE);
                break;
        }
        return;
    }
}

#ifdef __WIN32__
#undef main
#endif

int main(int argc, char **argv )
{
#ifdef FULLSCREEN
    g_bFullscreen = true;
#else
    g_bFullscreen = false;
#endif

    if (!CreateSDLWindow(WINDOW_WIDTH, WINDOW_HEIGHT))
        return 0;

    SDL_Event event;
    bool bDone = false;
    while (!bDone)
    {
        if (SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
                bDone = true;
            else
                ProcessEvent(event);
        }
        else
        {
            if (g_bActive)
            {
                if (g_abKeys[SDLK_ESCAPE])
                {
                    bDone = true;
                }
                else
                {
                    DrawGLScene();
                    SDL_GL_SwapBuffers();
                }
            }

            /*if (g_abKeys[SDLK_F1])
            {
                g_abKeys[SDLK_F1] = false;
                SDL_Quit();
                g_bFullscreen = !g_bFullscreen;

                if (!CreateSDLWindow(WINDOW_WIDTH, WINDOW_HEIGHT))
                    return 0;
            }*/
        }
    }

    SDL_Quit();
    return 0;
}

/*void KillGLWindow()								// Properly Kill The Window
{
    g_bsp.Destroy();

    if (g_bFullscreen)										// Are We In Fullscreen Mode?
    {
        ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
    }

    if (g_hRC)											// Do We Have A Rendering Context?
    {
        if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
        {
            MSGBOX_ERROR("Release Of DC And RC Failed.");
        }

        if (!wglDeleteContext(g_hRC))						// Are We Able To Delete The RC?
        {
            MSGBOX_ERROR("Release Rendering Context Failed.");
        }
        g_hRC=NULL;										// Set RC To NULL
    }

    if (g_hDC && !ReleaseDC(g_hWnd,g_hDC))					// Are We Able To Release The DC
    {
        MSGBOX_ERROR("Release Device Context Failed.");
        g_hDC=NULL;										// Set DC To NULL
    }

    if (g_hWnd && !DestroyWindow(g_hWnd))					// Are We Able To Destroy The Window?
    {
        MSGBOX_ERROR("Could Not Release g_hWnd.");
        g_hWnd=NULL;										// Set g_hWnd To NULL
    }

    if (!UnregisterClass(WINDOW_CLASS_NAME, g_hInstance))			// Are We Able To Unregister Class
    {
        MSGBOX_ERROR("Could Not Unregister Class.");
        g_hInstance=NULL;									// Set g_hInstance To NULL
    }
}*/

/*bool CreateGLWindow(const char* szTitle, int nWidth, int nHeight, int bits)
{
    LOG("##### Initialization #####\n");

    GLuint		PixelFormat;			// Holds The Results After Searching For A Match
    WNDCLASS	wc;						// Windows Class Structure
    DWORD		dwExStyle;				// Window Extended Style
    DWORD		dwStyle;				// Window Style
    RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
    WindowRect.left=(long)0;			// Set Left Value To 0
    WindowRect.right=(long)nWidth;		// Set Right Value To Requested Width
    WindowRect.top=(long)0;				// Set Top Value To 0
    WindowRect.bottom=(long)nHeight;		// Set Bottom Value To Requested nHeight

    g_hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
    wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;	// Redraw On Size, And Own DC For Window.
    wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
    wc.cbClsExtra		= 0;									// No Extra Window Data
    wc.cbWndExtra		= 0;									// No Extra Window Data
    wc.hInstance		= g_hInstance;							// Set The Instance
    wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
    wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
    wc.hbrBackground	= NULL;									// No Background Required For GL
    wc.lpszMenuName		= NULL;									// We Don't Want A Menu
    wc.lpszClassName	= WINDOW_CLASS_NAME;					// Set The Class Name

    LOG("Registering window class ...\n");
    if (!RegisterClass(&wc))									// Attempt To Register The Window Class
    {
        MSGBOX_ERROR("Failed To Register The Window Class.");
        return false;											// Return false
    }

    if (g_bFullscreen)												// Attempt Fullscreen Mode?
    {
        DEVMODE dmScreenSettings;								// Device Mode
        memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
        dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
        dmScreenSettings.dmPelsWidth	= nWidth;				// Selected Screen Width
        dmScreenSettings.dmPelsHeight	= nHeight;				// Selected Screen nHeight
        dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
        dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

        // Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
        LOG("Changing display settings ...\n");
        if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
        {
            // If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
            if (MessageBox(NULL,"The requested fullscreen mode is not supported by your video card. Use windowed mode instead?","Not supported",MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
                g_bFullscreen = false;		// Windowed Mode Selected.  Fullscreen = false
            else
                return false;									// Return false
        }
    }

    if (g_bFullscreen)												// Are We Still In Fullscreen Mode?
    {
        dwExStyle = WS_EX_APPWINDOW;								// Window Extended Style
        dwStyle = WS_POPUP;										// Windows Style
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
        dwStyle = WS_OVERLAPPEDWINDOW;							// Windows Style
    }

    LOG("Adjusting window size ...\n");
    AdjustWindowRectEx(&WindowRect, dwStyle, false, dwExStyle);		// Adjust Window To True Requested Size

    // Create The Window
    LOG("Creating window ...\n");
    if (!(g_hWnd = CreateWindowEx( dwExStyle,							// Extended Style For The Window
                                   WINDOW_CLASS_NAME,					// Class Name
                                   szTitle,							// Window Title
                                   dwStyle |							// Defined Window Style
                                   WS_CLIPSIBLINGS |					// Required Window Style
                                   WS_CLIPCHILDREN,					// Required Window Style
                                   CW_USEDEFAULT, CW_USEDEFAULT,		// Window Position
                                   WindowRect.right-WindowRect.left,	// Calculate Window Width
                                   WindowRect.bottom-WindowRect.top,	// Calculate Window nHeight
                                   NULL,								// No Parent Window
                                   NULL,								// No Menu
                                   g_hInstance,						// Instance
                                   NULL)))							// Dont Pass Anything To WM_CREATE
    {
        KillGLWindow();								// Reset The Display
        MSGBOX_ERROR("Window Creation Error");
        return false;								// Return false
    }

    PIXELFORMATDESCRIPTOR pfd =                     // pfd Tells Windows How We Want Things To Be
    {
        sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
        1,											// Version Number
        PFD_DRAW_TO_WINDOW |						// Format Must Support Window
        PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
        PFD_DOUBLEBUFFER,							// Must Support Double Buffering
        PFD_TYPE_RGBA,								// Request An RGBA Format
        bits,										// Select Our Color Depth
        0, 0, 0, 0, 0, 0,							// Color Bits Ignored
        0,											// No Alpha Buffer
        0,											// Shift Bit Ignored
        0,											// No Accumulation Buffer
        0, 0, 0, 0,									// Accumulation Bits Ignored
        16,											// 16Bit Z-Buffer (Depth Buffer)
        0,											// No Stencil Buffer
        0,											// No Auxiliary Buffer
        PFD_MAIN_PLANE,								// Main Drawing Layer
        0,											// Reserved
        0, 0, 0										// Layer Masks Ignored
    };

    LOG("Getting handle to device context ...\n");
    if (!(g_hDC=GetDC(g_hWnd)))							// Did We Get A Device Context?
    {
        KillGLWindow();								// Reset The Display
        MSGBOX_ERROR("Can't get the device contest");
        return false;								// Return false
    }

    LOG("Choosing pixel format ...\n");
    if (!(PixelFormat=ChoosePixelFormat(g_hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
    {
        KillGLWindow();								// Reset The Display
        MSGBOX_ERROR("Can't Find A Suitable PixelFormat.");
        return false;								// Return false
    }

    LOG("Setting pixel format ...\n");
    if (!SetPixelFormat(g_hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
    {
        KillGLWindow();								// Reset The Display
        MSGBOX_ERROR("Can't Set The PixelFormat");
        return false;								// Return false
    }

    LOG("Creating GL rendering context ...\n");
    if (!(g_hRC=wglCreateContext(g_hDC)))				// Are We Able To Get A Rendering Context?
    {
        KillGLWindow();								// Reset The Display
        MSGBOX_ERROR("Can't Create A GL Rendering Context");
        return false;								// Return false
    }

    LOG("Activating GL rendering context ...\n");
    if (!wglMakeCurrent(g_hDC,g_hRC))					// Try To Activate The Rendering Context
    {
        KillGLWindow();								// Reset The Display
        MSGBOX_ERROR("Can't Activate The GL Rendering Context");
        return false;								// Return false
    }

    LOG("Setting up projection ...\n");
    ResizeGLScene(nWidth, nHeight);					// Set Up Our Perspective GL Screen

    if (!InitGL())									// Initialize Our Newly Created GL Window
    {
        KillGLWindow();								// Reset The Display
        MSGBOX_ERROR("Initialization Failed");
        return false;								// Return false
    }

    LOG("Opening Window ...\n");
    ShowWindow(g_hWnd, SW_SHOW);						// Show The Window
    SetForegroundWindow(g_hWnd);						// Slightly Higher Priority
    SetFocus(g_hWnd);									// Sets Keyboard Focus To The Window

    g_timer.Tick();
    LOG("##### Finished Initialization (%.3f s) #####\n", (float)g_timer.dInterval);

    return true;									// Success
}*/

/*LRESULT CALLBACK WndProc(HWND g_hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)									// Check For Windows Messages
    {
    case WM_ACTIVATE:							// Watch For Window Activate Message
    {
        if (!HIWORD(wParam))					// Check Minimization State
            g_bActive = true;						// Program Is Active
        else
            g_bActive = false;						// Program Is No Longer Active

        return 0;								// Return To The Message Loop
    }

    case WM_SYSCOMMAND:							// Intercept System Commands
    {
        switch (wParam)							// Check System Calls
        {
        case SC_SCREENSAVE:					// Screensaver Trying To Start?
        case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
            return 0;							// Prevent From Happening
        }
        break;									// Exit
    }

    case WM_CLOSE:								// Did We Receive A Close Message?
    {
        PostQuitMessage(0);						// Send A Quit Message
        return 0;								// Jump Back
    }

    case WM_KEYDOWN:							// Is A Key Being Held Down?
    {
        g_abKeys[wParam] = true;					// If So, Mark It As true

        switch(wParam)
        {
        case VK_TAB:
            g_camera.SetMoveSens(CAMERA_MOVE_SENS * 3.0f);
            break;
        case VK_SHIFT:
            g_camera.SetMoveSens(CAMERA_MOVE_SENS / 3.0f);
            break;
        case VK_F1:
            KillGLWindow();						// Kill Our Current Window
            g_bFullscreen =! g_bFullscreen;				// Toggle Fullscreen / Windowed Mode
            if(g_bFullscreen)
                LOG("Changing to fullscreen ...\n");
            else
                LOG("Changing to windowed mode ...\n");

            // Recreate Our OpenGL Window
            if (!CreateGLWindow(WINDOW_CAPTION, WINDOW_WIDTH, WINDOW_HEIGHT, 32))
                PostQuitMessage(0);						// Quit If Window Was Not Created
            break;
        case VK_F2:
            if(g_bShaderSupport)
            {
                g_bUseShader = !g_bUseShader;
                if(g_bUseShader)
                    g_hud.Printf("shaders enabled");
                else
                    g_hud.Printf("shaders disabled");
            }
            else
                g_hud.Printf("shaders are not supported");
            break;
        case VK_F5:
        {
            IMAGE* pImg = CreateImage(3, g_nWinWidth, g_nWinHeight);
            glReadPixels(0, 0, pImg->nWidth, pImg->nHeight, GL_RGB, GL_UNSIGNED_BYTE, pImg->pData);

            //get filename
            char szFileName[512];

            for (int i=1;; i++)
            {
                sprintf(szFileName, "screenshots/Screenshot%d.bmp", i);
                FILE* pFile;
                if ((pFile = fopen(szFileName, "rb")) == NULL)
                    break;
                fclose(pFile);
            }

            SaveBMP(pImg, szFileName);
            FreeImagePointer(pImg);
            break;
        }
        case 'C':
            g_bRenderCoords = !g_bRenderCoords;
            if(g_bRenderCoords)
                g_hud.Printf("coords enabled");
            else
                g_hud.Printf("coords disabled");
            break;
        case 'H':
            g_bRenderHUD = !g_bRenderHUD;
            if(g_bRenderHUD)
                g_hud.Printf("hud enabled");
            else
                g_hud.Printf("hud disabled");
            break;
        case 'L':
            g_bLightmaps = !g_bLightmaps;
            if(g_bLightmaps)
                g_hud.Printf("lightmaps enabled");
            else
                g_hud.Printf("lightmaps disabled");
            break;
        case 'N':
            g_bNightvision = !g_bNightvision;
            if(g_bNightvision)
                g_hud.Printf("nightvision enabled");
            else
                g_hud.Printf("nightvision disabled");
            break;
        case 'T':
            g_bTextures = !g_bTextures;
            if(g_bTextures)
                g_hud.Printf("textures enabled");
            else
                g_hud.Printf("textures disabled");
            break;
        case 'P':
            g_bPolygons = !g_bPolygons;
            if (g_bPolygons)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                g_hud.Printf("polygon mode set to line");
            }
            else
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                g_hud.Printf("polygon mode set to fill");
            }
            break;
        case 'V':
            g_bFlashlight = !g_bFlashlight;
            if(g_bFlashlight)
                g_hud.Printf("flashlight enabled");
            else
                g_hud.Printf("flashlight disabled");
            break;
        case '1':
            g_bRenderSkybox = !g_bRenderSkybox;
            if(g_bRenderSkybox)
                g_hud.Printf("skybox enabled");
            else
                g_hud.Printf("skybox disabled");
            break;
        case '2':
            g_bRenderStaticBSP = !g_bRenderStaticBSP;
            if(g_bRenderStaticBSP)
                g_hud.Printf("static geometry enabled");
            else
                g_hud.Printf("static geometry  disabled");
            break;
        case '3':
            g_bRenderBrushEntities = !g_bRenderBrushEntities;
            if(g_bRenderBrushEntities)
                g_hud.Printf("entities enabled");
            else
                g_hud.Printf("entities disabled");
            break;
        case '4':
            g_bRenderDecals = !g_bRenderDecals;
            if(g_bRenderDecals)
                g_hud.Printf("decals enabled");
            else
                g_hud.Printf("decals disabled");
            break;
        }

        return 0;								// Jump Back
    }

    case WM_KEYUP:								// Has A Key Been Released?
    {
        g_abKeys[wParam] = false;					// If So, Mark It As false

        switch(wParam)
        {
        case VK_TAB:
        case VK_SHIFT:
            g_camera.SetMoveSens(CAMERA_MOVE_SENS);
            break;

        }

        return 0;								// Jump Back
    }

    case WM_SIZE:								// Resize The OpenGL Window
    {
        ResizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=nHeight
        return 0;								// Jump Back
    }

    case WM_RBUTTONDOWN:
    {
        g_bCaptureMouse = true;
        POINT pt;
        GetCursorPos(&pt);
        g_windowCenter.x = pt.x;
        g_windowCenter.y = pt.y;
        ShowCursor(false);
        return 0;
    }

    case WM_RBUTTONUP:
    {
        g_bCaptureMouse = false;
        ShowCursor(true);
        return 0;
    }
    }

    // Pass All Unhandled Messages To DefWindowProc
    return DefWindowProc(g_hWnd,uMsg,wParam,lParam);
}*/

/*int WINAPI WinMain(HINSTANCE g_hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef FULLSCREEN
    g_bFullscreen = true;
#else
    g_bFullscreen = false;
#endif

    // Create Our OpenGL Window
    if (!CreateGLWindow(WINDOW_CAPTION, WINDOW_WIDTH, WINDOW_HEIGHT, 32))
        return 0;									// Quit If Window Was Not Created

    MSG		msg;									// Windows Message Structure
    BOOL	bDone = false;								// Bool Variable To Exit Loop
    while (!bDone)									// Loop That Runs While done=false
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))	// Is There A Message Waiting?
        {
            if (msg.message == WM_QUIT)				// Have We Received A Quit Message?
            {
                bDone = true;							// If So done=true
            }
            else									// If Not, Deal With Window Messages
            {
                TranslateMessage(&msg);				// Translate The Message
                DispatchMessage(&msg);				// Dispatch The Message
            }
        }
        else										// If There Are No Messages
        {
            // Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
            if (g_bActive)								// Program Active?
            {
                if (g_abKeys[VK_ESCAPE])				// Was ESC Pressed?
                {
                    bDone = true;						// ESC Signalled A Quit
                }
                else								// Not Time To Quit, Update Screen
                {
                    DrawGLScene();					// Draw The Scene
                    SwapBuffers(g_hDC);				// Swap Buffers (Double Buffering)
                }
            }
        }
    }

    // Shutdown
    KillGLWindow();									// Kill The Window
    return 0;							// Exit The Program
}*/
