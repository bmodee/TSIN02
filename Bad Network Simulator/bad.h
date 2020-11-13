#ifndef BADNETSIMULATOR
#define BADNETSIMULATOR

#ifndef INSIDEBADNETSIMULATOR
	#define socket BADsocket
	#define recvfrom BADrecvfrom
#endif

#include <sys/socket.h>
#include <netinet/in.h>

// These map over socket() and recvfrom()
int BADsocket(int domain, int xtype, int protocol);
ssize_t BADrecvfrom(int socket, void *msg, size_t msgLength, unsigned int flags, const struct sockaddr *srcAddr, socklen_t *addrLen);

// Configure the "bad" network in terms of delays and dropped packages
void BADsetUDPerrorrate(float rate);
void BADsetUDPdelay(float delayMin, float delayMax);
void BADsetTCPdelay(float delayMin, float delayMax);

#endif
