// Simple program that retrieves the IP of the computer.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

//Example: b1 == 192, b2 == 168, b3 == 0, b4 == 100
struct IPv4
{
    unsigned char b1, b2, b3, b4;
};

#define SOCKET_ERROR        -1

char getMyIP(struct IPv4 *myIP)
{
    char szBuffer[1024];
	struct in_addr a;

	if(gethostname(szBuffer, sizeof(szBuffer)) == SOCKET_ERROR)
	{
		return 0;
	}

	struct hostent *host = gethostbyname(szBuffer);
	if(host == NULL)
	{
		return 0;
	}
    
	if (host)
	{
		printf("name: %s\n", host->h_name);
		while (*host->h_aliases)
			printf("alias: %s\n", *host->h_aliases++);
		while (*host->h_addr_list)
		{
			bcopy(*host->h_addr_list++, (char *) &a, sizeof(a));
			printf("address: %s\n", inet_ntoa(a));
			
// Note: inet_addr goes the other way, string to binary!
// in_addr_t aa = inet_addr(inet_ntoa(a));
			
			myIP->b1 = ((unsigned char *)(&a))[0];
			myIP->b2 = ((unsigned char *)(&a))[1];
			myIP->b3 = ((unsigned char *)(&a))[2];
			myIP->b4 = ((unsigned char *)(&a))[3];
		}
	}
	
	return 1;
}

main()
{
	struct IPv4 myIP;
	getMyIP(&myIP);
	exit(0);
}
