
#include <sys/types.h>
#include <strings.h>
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
#define OUTNAME "test.zip"
#define PATH "/home/nova/files/"
#define FILEPATH "/home/nova/asd.mp3"
#define NOTFOUND "/home/nova/notFound.html"

void doGet(const int clientSocket, const char *fileName);
int sendLine(const int clientSocket, const char* writeBuffer);
int listFiles(const int clientSocket);
int validateCommands(const int clientSocket);

void start_protocol(const int clientSocket) 
{

	char readBuffer[BUFFERSIZE];
	int readBytes;
	char *cmdString;
	char *ptr;
	char *token;
	char *accessRoute;

	cmdString = (char *) calloc(255,1);
	while((readBytes = read(clientSocket,readBuffer,MAXBYTESREAD))>0)
	{
			cmdString = (char *) realloc(cmdString,strlen(cmdString)+readBytes+1);
			strncat(cmdString,readBuffer,readBytes);
			//Read the last 4 characters to determine the EOF
			ptr = cmdString+(strlen(cmdString)-4);
			if(strcmp(ptr,"\r\n\r\n")==0) 
			{	
				*ptr = '\0';
				break;
			}
	}	
		
	debug(4,"<- %s",cmdString);

	token = strtok(cmdString," \t\b");
	debug(4,"%s",token);
	if(strcmp(token,"GET")==0)
	{
		token = strtok(NULL," \t\b");
		if(token == NULL) 
		{
			sendLine(clientSocket, "ERROR ( Acces Route )\r\n\r\n");
		}
		else
		{
			accessRoute = token;
			token = strtok(NULL," \t\b"); 
			if(strcmp(token,"HTTP"))
			{
				doGet(clientSocket,accessRoute);
			}
			else
			{
				sendLine(clientSocket, "Missing Protocol\r\n");
			}
		}
	}
	else
	{
		sendLine(clientSocket, "Unkown command\r\n\r\n");
	}	
	
	if(cmdString != NULL)
		free(cmdString);

	
}

int start_server(const char iface[], const u_short port, const u_short maxClients) 
{
	int serverSocket;
	int clientSocket;
	char clientIP[18];
	u_int clientPort;
    int forkID;
	int localerror;
	
	serverSocket = newTCPServerSocket4(iface,port,maxClients);

	if(serverSocket == -1) 
    {
		return 0;
	}
	
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
            validateCommands(clientSocket);
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

void doGet(const int clientSocket, const char  baseDir[])
{
	char *writeBuffer = (char *) malloc(256);
	u_int writeBytes = 0;
	char *readBuffer;
	u_int readBytes = 0;
	int fd;
	int localError;
	struct stat fs;
	debug(1,"%s",baseDir);
	
	
	if(strcmp(baseDir,"/download")==0)
	{
		fd = open(FILEPATH,O_RDONLY);
		//debug(1,"%s","Si se encontro la palabra download");
		sendLine(clientSocket, "HTTP/1.1 200 OK\r\n");	
		sprintf(writeBuffer,"Content-Disposition: attachment; filename=%s\r\n",OUTNAME);
		sendLine(clientSocket,writeBuffer);
		writeBuffer = (char *) malloc(256);
		fstat(fd, &fs);
		sprintf(writeBuffer,"Content-Length: %u\r\n",(u_int)fs.st_size);
		sendLine(clientSocket,writeBuffer);
	}
	
	else
	{
		fd = open(NOTFOUND,O_RDONLY);
		debug(1,"%s","Abriendo error 404");
		sendLine(clientSocket, "HTTP/1.1 240 No Content\r\n");	
		sendLine(clientSocket,"Content-Type: text/html\r\n");
	}
	
	if(fd == -1)
	{	
		localError = errno;
		debug(1,"Can't open Requested File (%s)",strerror(localError));
		sprintf(writeBuffer,"NOT_FOUND file\r\n\r\n");
		sendLine(clientSocket, writeBuffer);
		free(writeBuffer);
		return;
	}
	
	sendLine(clientSocket,"\r\n");
	readBuffer = (char *) malloc(BUFFERSIZE);
	
	while((readBytes = read(fd,readBuffer,BUFFERSIZE))>0) 
	{
		writeBytes = 0;
		while(writeBytes < readBytes) 
		{
			writeBytes += write(clientSocket,readBuffer+writeBytes,readBytes-writeBytes);
		}
	}
	
	free(readBuffer);
	free(writeBuffer);
	close(fd);
	
}

int sendLine(const int clientSocket, const char* writeBuffer)
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

int validateCommands(const int clientSocket);
{
	int readBytes;
	char *cmdString;
	char readBuffer[BUFFERSIZE];
	cmdString = (char *) calloc(255,1);
	char *token;
	
	while((readBytes = read(clientSocket,readBuffer,MAXBYTESREAD))>0)
	{
			cmdString = (char *) realloc(cmdString,strlen(cmdString)+readBytes+1);
			strncat(cmdString,readBuffer,readBytes);
			//Read the last 4 characters to determine the EOF
			ptr = cmdString+(strlen(cmdString)-4);
			if(strcmp(ptr,"\r\n\r\n")==0) 
			{	
				*ptr = '\0';
				break;
			}
	}
	token = strtok(cmdString," \t\b");
	if(strcmp(token,"PING")==0)
	{
		sendLine(clientSocket, "PONG\r\n");
	}
	else if(strcmp(token,"FILELIST")==0)
	{
		listFiles();
	}
	else if(strcmp(token,"GETFILE")==0)
	{
		doget();
	}
	else if(strcmp(token,"GETFILEPART")==0)
	{
		listFiles();
	}
	else if(strcmp(token,"GETFILESIZE")==0)
	{
		listFiles();
	}
	
}

int listFiles(const int clientSocket)
{
	//256 porque es la longitud maxima establecida en linux para nombre de archivo
	//ver man readdir
	char filenames[MAXFILES][256];
	char *writeBuffer = (char *) malloc(256);
	DIR *dir;
	int i = 0;
	int j;
	struct dirent *ent;
	if ((dir = opendir (PATH)) != NULL) 
	{
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) 
		{
			if(strcmp(ent->d_name,"..") !=0 && strcmp(ent->d_name,".") !=0)
			{
				strcpy(filenames[i],ent->d_name);
				printf ("%s\n", filenames[i]);
				i++;
			}			
		}
		//printf("We found %d files\n",i);
		closedir (dir);  
	} 
	
	else 
	{
	  /* could not open directory */
	  perror ("Can't open directory\n");
	  return 0;
	}
	sprintf(writeBuffer,"Cantidad: %d",i);
	sendLine(clientSocket, writeBuffer);
	for(j=0;j<i;j++)
	{
		sendLine(clientSocket,filenames[i]);
	}
	
	return i;	
}
