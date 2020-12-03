// SpriteLight - Heavily simplified sprite engine
// by Ingemar Ragnemalm 2009
// Converted to OpenGL 3.2+ 2013.
// Sprite Light 2 2014 - extended to a more useful but still
// very simple engine. Made for the Internetworking course 2014.

// What does a mogwai say when it sees a can of soda?
// Eeek! Sprite light! Sprite light!

// 2015-02-18: Fixed a bug in deallocation of sprites! Important!
// 2015-02-19: Added alpha field in sprites, for fading. Better shader variable access (don't look up locations over and over).

#include "SpriteLight.h"
#include "LoadTGA.h"
#include <math.h>
#include "VectorUtils3.h"
#include "GL_utilities.h"
#include <time.h>
#include <stdlib.h>

// Globals: The sprite list, background texture and viewport dimensions (virtual or real pixels)
SpritePtr gSpriteRoot = NULL;
GLuint backgroundTexID = 0;
long gWidth=800, gHeight=600;
// Scene size as defined by SetSceneSize.
// If gMaxX is zero, this is considered undefined.
// If not, SetOrigin and the default HandleSprite are affected.
GLfloat gMaxX = 0;
GLfloat gMaxY = 0;
// Origin. Can be changed for scrolling.
vec2 gOrigin = {0.0, 0.0};
// Y direction. The default is downwards. (Upwards Y is hardly tested.)
float gYdirection = -1.0;

// vertex array object
unsigned int vertexArrayObjID;
// Reference to shader program
GLuint gDefaultShader;

GLuint alphaLoc; // = glGetUniformLocation(gDefaultShader, "alpha");
GLuint mLoc; // = glGetUniformLocation(gDefaultShader, "m");

void (*gCustomDrawBackground)();

// Uncomment if you are on a system without fabs
//GLfloat fabs(GLfloat in)
//{
//  if (in<0) return -1*in;
//  else return in;
//}
//
// OTOH... why don't I just:
// #define abs(a) (a>0?a:-a)

TextureData *GetFace(char *fileName)
{
	TextureData *fp;

	fp = (TextureData *)malloc(sizeof(TextureData));
	
	if (!LoadTGATexture(fileName, fp)) return NULL;
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	printf("Loaded %s\n", fileName);
	return fp;
}

// Essential inits for custom sprites
// You must allocate the data yourself! The whole point with the function is
// to make it possible to use extended sprite records!
void InitSprite(SpritePtr sp)
{
	sp->next = gSpriteRoot;
	gSpriteRoot = sp;
	
	sp->remove = 0; // Set to 1 to remove sprite!
	sp->handleSprite = NULL;
	sp->spriteCollide = NULL;
	sp->drawSprite = NULL;
	sp->spriteTimer = NULL;
	sp->lastTime = glutGet(GLUT_ELAPSED_TIME);
	sp->alpha = 1.0;
// Moved from NewSprite
	sp->rotation = 0;
	sp->scale.x = 1.0;
	sp->scale.y = 1.0;
}

// Create a "standard" sprite
struct SpriteRec *NewSprite(TextureData *f, GLfloat x, GLfloat y, GLfloat hs, GLfloat vs)
{
	SpritePtr sp;
	sp = (SpriteRec *)malloc(sizeof(SpriteRec));
	InitSprite(sp);

	sp->position.x = x;
	sp->position.y = y;
	sp->speed.x = hs;
	sp->speed.y = vs;
	sp->face = f;
//	sp->rotation = 0;
//	sp->scale.x = 1.0;
//	sp->scale.y = 1.0;

//	printf("C size = %i\n", sizeof(SpriteRec));

	return sp;
}

void RemoveSprite(SpritePtr sp)
{
	sp->remove = 1;
}

// A simple movement
void HandleSprite(SpritePtr sp)
{
// Move by speed, bounce off screen edges.
	sp->position.x += sp->speed.x;
	sp->position.y += sp->speed.y;
	if (sp->position.x < 0)
	{
		sp->speed.x = fabs(sp->speed.x);
		sp->position.x = 0;
	}
	if (sp->position.y < 0)
	{
		sp->speed.y = fabs(sp->speed.y);
		sp->position.y = 0;
	}
	if (gMaxX == 0) // No limits defined, use screen size
	{
		if (sp->position.x > gWidth)
		{
			sp->speed.x = -fabs(sp->speed.x);
			sp->position.x = gWidth;
		}
		if (sp->position.y > gHeight)
		{
			sp->speed.y = -fabs(sp->speed.y);
			sp->position.y = gHeight;
		}
	}
	else
	{
		if (sp->position.x > gMaxX)
		{
			sp->speed.x = -fabs(sp->speed.x);
			sp->position.x = gMaxX;
		}
		if (sp->position.y > gMaxY)
		{
			sp->speed.y = -fabs(sp->speed.y);
			sp->position.y = gMaxY;
		}
	}
	
	sp->rotation = atan2(sp->speed.y, sp->speed.x) * 180.0/3.1416;
}

