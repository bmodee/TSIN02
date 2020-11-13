// Bad TCP Simulator
// This is a simple layer on top of the Sockets API to simulate TCP traffic
// on a not so good network. All it is doing is to put messages in a queue,
// delaying them by a configurable time, with a certain variation.

// Assume that you work from the examples uploaded on the course page,
// so you are using sendto() and recvfrom(). (support recv?)

// måste skilja på recvfrom från UDP och TCP!
// -> mappa även "socket", kom ihåg SOCK_STREAM/SOCK_DGRAM
// SOCK_STREAM = TCP
// SOCK_DGRAM = UDP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "milli.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define INSIDEBADNETSIMULATOR
#include "bad.h"

#define nil 0L

typedef struct PkgData
{
	char *buffer;
	int msgLength;
	float deliverytime;
	struct PkgData *next;
} PkgData;

typedef struct SocketData
{
	int socket;
	int xtype;	
	struct PkgData *pkgRoot;
	struct SocketData *next;
} SocketData;

// Static variables
struct SocketData *gSockets;
char init = 0;
float gUDPErrorRate = 0.0; // No errors
float gUDPMinDelay = 0.0; // No delay
float gUDPMaxDelay = 0.0; // No delay
float gTCPMinDelay = 0.0; // No delay
float gTCPMaxDelay = 0.0; // No delay

int BADsocket(int domain, int xtype, int protocol)
// int BADsocket()
{
	int thesocket = socket(domain, xtype, protocol);
	
	// If valid socket, save to list of known sockets, with protocol
	SocketData *s = (SocketData *)malloc(sizeof(SocketData));
	s->socket = thesocket;
	s->xtype = xtype;
	s->pkgRoot = nil;
	// Link in
	s->next = gSockets;
	gSockets = s;
	
	return thesocket;
}

#define RAND ((double)rand() / RAND_MAX)

ssize_t BADrecvfrom(int socket, void *msg, size_t msgLength, unsigned int flags, const struct sockaddr *srcAddr, socklen_t *addrLen)
{
	int xtype;
	time_t t;
	struct SocketData *s;
	
	if (!init)
	{
		srand((unsigned) time(&t));
		ResetMilli();
		init = 1;
	}
	// Find socket
	for (s = gSockets; s != nil; s = s->next)
	{
		if (s->socket == socket)
		{
			xtype = s->xtype;
			break;
		}
	}
	
	// Read from the port
	void *buffer = malloc(msgLength);
	ssize_t sz = recvfrom(socket, buffer, msgLength, flags, srcAddr, addrLen);
	// If we got something
	if (sz > 0)
	{
		if (s == nil) return -1; // Can't do anything without a socket
		// If UDP, check if it should be discarded
		if (xtype == SOCK_DGRAM)
		if (RAND < gUDPErrorRate)
		{
			free(buffer);
			buffer = nil;
		}
		
		if (buffer)
		{
		// Save to list of messages
			struct PkgData *pkgdata = (struct PkgData *) malloc(sizeof(struct PkgData));
			pkgdata->buffer = buffer;
			pkgdata->msgLength = sz;
			// srcAdd and addrLen not supported at this time but can easily be added.
		// Decide when to deliver it
			if (xtype == SOCK_DGRAM)
			{
				float deliverytime = GetSeconds() + gUDPMinDelay + (gUDPMaxDelay - gUDPMinDelay) * RAND;
				pkgdata->deliverytime = deliverytime;
				struct PkgData *p = s->pkgRoot;
				// Skip all packages that are to be delivered after this!
				
				if (p == nil)
				{
					s->pkgRoot = pkgdata;
					pkgdata->next = nil;
				}
				else
				{
					if (s->pkgRoot->deliverytime < deliverytime) // deliver after the root
					{
						pkgdata->next = s->pkgRoot;
						s->pkgRoot = pkgdata;
					}
					else // Scan the list until we find one that is delivered earlier
					{
					for (; p != nil && p->next != nil && deliverytime < p->next->deliverytime; p = p->next);
						if (p == nil)
							printf("BAD ERROR\n");
						pkgdata->next = p->next;
						p->next = pkgdata;
					}
					
// Print out for debugging
//					for (p = s->pkgRoot; p != nil; p = p->next)
//					{
//						printf("%f %s", p->deliverytime, p->buffer);
//					}

				}
			}
			else
			{
				// Time can not be before the previous
				float deliverytime = GetSeconds() + gTCPMinDelay + (gTCPMaxDelay - gTCPMinDelay) * RAND;
				if (s->pkgRoot)
				{
					if (deliverytime < s->pkgRoot->deliverytime)
						deliverytime = s->pkgRoot->deliverytime;
				}
				pkgdata->deliverytime = deliverytime;
				// Link in
				pkgdata->next = s->pkgRoot;
				s->pkgRoot = pkgdata;
			}
		}
	}
	
	// Check if it is time to send the oldest message
	struct PkgData *p = s->pkgRoot;
	struct PkgData *prev = nil;
	for (; p != nil; p = p->next)
	{
		if (p->next == nil) // Found the last, that is the oldest
		{
			if (p->deliverytime > GetSeconds())
				return -1;
			if (p->buffer)
				memcpy(msg, p->buffer, p->msgLength);
			else
				return -1;
			if (prev) prev->next = nil;
			if (p == s->pkgRoot)
				s->pkgRoot = nil;
			free(p->buffer);
			int m = p->msgLength;
			free(p);
			return m;
		}
		prev = p;
	}
	return -1;
}

void BADsetUDPerrorrate(float rate)
{
	gUDPErrorRate = rate;
}

void BADsetUDPdelay(float delayMin, float delayMax)
{
	gUDPMinDelay = delayMin;
	gUDPMaxDelay = delayMax;
}

void BADsetTCPdelay(float delayMin, float delayMax)
{
	gTCPMinDelay = delayMin;
	gTCPMaxDelay = delayMax;
}
