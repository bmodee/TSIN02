// Sample UDP server
// Compile with
// gcc server.c -o server
// gcc server.c common/*.c common/Linux/MicroGlut.c -o server -I common -I common/Linux -DGL_GLEXT_PROTOTYPES -lGL -lX11 -lopenal -lm

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

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
// uses framework Cocoa
// uses framework OpenAL

#include "SpriteLight.h"
#include "GL_utilities.h"
#include "CallMeAL.h"
#include "simplefont.h"
#include "LoadTGA.h"
#include "VectorUtils3.h"

int directionVector[5][2] =
{
	{1, 0},
	{0, 1},
	{-1, 0},
	{0, -1},
	{0, 0},
};

char *map;
int gridSizeX = 15; // Including CR
int gridSizeY = 9;
int blockSize = 64;
float xMargin = 50.0;
float yMargin = 50.0;

//Network stuff
int sockfd,n;
struct sockaddr_in servaddr,cliaddr;
socklen_t len;
char* resp;


typedef struct GridSpriteRec
{
	struct SpriteRec sp;

	int ghostkind; // I.e. ghost number
	int direction; // 0, 1, 2, 3
	int gridX, gridY;
	float partMove; // 0 to 1
	int nextDirection; // For player controlled sprites
} GridSpriteRec, *GridSpritePtr;

vec2 GridPosToPos(int x, int y, int dir, float partMove)
{
	vec2 p;
	p.x = (x + partMove * directionVector[dir][0]) * blockSize + xMargin;
	p.y = (y + partMove * directionVector[dir][1]) * blockSize + yMargin;
	return p;
}

char GetGridValue(int x, int y)
{
	return map[x + y*gridSizeX];
}

void SetGridValue(int x, int y, char value)
{
	map[x + y*gridSizeX] = value;
}

char GetNeighborGridValue(int x, int y, int direction)
{
	return map[x + directionVector[direction][0] + (y + directionVector[direction][1])*gridSizeX];
}

// Game data
GridSpritePtr pacman1, pacman2;
ALuint snd, blipsnd, losesnd;
TextureData *ghostFace[10];
TextureData *pacmanFace[4];
TextureData *deathFace[4];
int score1, score2;


void HandleGridSprite(SpritePtr sp)
{
	int d;
	char v, cfv;
	GridSpritePtr gsp = (GridSpritePtr)sp;
	// Move by direction
	// Limit by grid!
	sp->position = GridPosToPos(gsp->gridX, gsp->gridY, gsp->direction, gsp->partMove);
	gsp->partMove += 0.05;
	if (gsp->partMove >= 1.0) // Reaches grid position!
	{
		gsp->partMove = 0;
		gsp->gridX += directionVector[gsp->direction][0];
		gsp->gridY += directionVector[gsp->direction][1];
		if (gsp->ghostkind < 0) // -1 = player!
			if (GetGridValue(gsp->gridX, gsp->gridY) == '.')
			{
				SetGridValue(gsp->gridX, gsp->gridY, ' ');
				// Add to score
				if (gsp == pacman1)
					score1++;
				if (gsp == pacman2)
					score2++;
				//PlaySound(blipsnd);
				// NETWORK ISSUE - must be the same on both!
			}

		// Decide new direction!
		d = gsp->direction;
		cfv = GetNeighborGridValue(gsp->gridX, gsp->gridY, d);
		// nextDirection is decided by the player
		if (gsp->nextDirection > -1)
			gsp->direction = gsp->nextDirection;
		else
		{
		// Random directions for the ghosts!
		// IMPORTANT! MUST BE SYNCHED OVER THE NETWORK!
		// Hint: If both games see the same random sequence, the ghost will move
		// the same in both games! (This is a simplification from the original game
		// where the ghosts depend on the players' movements - to make life a bit
		// easier for you!)
			gsp->direction = GetRandom(4);
		}
		v = GetNeighborGridValue(gsp->gridX, gsp->gridY, gsp->direction);
		if (v == 'X') // blocked
		{
			if (cfv != 'X') // Open
				gsp->direction = d; // Continue forward
			else
			{
				//PlaySound(snd);
				gsp->direction = 4; // Don't move
			}
		}
	}

	// if direction / switch direction
	if (gsp->ghostkind < 0)
	{
		switch (gsp->direction)
		{
			case 0:
				sp->rotation = 0;break;
			case 3:
				sp->rotation = 90;break;
			case 2:
				sp->rotation = 180;break;
			case 1:
				sp->rotation = 270;break;
			case 4:
				; // No change
			//	sp->rotation = 0;
		}
	}
	else
		sp->rotation = 0; // atan2(sp->speed.v, sp->speed.h) * 180.0/3.1416;
}

