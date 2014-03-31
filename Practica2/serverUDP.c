#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "defaults.h"

int main(int argc, char *argv[])
{
    int bcSock;
    struct sockaddr_in broadcastAddr;
    char *broadcastIP;
    char *tcpIP = CONFIG_LISENT_IFACE;
    unsigned short broadcastPort;
    
    char *sendString;
    
    int broadcastPermission = 1;
    unsigned int sendStringLen;

	int status;
	int i;

    broadcastIP = BROADCAST_IFACE; /* First arg: broadcast IP address */
    broadcastPort = BROADCAST_DEFAULT_PORT; /* Second arg: broadcast port */
    sprintf(sendString,"Hello from: %s\n\r\n\r",tcpIP); 
    //sendString = "Hello from: \n\r\n\r"; /* Third arg: string to broadcast */

	bcSock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(bcSock == -1)
	 {
		fprintf(stderr,"Can't create Socket");
		return 1;
	}
  
    status = setsockopt(bcSock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission));
    if(status == -1) 
    {
		fprintf(stderr,"Can't set Brodcast Option");
		return 1;
    }

    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
	inet_pton(AF_INET,broadcastIP,&broadcastAddr.sin_addr.s_addr);
    broadcastAddr.sin_port = htons(broadcastPort);

    sendStringLen = strlen(sendString);
	for(i=0; i<100; i++) 
	{
		status = sendto(bcSock,sendString,sendStringLen,0,(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
		printf("Send %i bytes to brodcast addr\n",status);
		sleep(3);
	}
	return 0;
}
