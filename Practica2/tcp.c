#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "debug.h"

int getNewTCPSocket(int addrType)
{
    int socketFD;
    int localerror;

    socketFD = socket(addrType,SOCK_STREAM,0);
    if(socketFD == -1)
    {
        localerror = errno;
        fprintf(stderr,"Can't create socket (%s)\n",strerror(localerror));
        return -1;
    }

    return socketFD;
}

int buildAddr4(struct sockaddr_in *addr, const char *ip, const u_short port)
{
    int status;
    int localerror;

    bzero(addr, sizeof(addr));
    addr->sin_family = AF_INET;
    status = inet_pton(AF_INET,ip,&(addr->sin_addr.s_addr));
    if(status == 0)
    {
        fprintf(stderr,"Invalid IPv4 Address\n");
        return 0;
    }
    else if(status == -1)
    {
        localerror = errno;
        fprintf(stderr,"Error on IP Address (%s)\n",strerror(localerror));
        return 0;
    }

    addr->sin_port = htons(port);

    return 1;

}

int newTCPServerSocket4(const char *ip, const u_short port, const int maxClients)
{
	int socketFD;
	int status;
	int localerror;
	struct sockaddr_in addr;

	if(!buildAddr4(&addr,ip,port))
		return -1;

	if((socketFD = getNewTCPSocket(PF_INET))== -1)
		return -1;

	status = bind(socketFD, (struct sockaddr*)&addr,sizeof(addr));
	if(status != 0)
	{	
		localerror = errno;
		fprintf(stderr,"Error: Can't change socket mode to listen (%s)\n",strerror(localerror));
		return -1;
	}

    
    status = listen(socketFD,maxClients);
    if(status != 0)
    {
        localerror = errno;
        fprintf(stderr,"Error: Can't change socket mode to listen (%s)\n",strerror(localerror));
        return -1;
    }

    debug(4,"Socket on %s:%u created",ip,port);

    return socketFD;
}

void closeTCPSocket(const int socketFD) 
{
    close(socketFD);
    debug(4,"Socket(%i) closed",socketFD);
    return;
}


int waitConnection4(int socket, char *clientIP, u_int *clientPort) 
{
    int client;
    struct sockaddr_in addrClient;
    socklen_t addrLen;
    char ip[INET_ADDRSTRLEN]={0};
    int localerror;

    addrLen = sizeof(addrClient);

    bzero(&addrClient, sizeof(addrClient));
    client = accept(socket, (struct sockaddr *)&addrClient,&addrLen);
    if(client == -1) 
    {
        localerror = errno;
        fprintf(stderr,"Can't retrive client Socket (%s)\n",strerror(localerror));
        return -1;
    }

    if(clientIP!=NULL) 
    {
        inet_ntop(AF_INET,&(addrClient.sin_addr),ip,INET_ADDRSTRLEN);
        strcpy(clientIP,ip);
    }

    if(clientPort!=NULL) {
        *clientPort = ntohs(addrClient.sin_port);
    }

    return client;
}

int newTCPClientSocket4(const char *ip, const u_short port) 
{
    int clientSocket;
    int status;
    int localerror;
    struct sockaddr_in addr;

    if(!buildAddr4(&addr,ip,port)) 
        return -1;

    if((clientSocket = getNewTCPSocket(PF_INET))==-1) 
        return -1;

    status = connect(clientSocket, (struct sockaddr*)&addr, sizeof(addr));
    if(status == -1) 
    {
        localerror = errno;
        fprintf(stderr,"Can't connect to %s:%i (%s)",ip,port,strerror(localerror));
        return -1;
    }

    debug(3,"Connected to %s:%i",ip,port);

    return clientSocket;
}
