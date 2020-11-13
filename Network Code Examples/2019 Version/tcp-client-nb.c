
/* Sample TCP client */
// Non-blocking version by Ingemar 2019
// Test localy by running tcp-server and then
// ./tcp-client 127.0.0.1
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>

#define PORT 32000

int main(int argc, char**argv)
{
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   char sendline[1000];
   char recvline[1000];

   if (argc != 2)
   {
      printf("usage:  client <IP address>\n");
      exit(1);
   }

   sockfd=socket(AF_INET,SOCK_STREAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(argv[1]);
   servaddr.sin_port=htons(PORT);

   if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0)
   {
       printf("\n Error : Connect Failed \n");
       return 1;
   };

   printf("Type anything followed by CR!\n");

   // Set up non-blocking TCP I/O
   struct timeval tv;
   fd_set readfds;
   fcntl(sockfd, F_SETFL, O_NONBLOCK);
   tv.tv_sec = 0;
   tv.tv_usec = 0;
   int rv = select(sockfd, &readfds, NULL, NULL, &tv); 

   // Set up non-blocking text input
   char buf[20];
   fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);    
   
   char done = 0;
   
   while (!done)
   {
       int numRead = read(0,buf,10);
       if(numRead > 0)
       {
           buf[numRead] = 0;
           printf("Outgoing message: %s", buf);
           if (buf[0] == 'q') done = 1;
           sendto(sockfd,buf,strlen(buf),0,
               (struct sockaddr *)&servaddr,sizeof(servaddr));
       }
      n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
      recvline[n]=0;
      if (n != 0)
      {
          printf("Response from TCP server: %s\n", recvline);
          n = 0;
          recvline[0]=0;
      }
      sleep(1);
      printf("Tick.\n");
   }

    close(sockfd);
}
