// Demo for SpriteLight2/base code for the "Pacman" scenario for TSIN02:
// Dual PacMan game.

// 141110: First draft! Ghosts, two locally controlled PacMan instances,
// score. No networking - that is your task - but some comments on where
// networking issues are important. Some bugs remain - don't worry about them.
// No power-ups yet.
// 141112: Second version. Sound now working on Linux (but please turn sound
// level down if you are in the lab). Some bug fixes. Mac and Linux in one package.
// (Some versions here not documented.)
// 171116: Updated the command-lines below.
// 2020: The code is NOT compatible with 64-bit MacOSX. I will fix that.

// You can compile from the command-line with
// Mac:
// gcc DualPac.c common/*.c common/Mac/MicroGlut.m -o DualPac -I"common" -L"common" -I"common/Mac" -L"common/Mac" -framework OpenGL  -framework Cocoa  -framework OpenAL -Wno-deprecated-declarations
// Linux:
// gcc DualPac.c common/*.c common/Linux/MicroGlut.c -o DualPac -I common -I common/Linux -DGL_GLEXT_PROTOTYPES -lGL -lX11 -lopenal -lm
// Most source files have also been tested on MS Windows
// Visual Studio files are included but are not tested recently.


#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>


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

// Grid management

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
int player, d1, d2;

//Network stuff
int sockfd,n;
struct sockaddr_in servaddr,cliaddr;
socklen_t len;
char* resp;

char prevKey = '0';


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

// This procedure handles the movement of the sprites.
// Some network related changes may be needed as noted below,
// generally synchronization issues! How can you ensure that both
// players see the same game?
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
				PlaySound(blipsnd);
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
				PlaySound(snd);
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

// PacMans die on collision with ghosts! (When I support power-ups, this will also work
// the other way around.)
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
		PlaySound(losesnd);
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

TextureData *vertical, *horizontal;
TextureData *pill[3];
TextureData *powerup;

// All drawing that are not controlled by the sprites, that is the background, the map
// and the score pills.
void MyBackground()
{
	int h,v,d;
	vec2 position;
	char s[256];

	DrawBackground();

	// Draw the map
	for (h = 0; h < gridSizeX-1; h++)
		for (v = 0; v < gridSizeY-1; v++)
		{
			char center = GetGridValue(h, v);
			if (center == 'X') // blocked
			for (d = 0; d < 2; d++)
			{
				char n = GetNeighborGridValue(h, v, d);
				if (n == 'X')
				{
					position = GridPosToPos(h, v, d, 0.5);
					if (d == 0)
					{
						if (h < gridSizeX-2)
						DrawFace(horizontal, position, 0);
					}
					else
						if (v < gridSizeY-2)
						DrawFace(vertical, position, 0);
				}
			}
			if (center == '.')
			{
				position = GridPosToPos(h, v, 0, 0.0);
				DrawFace(pill[0], position, 0);
			}
		}

	sprintf(s, "Score: %d", score1);
	sfDrawString(20, 40, s);
	sprintf(s, "Score: %d", score2);
	sfDrawString(-20, 40, s);
}

