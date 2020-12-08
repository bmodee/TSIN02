// Sample UDP client
// Compile with
// gcc client.c -o client

// Test by starting the udp-server locally and then run this as:
// ./udp-client 127.0.0.1
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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



//=======================================================

typedef struct GridSpriteRec
{
	struct SpriteRec sp;

	int ghostkind; // I.e. ghost number
	int direction; // 0, 1, 2, 3
	int gridX, gridY;
	float partMove; // 0 to 1
	int nextDirection; // For player controlled sprites
} GridSpriteRec, *GridSpritePtr;






//=======================================================

int main(int argc, char**argv)
{
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   char sendline[1000];
   char recvline[1000];
   int key;

   if (argc != 2)
   {
      printf("usage:  udpcli <IP address>\n");
      exit(1);
   }

   sockfd=socket(AF_INET,SOCK_DGRAM,0);
	 printf("SOCKFD: %d\n", sockfd);
		printf("AF_INET: %d\n", AF_INET);
		printf("SOCK_DGRAM: %d\n", SOCK_DGRAM);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(argv[1]);
   servaddr.sin_port=htons(32000);

	 printf("Connected to socket: %d\n", sockfd);
	 //printf("On adress: %d\n", servaddr.sin_addr.s_addr);

   printf("Type anything followed by CR!\n");

   //while (fgets(sendline, 10000,stdin) != NULL)
   //system("/bin/stty raw");
   while (fgets(sendline, 10000, stdin) != NULL) {
		 /*
		 		printf("===========\n");
		 		printf ("strcmp w: %d\n", strcmp(sendline, "w"));
				printf ("strcmp a: %d\n", strcmp(sendline, "a"));
				printf ("strcmp s: %d\n", strcmp(sendline, "s"));
				printf ("strcmp d: %d\n", strcmp(sendline, "d"));
				printf("===========\n");
			*/
        sendto(sockfd,sendline,strlen(sendline),0,
               (struct sockaddr *)&servaddr,sizeof(servaddr));
			  printf("WAITING FOR RESPONSE from %d\n", sockfd);
        n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
        recvline[n]=0;
        printf("Response from UDP server: \n");
        fputs(recvline,stdout);
      //default: exit(0);
    }

   //system("/bin/stty cooked");
}
