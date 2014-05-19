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
#define FILEPATH "/home/israel/"


int sendBytes(const int clientSocket, const char* writeBuffer);
int sendLine(const int clientSocket, const char *s);
int readBytes(const int clientSocket, char *buffer, int bufferSize);
int validateString(const int clientSocket, char *s, int n);
char *valS(const int clientSocket, char *s, int n);

void HandleReceive(const int clientSocket) 
{
	static char buffer[BUFFERSIZE];
	static char fileloc[BUFFERSIZE];
	struct stat fileStat;
    int filesize;
    int fileDescriptor;
    char *pf, *pr;

    printf("HOME FOLDER: %s\n", FILEPATH);
	if((pf = valS(clientSocket, "GET:", 3)) == NULL)
		goto cleanup;

	printf("%s\n", pf);
	pr = pf;
	while(*pr != '\r')
		pr++;

	*pr = '\0';

	pr = pf;
	while(*pr != ':')
		pr++;
	pr++;


	strcpy(fileloc, FILEPATH);

	strcat(fileloc, pr);

	printf("REQUESTED %s\n", fileloc);

	fileDescriptor = open(fileloc, O_RDONLY);
	if(fileDescriptor >= 0)
	{
		
		
		fstat(fileDescriptor, &fileStat);
		filesize = fileStat.st_size;
		sprintf(buffer, "OK SIZE:%d\r\n\r\n", filesize + 4);
		sendLine(clientSocket, buffer);
	}
	else
	{
		sendLine(clientSocket, "NOTFOUND\r\n\r\n");
		goto cleanup;
	}

	if(validateString(clientSocket, "OK", 2) != 0)
		goto cleanup;

	//FILE
	
	int readBytes, writeBytes;
	
	printf("SENDING FILE\n");
	while((readBytes = read(fileDescriptor, buffer, BUFFERSIZE)) > 0) 
	{
		writeBytes = 0;
		while(writeBytes < readBytes) 
			writeBytes += write(clientSocket, buffer + writeBytes, readBytes - writeBytes);
	}
	sendLine(clientSocket, "\r\n\r\n");
	printf("FILE SENT\n");
	if(validateString(clientSocket, "BYE", 3) != 0)
		goto cleanup;

	sendLine(clientSocket, "BYE\r\n\r\n");
	

cleanup:

	return;
	
	
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
	
	
	
	return writeBytes;
}

int sendLine(const int clientSocket, const char *s)
{
	int val = sendBytes(clientSocket, s);
	printf("SENT [%s].\n", s);
	return val;
}

int readBytes(const int clientSocket, char *writebuffer, int bufferSize)
{
	char *command, *ptr, *buffer;
	int readBytes;

	command = calloc(255, 1);
	buffer = calloc(BUFFERSIZE + 1, sizeof(char));

	while((readBytes = read(clientSocket, buffer, BUFFERSIZE)) > 0)
	{

		command = realloc(command, strlen(command) + readBytes + 1);
		strncat(command, buffer, readBytes);

		//Read the last 4 characters to determine the EOF
		ptr = command + (strlen(command) - 4);
		if(strcmp(ptr,"\r\n\r\n") != 0)
			continue; 
		*ptr = '\0';
		break;
	}
	
	strncpy(writebuffer, command, bufferSize);
	free(command);
	free(buffer);
	return 0;
}

char *valS(const int clientSocket, char *s, int n)
{
	char *buffer = malloc(sizeof(char)*BUFFERSIZE);
	printf("WAITING FOR \"%s\" at %d.\n", s, n);
	readBytes(clientSocket, buffer, BUFFERSIZE);
	if(strncmp(buffer, s, n) != 0)
	{
		printf("\tFAIL\n");
		free(buffer);
		return NULL;
	}
	printf("\tOK\n");
	return buffer;

}

int validateString(const int clientSocket, char *s, int n)
{
	char *buffer = malloc(sizeof(char)*BUFFERSIZE);
	printf("WAITING FOR \"%s\" at %d.\n", s, n);
	readBytes(clientSocket, buffer, BUFFERSIZE);
	if(strncmp(buffer, s, n) != 0)
	{
		printf("\tFAIL\n");
		free(buffer);
		return -1;
	}
	printf("\tOK\n");
	free(buffer);
	return 0;
	
}