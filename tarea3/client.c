#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include "tcp.h"
#include "debug.h"
#include "defaults.h"

int sendBytes(const int clientSocket, const char* writeBuffer);
int sendLine(const int clientSocket, const char *s);
int readBytes(const int clientSocket, char *buffer, int bufferSize);
int validateString(const int clientSocket, char *s, int n);

int debugLevel = 0;

int main(int argc, char *args[])
{
	static char buffer[BUFFERSIZE];
	char *ip = "127.0.0.1";
	int sendPort = atoi(args[1]);
	int sendSocket;
	system("clear");
	printf("CLIENT CONNECTED TO %s : %d.\n", ip, sendPort);
	sendSocket = newTCPClientSocket4(ip, sendPort);

	strcpy(buffer, "GET:");
	strcat(buffer, args[2]);
	strcat(buffer, "\r\n\r\n");


	sendLine(sendSocket, buffer);

	printf("WAITING FOR OK.\n");
	readBytes(sendSocket, buffer, BUFFERSIZE);
	if(strncmp(buffer, "OK", 2) != 0)
	{
		printf("\tFAIL.\n", buffer);
		goto cleanup;
	}

	char *p = buffer;
	while(*p != ':')
		p++;
	p++;
	int filesize = atoi(p);
	printf("\tFILESIZE: %d\n", filesize);

	sendLine(sendSocket, "OK\r\n\r\n");

	printf("WAITING FOR FILE.\n");


	readBytes(sendSocket, buffer, filesize);

	int i;
	for (i = 0; i < filesize; ++i)
	{
		printf("%c", buffer[i]);
	}
	printf("\n");
	//FILE
	printf("GOT FILE.\n");

	sendLine(sendSocket, "BYE\r\n\r\n");

	if(validateString(sendSocket, "BYE", 3) != 0)
		goto cleanup;

cleanup:
	closeTCPSocket(sendSocket);
	return 0;
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