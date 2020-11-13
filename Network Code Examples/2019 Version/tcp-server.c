
/* Sample TCP server */
// gcc tcp-server.c -o tcp-server

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 32000

int main(int argc, char**argv)
{
   int listenfd,connfd,n;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t clilen;
   pid_t     childpid;
   char mesg[1000];

   listenfd=socket(AF_INET,SOCK_STREAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(PORT);
   bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

   printf("TCP server up and listens to port\n");
   
   listen(listenfd,1024);

   for(;;)
   {
      clilen=sizeof(cliaddr);
      connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);

      if ((childpid = fork()) == 0)
      {
         close (listenfd);

         for(;;)
         {
            n = recvfrom(connfd,mesg,1000,0,(struct sockaddr *)&cliaddr,&clilen);
            if (n > 0)
            {
               sendto(connfd,mesg,n,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
               printf("-------------------------------------------------------\n");
               mesg[n] = 0;
               printf("Received the following:\n");
               printf("%s",mesg);
               printf("-------------------------------------------------------\n");
            }
            sleep(1);
 //           printf("Tick.\n");
         }
         
      }
      close(connfd);
   }
}
