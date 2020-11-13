
// Sample TCP client for testing the bad network simulator
// Based on "tcp-client", modified by Ingemar 2014

// Test localy by running tcp-server and then
// ./tcp-client 127.0.0.1
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
      printf("usage:  client <IP address>\n");
      exit(1);
   }

   sockfd=socket(AF_INET,SOCK_STREAM,0);

   BADsetTCPdelay(0.5, 1.0);

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

   if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0)
   {
       printf("\n Error : Connect Failed \n");
       return 1;
   };

   printf("Type anything followed by CR!\n");

   printf("I will now send some numbers!\n");
   for (;;)
   {
      count++;
      if ((count % 10) == 0)
      {
	      sprintf(sendline, "%d\n", count / 10);
	      printf("Sending %s", sendline);
          sendto(sockfd,sendline,strlen(sendline),0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
      }
      n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
      if (n > 0)
      {
	      recvline[n]=0;
	      printf("Response from TCP server: ");
	      fputs(recvline,stdout);
      }
      usleep(10000);
   }
}
