#ifndef GLSL_H_INCLUDED
#define GLSL_H_INCLUDED

//#include <gl/gl.h>
//#include <gl/glext.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

/* GL Functions */
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;

extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;

extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;

extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;

extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation; //GLint glGetUniformLocation(GLuint program, const char *name);
extern PFNGLUNIFORM1FPROC glUniform1f; //void glUniform1f(GLint location, GLfloat v0);
extern PFNGLUNIFORM2FPROC glUniform2f; //void glUniform2f(GLint location, GLfloat v0, GLfloat v1);
extern PFNGLUNIFORM3FPROC glUniform3f; //void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern PFNGLUNIFORM4FPROC glUniform4f; //void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern PFNGLUNIFORM1FVPROC glUniform1fv; //GLint glUniform1fv(GLint location, GLsizei count, const GLfloat *v);
extern PFNGLUNIFORM2FVPROC glUniform2fv; //GLint glUniform2fv(GLint location, GLsizei count, const GLfloat *v);
extern PFNGLUNIFORM3FVPROC glUniform3fv; //GLint glUniform3fv(GLint location, GLsizei count, const GLfloat *v);
extern PFNGLUNIFORM4FVPROC glUniform4fv; //GLint glUniform4fv(GLint location, GLsizei count, const GLfloat *v);
extern PFNGLUNIFORM1IPROC glUniform1i; //void glUniform1i(GLint location, GLint v0);
extern PFNGLUNIFORM2IPROC glUniform2i; //void glUniform2i(GLint location, GLint v0, GLint v1);
extern PFNGLUNIFORM3IPROC glUniform3i; //void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
extern PFNGLUNIFORM4IPROC glUniform4i; //void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern PFNGLUNIFORM1IVPROC glUniform1iv; //GLint glUniform1iv(GLint location, GLsizei count, const GLint *v);
extern PFNGLUNIFORM2IVPROC glUniform2iv; //GLint glUniform2iv(GLint location, GLsizei count, const GLint *v);
extern PFNGLUNIFORM3IVPROC glUniform3iv; //GLint glUniform3iv(GLint location, GLsizei count, const GLint *v);
extern PFNGLUNIFORM4IVPROC glUniform4iv; //GLint glUniform4iv(GLint location, GLsizei count, const GLint *v);
extern PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv; //GLint glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, GLfloat *v);
extern PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv; //GLint glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, GLfloat *v);
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv; //GLint glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, GLfloat *v);

/* Own Functions */
bool glslCheckSupport();
bool glslInitProcs();

bool glslShaderSourceFile(GLuint uObj, const char* pszFileName);

void glslPrintShaderInfoLog(GLuint uObj);
void glslPrintProgramInfoLog(GLuint uObj);

#endif // GLSL_H_INCLUDED
