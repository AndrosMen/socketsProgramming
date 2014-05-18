#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include "tcp.h"
#include "debug.h"
#define BUFFERSIZE 256

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
int debugLevel = 0;

int main(void)
{
	char *ip = "127.0.0.1";
	char buffer[BUFFERSIZE];
	char *command, *ptr;
	int receivePort = 8081, sendPort = 8080;
	int sendSocket;
	int readBytes;

	printf("CLIENT CONNECTED TO %s : %d.\n", ip, sendPort);
	sendSocket = newTCPClientSocket4(ip, 8080);
	
	command = malloc(BUFFERSIZE*sizeof(char));
	while((readBytes = read(sendSocket, buffer, BUFFERSIZE)) > 0)
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
	
	if(strcmp(command, "READY") != 0)
	{
		printf("EXPECTED READY GOT [%s].\n", command);
		goto cleanup;
	}
	printf("RECEIVED READY.\n");

	free(command);
	command = NULL;

	sendBytes(sendSocket,"OK\r\n\r\n");
	printf("SENT OK.\n");
	printf("WAITING FOR FILE.\n");

	command = malloc(BUFFERSIZE*sizeof(char));
	while((readBytes = read(sendSocket, buffer, BUFFERSIZE)) > 0)
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

	printf("FILE: [%s].\n", command);

cleanup:
	closeTCPSocket(sendSocket);
	free(command);
	return 0;
}