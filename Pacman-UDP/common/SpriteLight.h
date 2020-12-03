// SpriteLight - Heavily simplified sprite engine
// by Ingemar Ragnemalm 2009
// Extended version 2013

// What does a mogwai say when it sees a can of soda?
// Eeek! Sprite light! Sprite light!

#ifndef SPRITELIGHT
#define SPRITELIGHT

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
#include "MicroGlut.h"

#include "LoadTGA.h"

typedef struct vec2
{
	GLfloat x, y;
} vec2;

typedef struct SpriteRec
{
	vec2 position;
	TextureData *face;
	vec2 speed;
	GLfloat rotation;
	struct SpriteRec *next;
	GLfloat alpha;
	
	// New fields for SpriteLight2
	void (*spriteTimer)(struct SpriteRec *sp, long time);
	void (*handleSprite)(struct SpriteRec *sp);
	void (*spriteCollide)(struct SpriteRec *sp, struct SpriteRec *otherSp);
	void (*drawSprite)(struct SpriteRec *sp);
	char remove;
	long timerInterval;
	long lastTime;
	GLfloat radius;
	vec2 scale;
	
	// Add custom sprite data here as needed
	int a, b, c;
	int kind;
	void *whatever;
	// Note that you can inherit SpriteRec to a larger record and have
	// NewSpriteExt allocating a larger area.
} SpriteRec, *SpritePtr;

// Globals: The sprite list, background texture and viewport dimensions (virtual or real pixels)
extern SpritePtr gSpriteRoot;
extern GLuint backgroundTexID;
extern long gWidth, gHeight;

// Functions
TextureData *GetFace(char *fileName);
void InitSprite(SpritePtr sp);
struct SpriteRec *NewSprite(TextureData *f, GLfloat h, GLfloat v, GLfloat hs, GLfloat vs);
void RemoveSprite(SpritePtr sp);
void HandleSprite(SpritePtr sp); // Default handler
void DrawSprite(SpritePtr sp); // Default handler
void DrawBackground();
void SetOrigin(GLfloat x, GLfloat y);
vec2 GetOrigin();
void SetSceneSize(GLfloat x, GLfloat y);
void SetCenter(GLfloat x, GLfloat y);
void SetYdirection(float d);
float GetYdirection();
GLuint GetDefaultVAO();
void DrawFace(TextureData *face, vec2 position, GLfloat rotation); // Face drawing routine for drawing in background

//void InitSpriteLight();
void InitSpriteLight(int argc, char **argv, int w, int h, char *title);

void SetBackgroundDrawingProc(void (myBackground)());
GLuint GetDefaultShader();

int GetRandom(int range);

#endif