#define max(a, b) (a>b?a:b)

// Default drawing routine
void DrawSprite(SpritePtr sp)
{
	mat4 trans, rot, scale, m;
	if (sp->face == NULL) return;
	
	glUseProgram(gDefaultShader);
	// Update matrices
	scale = S((float)sp->face->width/gWidth * 2 * sp->scale.x, (float)sp->face->height/gHeight * 2 * sp->scale.y, 1);
//	trans = T(sp->position.x/gWidth, sp->position.y/gHeight, 0);
	trans = T((sp->position.x - gOrigin.x)/gWidth * 2 - 1, gYdirection*((sp->position.y - gOrigin.y)/gHeight * 2 - 1), 0);
	rot = Rz(sp->rotation * 3.14 / 180);
	m = Mult(trans, Mult(scale, rot));
	
	glUniformMatrix4fv(mLoc, 1, GL_TRUE, m.m);
	glBindTexture(GL_TEXTURE_2D, sp->face->texID);

	if (sp->alpha < 1.0)
		glUniform1f(alphaLoc, sp->alpha);

	// Draw
	glBindVertexArray(vertexArrayObjID);	// Select VAO
	glDrawArrays(GL_TRIANGLES, 0, 6);	// draw object

	if (sp->alpha <1.0)
		glUniform1f(alphaLoc, 1.0);

	// Store the current face size as radius, since this might change over time.
	// Or rather save as a polygon...
	sp->radius = max(sp->face->width * sp->scale.x, sp->face->height * sp->scale.y) / 2.0;
}

// Face drawing routine for drawing in background
void DrawFace(TextureData *face, vec2 position, GLfloat rotation)
{
	mat4 trans, rot, scale, m;
	
	// Update matrices
	glUseProgram(gDefaultShader);
	
	scale = S((float)face->width/gWidth * 2, (float)face->height/gHeight * 2, 1);
//	trans = T(position.x/gWidth, sp->position.y/gHeight, 0);
	trans = T((position.x - gOrigin.x)/gWidth * 2 - 1, gYdirection*((position.y - gOrigin.y)/gHeight * 2 - 1), 0);
	rot = Rz(rotation * 3.14 / 180);
	m = Mult(trans, Mult(scale, rot));
	
	glUniformMatrix4fv(mLoc, 1, GL_TRUE, m.m);
	glBindTexture(GL_TEXTURE_2D, face->texID);
	
	// Draw
	glBindVertexArray(vertexArrayObjID);	// Select VAO
	glDrawArrays(GL_TRIANGLES, 0, 6);	// draw object
}

void DrawBackground()
{
	mat4 scale;
	
	glUseProgram(gDefaultShader);
	glBindTexture(GL_TEXTURE_2D, backgroundTexID);
	// Update matrices
	scale = Mult(T(-2*gOrigin.x/gWidth, -2*gOrigin.y/gWidth, 0.0), S(2, 2, 1));
	glUniformMatrix4fv(mLoc, 1, GL_TRUE, scale.m);
	
	// Draw
	glBindVertexArray(vertexArrayObjID);	// Select VAO
	glDrawArrays(GL_TRIANGLES, 0, 6);	// draw object
}

void SetBackgroundDrawingProc(void (*myBackground)())
{
	gCustomDrawBackground = myBackground;
}

GLuint GetDefaultShader()
{
	return gDefaultShader;
}

