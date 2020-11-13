
// Sample UDP client for testing the bad network simulator
// Based on "udp-client", modified by Ingemar 2014
// Compile with
// gcc udp-client.c -o udp-client

// Test by starting the udp-server locally and then run this as:
// ./udp-client 127.0.0.1
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PORT 32000

#include "bad.h"

int main(int argc, char**argv)
{
   int sockfd,n, count = 0;
   struct sockaddr_in servaddr,cliaddr;
   char sendline[1000];
   char recvline[1000];
   struct timeval tv;

   if (argc != 2)
   {
      printf("usage:  udpcli <IP address>\n");
      exit(1);
   }

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   BADsetUDPerrorrate(0.1);
   BADsetUDPdelay(0.5, 1.0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(argv[1]);
   servaddr.sin_port=htons(PORT);

   // Set a timeout on the socket so it is (almost) non-blocking
   tv.tv_sec = 0; /* seconds */
   tv.tv_usec = 1;
   if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        printf("Cannot Set SO_SNDTIMEO for socket\n");
   if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        printf("Cannot Set SO_RCVTIMEO for socket\n");

   printf("I will now send some numbers!\n");
   for (;;)
   {
//   printf("x\n");
      count++;
      if ((count % 10) == 0)
      {
	      sprintf(sendline, "%d\n", count / 10);
	      printf("Sending %s", sendline);
	      sendto(sockfd,sendline,strlen(sendline),0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
      }
//      printf("reading\n");
      n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
      if (n > 0)
      {
        recvline[n]=0;
      	printf("Response from UDP server: ");
      	fputs(recvline,stdout);
      }
      usleep(10000);
   }
}
