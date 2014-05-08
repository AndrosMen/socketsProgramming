#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "tcp.h"
#include "debug.h"
#include "defaults.h"
#define FILEPATH "/home/israel/enviar.txt"

//void doGet(const int clientSocket, const char *fileName);
int sendBytes(const int clientSocket, const char* writeBuffer);

void HandleReceive(const int clientSocket) 
{

	char readBuffer[BUFFERSIZE];
	int readBytes;
	char *command;
	char *ptr;
	char *accessRoute;
	command = calloc(255, 1);

	sendBytes(clientSocket, "READY\r\n\r\n");

	while((readBytes = read(clientSocket, readBuffer, MAXBYTESREAD)) > 0)
	{
		command = realloc(command,strlen(command) + readBytes + 1);
		strncat(command,readBuffer,readBytes);
		//Read the last 4 characters to determine the EOF
		ptr = command + (strlen(command) - 4);
		if(strcmp(ptr,"\r\n\r\n") != 0)
			continue; 
		*ptr = '\0';
		break;
	}	
		
	debug(4,"<- %s",command);

	if(0 == strcmp(command,"OK"))
	{
		char buffer[256];
		int fileDescriptor = open(FILEPATH, O_RDONLY), readBytes, writeBytes;

		while((readBytes = read(fileDescriptor, buffer,256)) > 0) 
		{
			writeBytes = 0;
			while(writeBytes < readBytes) 
				writeBytes += write(clientSocket, buffer + writeBytes, readBytes - writeBytes);
		}

	}
	else
	{
		debug(4, "EXPECTED OK FROM %d GOT [%10s].", clientSocket, command);
		//sendBytes(clientSocket, "Unkown command\r\n\r\n");
	}	
	
	if(command != NULL)
		free(command);
	
	
}

int start_server(const char iface[], const u_short port, const u_short maxClients) 
{
	int serverSocket, clientSocket, forkID, localerror;
	u_int clientPort;
	char clientIP[18];
	
	serverSocket = newTCPServerSocket4(iface, port, maxClients);

	if(serverSocket == -1) 
		return -1;
	
    while(1)
    {
        bzero(clientIP,sizeof(clientIP));
        clientPort = 0;

        debug(1,"%s","Waiting for a Client...");
        clientSocket = waitConnection4(serverSocket,clientIP,&clientPort); 
        if(clientSocket == -1)
        {                        
            debug(1,"%s","ERROR: Invalid Client Socket");
            continue;    
        }

        debug(2,"Connected Client %s:%u",clientIP,clientPort);

        forkID = fork();
        if(forkID == 0)
        {
            HandleReceive(clientSocket);
            closeTCPSocket(clientSocket);
            debug(2,"Close connection (%s:%u)",clientIP,clientPort);
        }
        else if(forkID > 0)
        {
            closeTCPSocket(clientSocket);
        }
        else
        {
            localerror = errno;
            debug(0,"ERROR, Cant Fork for Client %s",strerror(localerror));
            return 1;
        }

    }

	closeTCPSocket(serverSocket);	
	return 1;
	
}

int sendBytes(const int clientSocket, const char* writeBuffer)
{
	u_int writeBytes = 0;
	
	writeBytes += write(clientSocket,writeBuffer+writeBytes,strlen(writeBuffer)-writeBytes);
	
	while(writeBytes < strlen(writeBuffer))
	{
		writeBytes += write(clientSocket,writeBuffer+writeBytes,strlen(writeBuffer)-writeBytes);
	}
	debug(4,"-> %s",writeBuffer);	
	
	return writeBytes;
}