// Initialization of local resources.
// You hardly have to change anything here.
void Init()
{
	int i, s, r, rl;
  //player = 1;
//	TextureData texture;

// Ugly background - the same one that I used in the past.
	LoadTGATextureSimple("graphics/stars.tga", &backgroundTexID); // Bakgrund

	vertical = GetFace("graphics/Vertical.tga");
	horizontal = GetFace("graphics/Horizontal.tga");
	pill[0] = GetFace("graphics/pill1.tga");
	pill[1] = GetFace("graphics/pill2.tga");
	pill[2] = GetFace("graphics/pill3.tga");
	ghostFace[0] = GetFace("graphics/ghostB1.tga");
	ghostFace[1] = GetFace("graphics/ghostB2.tga");
	ghostFace[2] = GetFace("graphics/ghostB3.tga");
	ghostFace[3] = GetFace("graphics/ghostG1.tga");
	ghostFace[4] = GetFace("graphics/ghostG2.tga");
	ghostFace[5] = GetFace("graphics/ghostG3.tga");
	ghostFace[6] = GetFace("graphics/ghostP1.tga");
	ghostFace[7] = GetFace("graphics/ghostP2.tga");
	ghostFace[8] = GetFace("graphics/ghostP3.tga");
	pacmanFace[0] = GetFace("graphics/pacman1.tga");
	pacmanFace[1] = GetFace("graphics/pacman2.tga");
	pacmanFace[2] = GetFace("graphics/pacman3.tga");
	pacmanFace[3] = GetFace("graphics/pacman4.tga");
	deathFace[0] = GetFace("graphics/die1.tga");
	deathFace[1] = GetFace("graphics/die2.tga");
	deathFace[2] = GetFace("graphics/die3.tga");
	deathFace[3] = GetFace("graphics/die4.tga");

	//CreateGhost(1, 1, 4, 0);
	//CreateGhost(3, 1, 4, 1);
	//CreateGhost(1, 4, 4, 2);
	pacman1 = CreatePacman(8, 6, 4);
	pacman2 = CreatePacman(8, 6, 2);

	snd = LoadSound("sounds/toff16.wav");
	blipsnd = LoadSound("sounds/Blip.wav");
	losesnd = LoadSound("sounds/Lose.wav");

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

	// Init font
	sfMakeRasterFont();

	SetBackgroundDrawingProc(MyBackground);
}

void Mouse(int button, int state, int x, int y)
{
//	exit(0);
}

void Key(unsigned char key,
	#if defined(_WIN32)
         int x, int y)
	#else
         __attribute__((unused)) int x,
         __attribute__((unused)) int y)
	#endif
{
	d1 = -1;
	//printf("Player%d pressed key %c\n", (player == 1 ? 1 : 2), key);


	// ((GridSpritePtr)pacman1)->nextDirection = 1;break;
	switch (key) {
		case 'w':
			d1 = 3;break;
		case 'a':
			d1 = 2;break;
		case 's':
			d1 = 1;break;
		case 'd':
			d1 = 0;break;
		case 0x1b:
			exit(0);
	}

	if (player == 2 && d1 > -1)
	{
		if (d1 == pacman1->direction) return;
		if ((d1+2 % 4) == pacman1->direction && pacman1->direction != 4) // turn around
		{
			pacman1->gridX += directionVector[pacman1->direction][0];
			pacman1->gridY += directionVector[pacman1->direction][1];
			pacman1->direction = d1;
			pacman1->partMove = 1 - pacman1->partMove;
			pacman1->nextDirection = d1;
		}
		else
			pacman1->nextDirection = d1;
	}


	//================================= Remote Player ========================================================

	// Test whether the new direction should be applied immediately or later
	// REPLACE THIS! This should be controlled from the remote player!

	if (player == 1 && d1 > -1)
	{
		if (d1 == pacman2->direction) return;
		if ((d1+2 % 4) == pacman2->direction && pacman2->direction != 4) // turn around
		{
			pacman2->gridX += directionVector[pacman2->direction][0];
			pacman2->gridY += directionVector[pacman2->direction][1];
			pacman2->direction = d1;
			pacman2->partMove = 1 - pacman2->partMove;
			pacman2->nextDirection = d1;
		}
		else
			pacman2->nextDirection = d1;
	}

	if(d1 > -1) {
		d1 = d1 + 2;
		if (sendto(sockfd,&d1,strlen(&d1),0,(struct sockaddr *)&servaddr, sizeof(servaddr))< 0 )
		perror("Error msg");
	//printf("sent instruction: %d to player%d\n", d1 - 2, (player == 1 ? 2 : 1));
	}
}





