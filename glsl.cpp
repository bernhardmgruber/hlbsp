#include "glsl.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;

PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;

PFNGLDETACHSHADERPROC glDetachShader;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;

PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;

PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM2IPROC glUniform2i;
PFNGLUNIFORM3IPROC glUniform3i;
PFNGLUNIFORM4IPROC glUniform4i;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLUNIFORM2IVPROC glUniform2iv;
PFNGLUNIFORM3IVPROC glUniform3iv;
PFNGLUNIFORM4IVPROC glUniform4iv;
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;

char* pszExtensions = NULL;

bool IsExtensionSupported(const char* pszExtensionName)
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

bool glslCheckSupport()
{
    //Shader Extension
    if (!IsExtensionSupported("GL_ARB_shader_objects"))
    {
        //MessageBox(NULL, "GL_ARB_shader_objects is not supported", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!IsExtensionSupported("GL_ARB_shading_language_100"))
    {
        //MessageBox(NULL, "GL_ARB_shading_language_100 is not supported", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!IsExtensionSupported("GL_ARB_vertex_shader"))
    {
        //MessageBox(NULL, "GL_ARB_vertex_shader is not supported", "Error", MB_OK | MB_ICONERROR);
        return false;
    }
    if (!IsExtensionSupported("GL_ARB_fragment_shader"))
    {
        //MessageBox(NULL, "GL_ARB_fragment_shader is not supported", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

bool glslInitProcs()
{
    //Shader Functions
    glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
    if (glCreateShader == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glCreateShader", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
    if (glShaderSource == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glShaderSource", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
    if (glCompileShader == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glCompileShader", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
    if (glCreateProgram == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glCreateProgram", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
    if (glAttachShader == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glAttachShader", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
    if (glLinkProgram == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glLinkProgram", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    if (glUseProgram == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUseProgram", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
    if (glDetachShader == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glDetachShader", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
    if (glDeleteShader == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glDeleteShader", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
    if (glDeleteProgram == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glDeleteProgram", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
    if (glGetShaderiv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glGetShaderiv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
    if (glGetProgramiv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glGetProgramiv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
    if (glGetShaderInfoLog == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glGetShaderInfoLog", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    if (glGetProgramInfoLog == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glGetProgramInfoLog", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    if (glGetUniformLocation == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glGetUniformLocation", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    if (glUniform1f == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform1f", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
    if (glUniform2f == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform2f", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
    if (glUniform3f == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform3f", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
    if (glUniform4f == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform4f", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform1fv = (PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform1fv");
    if (glUniform1fv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform1fv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform2fv = (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv");
    if (glUniform2fv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform2fv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
    if (glUniform3fv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform3fv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
    if (glUniform4fv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform4f", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
    if (glUniform1i == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform1i", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i");
    if (glUniform2i == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform2i", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform3i = (PFNGLUNIFORM3IPROC)wglGetProcAddress("glUniform3i");
    if (glUniform3i == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform3i", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform4i = (PFNGLUNIFORM4IPROC)wglGetProcAddress("glUniform4i");
    if (glUniform4i == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform4i", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform1iv = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv");
    if (glUniform1iv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform1iv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform2iv = (PFNGLUNIFORM2IVPROC)wglGetProcAddress("glUniform2iv");
    if (glUniform2iv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform2iv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform3iv = (PFNGLUNIFORM3IVPROC)wglGetProcAddress("glUniform3iv");
    if (glUniform3iv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform3iv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniform4iv = (PFNGLUNIFORM4IVPROC)wglGetProcAddress("glUniform4iv");
    if (glUniform4iv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniform4iv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)wglGetProcAddress("glUniformMatrix2fv");
    if (glUniformMatrix2fv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniformMatrix2fv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)wglGetProcAddress("glUniformMatrix3fv");
    if (glUniformMatrix3fv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniformMatrix3fv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
    if (glUniformMatrix4fv == NULL)
    {
        //MessageBox(NULL, "Error retrieving funtion pointer: glUniformMatrix4fv", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
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

void glslPrintShaderInfoLog(GLuint uObj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetShaderiv(uObj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        if(infoLog == NULL)
        {
            printf("Memory allocation failed\n");
            return;
        }
        glGetShaderInfoLog(uObj, infologLength, &charsWritten, infoLog);
        if(strcmp(infoLog, ""))
            printf("%s", infoLog);
        else
            printf("(no shader info log)\n");
        free(infoLog);
    }
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
            printf("%s", infoLog);
        else
            printf("(no program info log)\n");
        free(infoLog);
    }
}
