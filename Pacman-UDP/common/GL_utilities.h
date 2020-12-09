#ifndef _GL_UTILITIES_
#define _GL_UTILITIES_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
#else
	#if defined(_WIN32)
		#include "glew.h"
		#include <GL/gl.h>
	#else
		#include <GL/gl.h>
	#endif
#endif
//#include "MicroGlut.h"

char* readFile(char *file);

void printError(const char *functionName);
GLuint loadShaders(const char *vertFileName, const char *fragFileName);
GLuint loadShadersG(const char *vertFileName, const char *fragFileName, const char *geomFileName);
GLuint loadShadersGT(const char *vertFileName, const char *fragFileName, const char *geomFileName,
						const char *tcFileName, const char *teFileName);
void dumpInfo(void);

GLuint compileShaders(const char *vs, const char *fs, const char *gs, const char *tcs, const char *tes,
								const char *vfn, const char *ffn, const char *gfn, const char *tcfn, const char *tefn);

// This is obsolete! Use the functions in MicroGlut instead!
//void initKeymapManager();
//char keyIsDown(unsigned char c);

// FBO support

//------------a structure for FBO information-------------------------------------
typedef struct
{
	GLuint texid;
	GLuint fb;
	GLuint rb;
	GLuint depth;
	int width, height;
} FBOstruct;

FBOstruct *initFBO(int width, int height, int int_method);
FBOstruct *initFBO2(int width, int height, int int_method, int create_depthimage);
void useFBO(FBOstruct *out, FBOstruct *in1, FBOstruct *in2);
void updateScreenSizeForFBOHandler(int w, int h); // Temporary workaround to inform useFBO of screen size changes

#ifdef __cplusplus
}
#endif

#endif