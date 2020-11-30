// Sample UDP server
// Compile with
// gcc udp-server.c -o udp-server

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char**argv)
{
   printf("starting server...\n");
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char mesg[1000];
   char* resp;

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(32000);
   bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   printf("setup complete on socket: %d\n", sockfd);

   for (;;)
   {
      printf("waiting for packets..\n");
      len = sizeof(cliaddr);
      n = recvfrom(sockfd,mesg,1000,0,(struct sockaddr *)&cliaddr,&len);
      //sendto(sockfd,mesg,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
      printf("-------------------------------------------------------\n");
      mesg[n] = 0;
      printf("Received the following:\n");
      printf("%s",mesg);

      switch(atoi(mesg)){
          case 0:
            resp = "going up\n";break;
          case 1:
            resp = "going down\n";break;
          case 2:
            resp = "going left\n";break;
          case 3:
            resp = "going right\n";break;
      }

      sendto(sockfd,resp,strlen(resp),0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));

      printf("-------------------------------------------------------\n");
   }
}