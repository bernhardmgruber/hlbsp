/*
    Link against: gdi32, winmm, opengl32, glu32
*/
#include <windows.h>  // Header for windows
#include <gl/gl.h>    // Header for OpenGL core functions
#include <gl/glu.h>	  // Header for OpenGl utility functions
#include <gl/glext.h> // Header for OpenGL extension definitions
#include <math.h>
#include "hlbsp.h"
#include "camera.h"
#include "timer.h"
#include "inline.h"

#define WINDOW_CLASS_NAME "hlbsp"
#define WINDOW_TITLE "HL BSP"

#define BSP_DIR "data\\maps"
#define BSP_FILE_NAME "cs_assault.bsp"

//#define FULLSCREEN

#define WINDOW_HEIGHT 768
#define WINDOW_WIDTH 1024

PFNGLACTIVETEXTUREARBPROC   glActiveTextureARB   = NULL;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;

CBSP    g_bsp;
CCamera g_camera;

HDC	        g_hDC;		 // Handle to device context from windows
HGLRC		g_hRC;		 // OpenGL rendering context
HWND		g_hWnd;		 // Handle to our window
HINSTANCE	g_hInstance; // Handle to our application instance

bool g_abKeys[256];			// Array Used For The Keyboard Routine
bool g_bActive = true;		// Window Active Flag Set To true By Default
bool g_bFullscreen = true;	// Fullscreen Flag Set To Fullscreen Mode By Default

bool g_bTextures = true;
bool g_bPolygons = false;
bool g_bLightmaps = true;

bool g_bRenderStaticBSP = true;
bool g_bRenderBrushEntities = true;
bool g_bRenderSkybox = true;

bool g_bTexNPO2Support;

unsigned int g_nWinWidth = WINDOW_WIDTH;
unsigned int g_nWinHeight = WINDOW_HEIGHT;

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

GLvoid ReSizeGLScene(GLsizei nWidth, GLsizei nHeight)		// Resize And Initialize The GL Window
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
    gluPerspective(60.0f, (GLfloat)nWidth/(GLfloat)nHeight, 16.0f, 4000.0f);

    glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
    glLoadIdentity();									// Reset The Modelview Matrix
}

bool CheckForExtension(char* pszExtensionName)
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

    // Extensions
    LOG("Getting multitexture extension function pointers ...\n");
    if (CheckForExtension("GL_ARB_multitexture"))
    {
        // Obtain the functions entry point
        if ((glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB")) == NULL)
        {
            MSGBOX_ERROR("Error retrieving function pointer. glActiveTextureARB is not supported");
            return false;
        }
        if ((glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB")) == NULL)
        {
            MSGBOX_ERROR("Error retrieving function pointer. glMultiTexCoord2fARB is not supported");
            return false;
        }
    }
    else
    {
        MSGBOX_ERROR("GL_ARB_multitexture is not supported. Please upgrade your video driver.");
        return false;
    }

    LOG("Checking ARB_texture_non_power_of_two extension ...\n");
    g_bTexNPO2Support = CheckForExtension("GL_ARB_texture_non_power_of_two");
    if(g_bTexNPO2Support)
        LOG("Supported, no image scaling needed\n");
    else
        LOG("Not supported, lightmaps will be scaled to 16 x 16\n");

    if (!g_bsp.LoadBSPFile(BSP_DIR "\\" BSP_FILE_NAME))
        return false;

    // Get the players start pos
    LOG("Setting start position and angles...\n");
    CEntity* info_player_start = g_bsp.FindEntity("info_player_start");

    const char* pszOrigin = info_player_start->FindProperty("origin");
    if(pszOrigin != NULL)
    {
        int x, y, z;
        sscanf(pszOrigin, "%d %d %d", &x, &y, &z);
        g_camera.SetPosition(x, y, z);
    }

    const char* pszAngle = info_player_start->FindProperty("angle");
    if(pszAngle != NULL)
    {
        float fZAngle;
        sscanf(pszAngle, "%f", &fZAngle);
        g_camera.SetViewAngles(0.0f, fZAngle);
    }

    return true;										// Initialization Went OK
}

int DrawGLScene()									// Here's Where We Do All The Drawing
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

    glLoadIdentity();									// Reset The Current Modelview Matrix

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //Reset Color

    TimerTick();

    char szWindowText[256];
    sprintf(szWindowText, "%s - %.1f FPS", WINDOW_TITLE, g_fFPS);
    SetWindowText(g_hWnd, szWindowText);

    g_camera.UpdateView(g_dFrameInterval);
    g_camera.Look();

    g_bsp.RenderLevel(g_camera.GetPosition());

    ///Brightness
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

    return true;										// Everything Went OK
}

GLvoid KillGLWindow()								// Properly Kill The Window
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
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	szTitle			- Title To Appear At The Top Of The Window				*
 *	nWidth			- Width Of The GL Window Or Fullscreen Mode				*
 *	nHeight			- nHeight Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (true) Or Windowed Mode (false)	*/

BOOL CreateGLWindow(char* szTitle, int nWidth, int nHeight, int bits)
{
    InitTimer();
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
    if (!(g_hWnd=CreateWindowEx( dwExStyle,							// Extended Style For The Window
                                 WINDOW_CLASS_NAME,					// Class Name
                                 szTitle,							// Window Title
                                 dwStyle |							// Defined Window Style
                                 WS_CLIPSIBLINGS |					// Required Window Style
                                 WS_CLIPCHILDREN,					// Required Window Style
                                 0, 0,								// Window Position
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
    ReSizeGLScene(nWidth, nHeight);					// Set Up Our Perspective GL Screen

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

    TimerTick();
    LOG("##### Finished Initialization (%.3f s) #####\n", (float)g_dFrameInterval);

    return true;									// Success
}

LRESULT CALLBACK WndProc(HWND g_hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                if (!CreateGLWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 32))
                    PostQuitMessage(0);						// Quit If Window Was Not Created
                break;
            case 'L':
                g_bLightmaps = !g_bLightmaps;
                break;
            case 'T':
                g_bTextures = !g_bTextures;
                break;
            case 'P':
                g_bPolygons = !g_bPolygons;
                if (g_bPolygons)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                else
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            case '1':
                g_bRenderSkybox = !g_bRenderSkybox;
                break;
            case '2':
                g_bRenderStaticBSP = !g_bRenderStaticBSP;
                break;
            case '3':
                g_bRenderBrushEntities = !g_bRenderBrushEntities;
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
        ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=nHeight
        return 0;								// Jump Back
    }

    case WM_RBUTTONDOWN:
    {
        g_camera.CaptureMouse(true);
        ShowCursor(false);
        return 0;
    }

    case WM_RBUTTONUP:
    {
        g_camera.CaptureMouse(false);
        ShowCursor(true);
        return 0;
    }
    }

    // Pass All Unhandled Messages To DefWindowProc
    return DefWindowProc(g_hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(HINSTANCE g_hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef FULLSCREEN
    g_bFullscreen = true;
#else
    g_bFullscreen = false;
#endif

    // Create Our OpenGL Window
    if (!CreateGLWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, 32))
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
}