void InitNetwork(char* address){
// YOUR CODE HERE
// Establish connection with the other computer

// ========== Setup UDP ================
/* Theory:
In UDP, the client does not form a connection with the server like in TCP and instead just sends a datagram.
Similarly, the server need not accept a connection and just waits for datagrams to arrive.
Datagrams upon arrival contain the address of sender which the server uses to send data to the correct client. */


    // ============== 1. Create UDP socket. ==============
		printf("argv[1]%s\n", address);
		//int sockfd,n;
    //struct sockaddr_in servaddr,cliaddr;
    //char sendline[1000];
    //char recvline[1000];
    int key;


    sockfd=socket(AF_INET,SOCK_DGRAM,0);
	printf("-------------------------------------------------------\n");
	printf("SOCKFD: %d\n", sockfd);
	printf("AF_INET: %d\n", AF_INET);
	printf("SOCK_DGRAM: %d\n", SOCK_DGRAM);
	printf("-------------------------------------------------------\n");


		// ============== 1.1 Configure socket ==============

	bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=INADDR_ANY;
    servaddr.sin_port=htons(32000);

	printf("Connected to socket: %d\n", sockfd);
	printf("On adress: %d\n", servaddr);
	printf("-------------------------------------------------------\n");


    // ============== 2. Bind the socket to peer address. ==============

	if (bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0) {
		perror("BIND FAILED");
		player = 2; //player one taken, becomes player 2
	}

	// struct timeval tv;
	// tv.tv_sec = 0;
	// tv.tv_usec = 100000;
	// if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
	// 	perror("Error");
	// }

	printf("YOU ARE PLAYER # %d\n", player);
	printf("setup complete on socket: %d\n", sockfd);
	printf("running on adress: %lu\n", servaddr.sin_addr.s_addr);



    // 3. Wait until datagram packet arrives from client.


    // 4. Process the datagram packet and send a reply to client.
    // 5. Go back to Step 3.


// ========== Setup TCP ================


// 	  ??????
// 1. using create(), Create TCP socket.
// 2. using bind(), Bind the socket to server address.
// 3. using listen(), put the server socket in a passive mode, where it waits for the client to approach the server to make a connection
// 4. using accept(), At this point, connection is established between client and server, and they are ready to transfer data.
// 5. Go back to Step 3.

}

void NetworkTick(){
	char mesg[1000];
	for(;;){
		mesg[0] = '\0';

		int p1 = -1;
		int p2 = -1;
		//printf("waiting for packets..\n");
		len = sizeof(&servaddr);


		if (n = recvfrom(sockfd, mesg,1000,0,(struct sockaddr *)&servaddr,&len) == 0)
			printf("Networktick error: %d\n", mesg);
	
		if (player == 1) {
			p1 = *mesg - 2;
			//printf("player1 recived %d\n", p1);
		} else if (player == 2) {
			p2 = *mesg - 2;
			//printf("player2 recived %d\n", p2);
		}
		//================================= Local Player ========================================================

		// Test whether the new direction should be applied immediately or later
		// LOCAL PLAYER. But what should you tell the other computer about this?
		// SOME NETWORK CODE NEEDED HERE

		if (p1 > -1)
		{
			if (p1 == pacman1->direction) return;
			if ((p1+2 % 4) == pacman1->direction && pacman1->direction != 4) // turn around
			{
				pacman1->gridX += directionVector[pacman1->direction][0];
				pacman1->gridY += directionVector[pacman1->direction][1];
				pacman1->direction = p1;
				pacman1->partMove = 1 - pacman1->partMove;
				pacman1->nextDirection = p1;
			}
			else
				pacman1->nextDirection = p1;
		}


		//================================= Remote Player ========================================================

		// Test whether the new direction should be applied immediately or later
		// REPLACE THIS! This should be controlled from the remote player!

		if (p2 > -1)
		{
			if (p2 == pacman2->direction) return;
			if ((p2+2 % 4) == pacman2->direction && pacman2->direction != 4) // turn around
			{
				pacman2->gridX += directionVector[pacman2->direction][0];
				pacman2->gridY += directionVector[pacman2->direction][1];
				pacman2->direction = p2;
				pacman2->partMove = 1 - pacman2->partMove;
				pacman2->nextDirection = p2;
			}
			else
				pacman2->nextDirection = p2;
		}
	}
	

}

int main(int argc, char **argv)
{
	player = 1;
	InitCallMeAL(8);
	InitSpriteLight(argc, argv, 950, 680, "Dual PacMan base game for TSIN02");
	
	//Network init
	InitNetwork(argv[1]);
	pthread_t tid1;
	pthread_create(&tid1, NULL, NetworkTick, NULL);
	
	// Install your own GLUT callbacks for keyboard and mouse
	glutKeyboardFunc(Key);
	glutMouseFunc(Mouse);
	sfSetRasterSize(950, 680);


	printf("Before the init\n");
	
	Init();

	//thread management

	// Start
	glutMainLoop();
	HaltCallMeAL();
	return 0;
}