GridSpritePtr NewGridSprite(int h, int v, int dir)
{
	GridSpritePtr gsp;
	gsp = (GridSpriteRec *)malloc(sizeof(GridSpriteRec));
	InitSprite((SpritePtr)gsp);

	gsp->gridX = h;
	gsp->gridY = v;
	gsp->direction = dir;
	gsp->partMove = 0;
	gsp->sp.position = GridPosToPos(gsp->gridX, gsp->gridY, gsp->direction, gsp->partMove);
	return gsp;
}

// Timers for controlling the animations.
void GhostTimer(SpritePtr sp, long time)
{
	sp->a = (sp->a + 1) % 3;
	sp->face = ghostFace[sp->a + ((GridSpritePtr)sp)->ghostkind * 3];
}
void PacTimer(SpritePtr sp, long time)
{
	sp->a = (sp->a + 1) % 4;
	sp->face = pacmanFace[sp->a];
}
void DeathTimer(SpritePtr sp, long time)
{
	sp->a++;
	if (sp->a > 3)
		RemoveSprite(sp);
	else
		sp->face = deathFace[sp->a];
}

void Collide(SpritePtr me, SpritePtr him)
{
	if (((GridSpritePtr)him)->ghostkind >= 0) // Check what kind of sprite we collided with!
	{
		me->handleSprite = NULL;
		me->speed.x = 0;
		me->speed.y = 0;
		me->spriteTimer = DeathTimer;
		me->spriteCollide = NULL;
		me->face = deathFace[0];
		me->a = 0;
		me->timerInterval = 500; // How often do we wish to change the face?
		//PlaySound(losesnd);
	}
}

GridSpritePtr CreateGhost(int h, int v, int dir, int kind)
{
	GridSpritePtr sp = NewGridSprite(h, v, dir);
	sp->sp.handleSprite = HandleGridSprite;
	sp->sp.timerInterval = 100; // How often do we wish to change the face?
	sp->sp.face = ghostFace[1];
	sp->sp.spriteTimer = GhostTimer;
	sp->ghostkind = kind; // 0..3, decides color.
	sp->nextDirection = -1;
	return sp;
}


GridSpritePtr CreatePacman(int h, int v, int dir)
{
	GridSpritePtr sp = NewGridSprite(h, v, dir);
	sp->sp.handleSprite = HandleGridSprite;
	sp->sp.timerInterval = 100; // How often do we wish to change the face?
	sp->sp.face = pacmanFace[0];
	sp->sp.spriteTimer = PacTimer;
	sp->ghostkind = -1; // Signal this as NOT being a ghost
	sp->nextDirection = 4;
	sp->sp.spriteCollide = Collide;
	return sp;
}



// All drawing that are not controlled by the sprites, that is the background, the map
// and the score pills.