// Drawing routine and callbacks
// This is the most central part of the engine.
void Display()
{
	SpritePtr sp;
	long time;
	SpritePtr a, b;
		SpritePtr prev = NULL;
	SpritePtr current = NULL;

	glClearColor(0, 0, 0.2, 1);
	glClear(GL_COLOR_BUFFER_BIT+GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (gCustomDrawBackground)
		gCustomDrawBackground();
	else
		DrawBackground();
	
// Loop though all sprites. This requires several loops for a couple of different tasks.
	time = glutGet(GLUT_ELAPSED_TIME);
	sp = gSpriteRoot;
	if (sp) do
	{
		// Timers!
		if (sp->spriteTimer)
			if (time > sp->lastTime + sp->timerInterval)
			{
				sp->lastTime = time;
				sp->spriteTimer(sp, time);
			}
		
		if (sp->handleSprite)
			sp->handleSprite(sp);
		else
			HandleSprite(sp); // Callback in a real engine
		if (sp->drawSprite)
			sp->drawSprite(sp);
		else
			DrawSprite(sp);
		sp = sp->next;
	} while (sp != NULL);
	
	// Collisions! Just circle tests. Note that custom drawing can conflict with this.
	// All-to-all tests, NOT suitable for large number of sprites!
	a = gSpriteRoot;
	if (a) do
	{
		if (a)
		{
			b = a->next;
			if (b)
			do
			{
				GLfloat dh = a->position.x - b->position.x;
				GLfloat dv = a->position.y - b->position.y;
				GLfloat dist = sqrt(dh*dh + dv*dv);
				if (dist < a->radius + b->radius)
				{
					if (a->spriteCollide)
						a->spriteCollide(a, b);
					if (b->spriteCollide)
						b->spriteCollide(b, a);
				}
				b = b->next;
			} while (b != NULL);
		}
		a = a->next;
	} while (a != NULL);
	
	// Removal!
	sp = gSpriteRoot;
	if (sp) do
	{
		prev = current;
		current = sp;
		sp = sp->next;
		if (current->remove)
		{
			if (current == gSpriteRoot)
				gSpriteRoot = sp;
			if (prev)
				prev->next = sp;
//			current->face = NULL;
			free(current);
			// FEL! Varför? Jo, prev sätts till current!
			current = prev;
		}
	} while (sp != NULL);
	
	glutSwapBuffers();
}

void Reshape(int h, int v)
{
	glViewport(0, 0, h, v);
	gWidth = h;
	gHeight = v;
}

void Timer(int value)
{
	glutTimerFunc(20, Timer, 0);
	glutPostRedisplay();
}

GLfloat vertices[] = {	-0.5f,-0.5f,0.0f,
						-0.5f,0.5f,0.0f,
						0.5f,-0.5f,0.0f,
						
						0.5f,-0.5f,0.0f,
						-0.5f,0.5f,0.0f,
						0.5f,0.5f,0.0f };

GLfloat texcoord[] = {	0.0f, 1.0f,
						0.0f, 0.0f,
						1.0f, 1.0f,
						
						1.0f, 1.0f,
						0.0f, 0.0f,
						1.0f, 0.0f};

int GetRandom(int range)
{
	unsigned int r = rand();
	return r % range;
}

void InitSpriteLight(int argc, char **argv, int w, int h, char *title)
{
	// two vertex buffer objects, used for uploading the
	unsigned int vertexBufferObjID;
	unsigned int texCoordBufferObjID;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(w, h);
	glutInitContextVersion(3, 2);
	if (title)
		glutCreateWindow(title);
	else
		glutCreateWindow("SpriteLight");

#ifdef WIN32
	glewInit();
#endif
	
	glutDisplayFunc(Display);
	glutTimerFunc(20, Timer, 0); // Should match the screen synch
	glutReshapeFunc(Reshape);

	// Load and compile shader
	gDefaultShader = loadShaders("shaders/SpriteLight.vert", "shaders/SpriteLight.frag");
	glUseProgram(gDefaultShader);
	printError("init shader");
	
	// Upload geometry to the GPU:
	
	// Allocate and activate Vertex Array Object
	glGenVertexArrays(1, &vertexArrayObjID);
	glBindVertexArray(vertexArrayObjID);
	// Allocate Vertex Buffer Objects
	glGenBuffers(1, &vertexBufferObjID);
	glGenBuffers(1, &texCoordBufferObjID);
	
	// VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, 18*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(gDefaultShader, "inPosition"), 3, GL_FLOAT, GL_FALSE, 0, 0); 
	glEnableVertexAttribArray(glGetAttribLocation(gDefaultShader, "inPosition"));
	
	// VBO for texCoord data
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(GLfloat), texcoord, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(gDefaultShader, "inTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(gDefaultShader, "inTexCoord"));
	
	glUniform1i(glGetUniformLocation(gDefaultShader, "tex"), 0); // Texture unit 0
	glUniform1f(glGetUniformLocation(gDefaultShader, "alpha"), 1.0);
	
	alphaLoc = glGetUniformLocation(gDefaultShader, "alpha");
	mLoc = glGetUniformLocation(gDefaultShader, "m");
	
	// End of upload of geometry
	
	printError("init arrays");

	srand(time(NULL));
}

void SetOrigin(GLfloat x, GLfloat y)
{
	if (gMaxX != 0) // No limits
	{
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x > gMaxX-gWidth) x = gMaxX-gWidth;
		if (y > gMaxY-gHeight) y = gMaxY-gHeight;
	}
	gOrigin.x = x;
	gOrigin.y = y;
}

vec2 GetOrigin()
{
	return gOrigin;
}

// Limit scrolling to scene
void SetSceneSize(GLfloat x, GLfloat y)
{
	gMaxX = x;
	gMaxY = y;
}

void SetCenter(GLfloat x, GLfloat y)
{
	SetOrigin(x - gWidth/2, y - gHeight/2);
}


void SetYdirection(float d)
{
	gYdirection = d;
}
float GetYdirection()
{
	return gYdirection;
}

GLuint GetDefaultVAO()
{
	return vertexArrayObjID;
}

