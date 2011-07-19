/*
    Link against:
	Windows: opengl32, glu32, gdi32, sdl
    Linux: SDL, GL, GLU
*/
#include "main.h"

#include <math.h>
#include "hlbsp.h"
#include "camera.h"
#include "timer.h"
#include "font.h"
#include "hud.h"

#define WINDOW_CLASS_NAME "hlbsp"
#define WINDOW_CAPTION "HL BSP"

#define BSP_DIR "data/maps"
#define BSP_FILE_NAME "cs_assault.bsp"

//#define FULLSCREEN

#define WINDOW_HEIGHT 768
#define WINDOW_WIDTH 1024

CBSP    g_bsp;
CCamera g_camera;
CHUD    g_hud;
CTimer  g_timer;
CPlayer g_player;

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
bool g_bUseShader = true;
bool g_bNightvision = false;
bool g_bFlashlight = false;

bool g_bCaptureMouse = false;
VECTOR2D g_windowCenter;

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

bool glslShaderSourceFile(GLuint uObj, const char* pszFileName)
{
    //read in shader source
    FILE* pShaderFile = fopen(pszFileName, "rb");
    if (pShaderFile == NULL)
        return false;

    fseek(pShaderFile, 0, SEEK_END);
    size_t nFileSize= ftell(pShaderFile);
    fseek(pShaderFile, 0, SEEK_SET);

    char* pszShaderSource = (char*) malloc(sizeof(char) * (nFileSize + 1)); // zero at end of string
    if(pszShaderSource == NULL)
    {
        printf("Memory allocation failed\n");
        return false;
    }

    size_t nResult = fread(pszShaderSource, sizeof(char), nFileSize, pShaderFile);

    if(nResult != nFileSize)
    {
        printf("Error reading Shader %s\n", pszFileName);
        return false;
    }

    pszShaderSource[nFileSize] = 0;

    glShaderSource(uObj, 1, (const char**)&pszShaderSource, NULL);
    printf("Read shader from file %s\n", pszFileName);

    free(pszShaderSource);
    fclose(pShaderFile);

    return true;
}

void glslPrintProgramInfoLog(GLuint uObj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetProgramiv(uObj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        if(infoLog == NULL)
        {
            printf("Memory allocation failed\n");
            return;
        }
        glGetProgramInfoLog(uObj, infologLength, &charsWritten, infoLog);
        if(strcmp(infoLog, ""))
            printf("%s\r", infoLog);
        else
            printf("(no program info log)\n");
        free(infoLog);
    }
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

    //
    // Extensions
    //

    LOG("Checking extensions ...\n");

    LOG("GL_ARB_multitexture ...");
    if(!GLEE_ARB_multitexture)
    {
        MSGBOX_ERROR("GL_ARB_multitexture is not supported. Please upgrade your video driver.");
        return false;
    }
    LOG(" OK\n");

    LOG("GL_ARB_texture_non_power_of_two ...");
    if(GLEE_ARB_texture_non_power_of_two)
        LOG(" OK (no lightmap scaling needed)\n");
    else
        LOG(" NOT SUPPORTED (lightmaps will be scaled to 16 x 16)\n");

    LOG("GLSL Shaders ...");
    if (GLEE_ARB_shader_objects &&
        GLEE_ARB_shading_language_100 &&
        GL_ARB_vertex_shader &&
        GL_ARB_fragment_shader)
    {
        LOG(" OK\n");
        g_bShaderSupport = true;
    }
    else
    {
        LOG(" NOT SUPPORTED (Several features will not be available)\n");
        g_bShaderSupport = false;
    }

    //
    // Shader
    //

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

        glslPrintProgramInfoLog(g_shpMain);
    }

    // turn off shader if not supported
    if(!g_bShaderSupport)
        g_bUseShader = false;

    //
    // BSP file
    //

    if (!g_bsp.LoadBSPFile(BSP_DIR "/" BSP_FILE_NAME))
        return false;

    //
    // lighting for compelex flashlight
    //

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

    //
    // Create local player
    //

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

    //
    // HUD
    //

    g_hud.Init();

    return true;
}

void Update()
{
    // update timeer interval
    g_timer.Tick();

    char szWindowText[256];
    sprintf(szWindowText, "%s - %.1f FPS", WINDOW_CAPTION, g_timer.fTPS);
    SDL_WM_SetCaption(szWindowText, szWindowText);

    // Player
    g_player.Update(g_timer.dInterval);

    // Camera
    g_camera.Update(g_timer.dInterval);
}

/*void DrawCube(float a)
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
}*/

/*void DrawPlane(BSPPLANE p)
{
    VECTOR3D v = p.vNormal * p.fDist;
    glPointSize(10.0f);
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_POINTS);
    glVertex3f(v.x, v.y, v.z);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(v.x, v.y, v.z);
    glVertex3f(v.x + p.vNormal.x * 20,v.y + p.vNormal.y * 20,v.z + p.vNormal.z * 20);
    glEnd();
}*/

void DrawGLScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

    // Set up camera
    glLoadIdentity();
    g_camera.Look();

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

    // Coords
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

    // Leaf outlines
    if(g_bRenderLeafOutlines)
        g_bsp.RenderLeavesOutlines();

    // HUD
    if (g_bRenderHUD)
    {
        g_hud.Render();
    }
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

	// vsync
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    unsigned int flags = SDL_HWSURFACE | SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE;
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
                    Update();
                    DrawGLScene();
                    SDL_GL_SwapBuffers();
                }
            }
        }
    }

    SDL_Quit();

    return 0;
}