// Initialization of local resources.
// You hardly have to change anything here.
void Init()
{
	int i, s, r, rl;
//	TextureData texture;



	CreateGhost(1, 1, 4, 0);
	CreateGhost(3, 1, 4, 1);
	CreateGhost(1, 4, 4, 2);
	pacman1 = CreatePacman(8, 6, 4);
	pacman2 = CreatePacman(8, 6, 2);


	map = readFile("map.txt");
	// Analyze map here if variable size is supported
	s = 0; // Previous row start
	r = 0; // Number of rows
	rl = 0; // Row length found
	for (i = 0; map[i] != 0; i++)
	{
		if (map[i] == 13 || map[i] == 10)
		{
			r++;
			rl = i - s;
			s = i + 1;
		}
	}
	gridSizeX = rl+1;
	gridSizeY = r;
	printf("Map size %d %d\n", gridSizeX, gridSizeY);
	printf("Map data size %d\n", i);


}


void newDirection(int d1){
  if (d1 == pacman1->direction) return;
  if ((d1+2 % 4) == pacman1->direction && pacman1->direction != 4) // turn around
  {
    pacman1->gridX += directionVector[pacman1->direction][0];
    pacman1->gridY += directionVector[pacman1->direction][1];
    pacman1->direction = d1;
    pacman1->partMove = 1 - pacman1->partMove;
    pacman1->nextDirection = d1;
  }
  else {
    pacman1->nextDirection = d1;
  }
}

void* GameTick(void* vargp) {
  int* myid = (int*) vargp;
  //int a = 0;

  for (;;){
		float a = 1/128;
		sleep(a);
    HandleGridSprite(pacman1);
		printf("Network tick, pos X: %d, pos Y: %d, current direction %d, next direction %d\r", pacman1->gridX, pacman1->gridY, pacman1->direction, pacman1->nextDirection);
    //printf("Gametick tid: %d\nGame tick number: %d\n", *myid, a);
    //()++a;
    //printf("GameTick runs\n");
  }
}

void* NetworkTick(void* vargp) {
  int* myid = (int*) vargp;

  for (;;)
  {
			 char mesg[1000];
	     //printf("waiting for packets..\n");
	     len = sizeof(cliaddr);
	     n = recvfrom(sockfd,mesg,1000,0,(struct sockaddr *)&cliaddr,&len);
	     //sendto(sockfd,mesg,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	     //printf("-------------------------------------------------------\n");
	     mesg[n] = 0;
	     //printf("Received the following:\n");
	     //printf(mesg);
	     //printf("\n");

	     if (strcmp(mesg, "w") == 10) {
	       newDirection(3);
	       resp = "going up\n";
	     } else if (strcmp(mesg, "s") == 10) {
	       newDirection(1);
	       resp = "going down\n";
	     } else if (strcmp(mesg, "a") == 10) {
	       newDirection(2);
	       resp = "going left\n";
	     } else if (strcmp(mesg, "d") == 10) {
	       newDirection(0);
	       resp = "going right\n";
	     } else {
	       resp = "bad input\n";
	     }
			 /*
	     printf("Current direction %d\n", pacman1->direction);
	     printf("Next Direction %d\n", pacman1->nextDirection);
	     printf("pacman1 pos x %d\n", pacman1->gridX);
	     printf("pacman1 pos y %d\n", pacman1->gridY);
			 */

	     if (sendto(sockfd,resp,strlen(resp),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr))< 0 ){
	       perror("Error msg");
	     }
	     //printf("Networktick tid: %d", *myid);
	     //printf("-------------------------------------------------------\n");
	  }
}

void InitNetwork() {
  printf("starting server...\n");

  sockfd=socket(AF_INET,SOCK_DGRAM,0);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=INADDR_ANY;
  servaddr.sin_port=htons(32000);
  if (bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0) {
    printf("BIND FAILED\n");
  }

  printf("setup complete on socket: %d\n", sockfd);
  printf("running on adress: %s\n", servaddr.sin_addr.s_addr);
}

int main(int argc, char**argv)
{
   Init();
   InitNetwork();

   pthread_t tid1;
   pthread_t tid2;

   pthread_create(&tid1, NULL, NetworkTick, NULL);
   pthread_create(&tid2, NULL, GameTick, NULL);
	 while(true);  //Main thread executing

	 //thread_create(ID, default attr, func, arg to func);
}
