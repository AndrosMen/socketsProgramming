#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <dirent.h>
#include <sys/stat.h>

#include "tcp.h"
#include "defaults.h"

#define PATH "/home/andros/files/"
#define MAXFILES 500;

int setUDPSocket(int *bSock, struct sockaddr_in *brAddr,char *ip, int port );
int sendLine(const int clientSocket, const char* writeBuffer);
int listFiles(const int clientSocket);
int validateCommands(const int clientSocket);
void TcpProcess(void);
void sendFile(const int clientSocket ,char* token );
int getFilesize(char* path);

int main(int argc, char* argv[])
 {
	int bcSock;
	struct sockaddr_in udpClient,broadcastAddr;
	socklen_t addrlen = sizeof(udpClient);
	char buffer[500];
	char ip[20];
	u_short clientPort;
	char sendString[500];
	int status, sendStringLen,numFiles = listFiles(0);
	pid_t pid;
	
	pid = fork();
	
	if(pid == 0)
	{
		TcpProcess();
	}
	
	else
	{
		//Creamos el Socket de Broadcast
		status = setUDPSocket(&bcSock,&broadcastAddr,CONFIG_LISENT_IFACE,UDP_DEFAULT_PORT);
		
		if(status != 1)
		{
			fprintf(stderr,"Can't create Broadcast Reciever Socket");
			return 1;
		
		}
		//DEBUG
		/*
		inet_ntop(AF_INET,&(broadcastAddr.sin_addr),ip,INET_ADDRSTRLEN);
		clientPort = ntohs(broadcastAddr.sin_port);
		printf("IP: %s\nPort: %i\n",ip,clientPort);*/

		status = bind(bcSock, (struct sockaddr*)&broadcastAddr,sizeof(broadcastAddr));
		if(status != 0) 
		{
			fprintf(stderr,"Can't bind Broadcast Reciever");
			return 1;
		}

		while(1) 
		{
			bzero(buffer,500);
			status = recvfrom(bcSock, buffer, MAXBYTESREAD, 0, (struct sockaddr*)&udpClient, &addrlen );
			
			inet_ntop(AF_INET,&(udpClient.sin_addr),ip,INET_ADDRSTRLEN);
			clientPort = ntohs(udpClient.sin_port);
			
			printf("Recibimos: [%s:%i] %s\n",ip,clientPort,buffer);
			fflush(stdout);
			
			//Ahora aqui mandamos la info*/
			sprintf(sendString,"Hi AndrosServer\n\rPuerto: %d\n\rArchivos: %d\n\r\n\r",CONFIG_DEFAULT_PORT,numFiles);
			sendStringLen = strlen(sendString);
			status = sendto(bcSock,sendString,sendStringLen,0,(struct sockaddr*)&udpClient,sizeof(udpClient));
			/*if(status <=0)
				printf("No mande nada\n");
			else
				printf("Si mande %d\n",status);*/
		}

		close(bcSock);
	}

	return 0;
}

void TcpProcess()
{
	int serverSocket;
	int clientSocket;
	char clientIP[18];
	u_int clientPort;
    //int forkID;
 
	
	serverSocket = newTCPServerSocket4(CONFIG_LISENT_IFACE,CONFIG_DEFAULT_PORT,5);

	if(serverSocket == -1) 
    {
		printf("Can't create serverSocket\n");
		return;
	}
	
    while(1)
    {
        bzero(clientIP,sizeof(clientIP));
        clientPort = 0;

        //printf("Waiting for a Client\n");
        clientSocket = waitConnection4(serverSocket,clientIP,&clientPort); 
        if(clientSocket == -1)
        {                        
           // debug(1,"%s","ERROR: Invalid Client Socket");
            continue;    
        }
		printf("Client connected\n");
        //debug(2,"Connected Client %s:%u",clientIP,clientPort);

        while(1)
        {
			validateCommands(clientSocket);
		}
		printf("Termine la validacion\n");
		closeTCPSocket(clientSocket);
        //debug(2,"Close connection (%s:%u)",clientIP,clientPort);
    }
    
	closeTCPSocket(serverSocket);	
}

int setUDPSocket(int *bSock, struct sockaddr_in *broadcastAddr,char *ip,int port )
{
	
	*bSock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(*bSock == -1)
	 {
		fprintf(stderr,"Can't create Socket");
		return 0;
	}
    
    memset(broadcastAddr, 0, sizeof(*broadcastAddr));
    (*broadcastAddr).sin_family = AF_INET;
    inet_pton(AF_INET,ip,&(*broadcastAddr).sin_addr.s_addr);
    (*broadcastAddr).sin_port = htons(port);
    return 1;
}

int sendLine(const int clientSocket, const char* writeBuffer)
{
	u_int writeBytes = 0;
	
	writeBytes += write(clientSocket,writeBuffer+writeBytes,strlen(writeBuffer)-writeBytes);
	
	while(writeBytes < strlen(writeBuffer))
	{
		writeBytes += write(clientSocket,writeBuffer+writeBytes,strlen(writeBuffer)-writeBytes);
	}
	//debug(4,"-> %s",writeBuffer);	
	
	return writeBytes;
}

int validateCommands(const int clientSocket)
{
	int readBytes;
	char *cmdString;
	char readBuffer[BUFFERSIZE];
	cmdString = (char *) calloc(1,1);
	char *token;
	char *ptr;
	int readed;
	bzero(readBuffer,BUFFERSIZE);
	
	while((readBytes = read(clientSocket,readBuffer,BUFFERSIZE))>0) 
	{
			printf("Lei %s\n",readBuffer);
			cmdString = (char *) realloc(cmdString,strlen(cmdString)+readBytes+1);
			//strncat(cmdString,readBuffer,readBytes);
			memcpy((char *)cmdString,readBuffer,readBytes);
			//Read the last 4 characters to determine the EOF
			ptr = cmdString+(strlen(cmdString)-4);
			if(strcmp(ptr,"\r\n\r\n")==0) 
			{	
				*ptr = '\0';
				break;
			}
	}
	token = strtok(cmdString," \r\n");
	if(strcmp(token,"PING")==0)
	{
		printf("LLego PING\n");
		readed = sendLine(clientSocket, "OK PING\r\n\r\nPONG");
		printf("PONG ENVIADO\n");
	}
	else if(strcmp(token,"FILELIST")==0)
	{
		listFiles(clientSocket);
		printf("Lista ENVIADO\n");
	}
	else if(strcmp(token,"GETFILE")==0)
	{
		token = strtok(NULL," \r\n");
		printf("MANDAR %s\n",token);
		sendFile(clientSocket,token);
	}
	else if(strcmp(token,"GETFILEPART")==0)
	{
		listFiles(clientSocket);
	}
	else if(strcmp(token,"GETFILESIZE")==0)
	{
		listFiles(clientSocket);
	}
	
	if(strlen(cmdString) > 1)
	{
		free(cmdString);
	}
	return 0;
	
}

int listFiles(const int clientSocket)
{
	//256 porque es la longitud maxima establecida en linux para nombre de archivo
	//ver man readdir
	//char filenames[20][256];
	char name [2560];
	char *writeBuffer = (char *) malloc(256);
	DIR *dir;
	int i = 0;
	struct dirent *ent;
	if ((dir = opendir (PATH)) != NULL) 
	{
		
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) 
		{
			if(strcmp(ent->d_name,"..") !=0 && strcmp(ent->d_name,".") !=0)
			{
				//strcpy(filenames[i],ent->d_name);
				if(clientSocket !=0)
				{
					strcat(name,ent->d_name);
					strcat(name,"\n");
					//sendLine(clientSocket, ent->d_name);
					//strcat(name,filenames[i]);
					//printf ("%s\n", ent->d_name);
				}
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
	if(clientSocket == 0)
	{
		return i;
	}
	//sprintf(writeBuffer,"OK FILELIST\r\n%d\r\n\r\n",i);
	sprintf(writeBuffer,"OK FILELIST\r\n%d\r\n\r\n%s\r\n\r\n",i,name);
	sendLine(clientSocket, writeBuffer);
	
	/*for(j=0;j<i;j++)
	{
		printf("FILENAME: %s\n",filenames[j]);
		sendLine(clientSocket,filenames[j]);
	}*/
	
	return 1;	
}

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

int buildAddr4(struct sockaddr_in *addr, const char *ip, const int port)
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

    //debug(4,"Socket on %s:%u created",ip,port);

    return socketFD;
}

void closeTCPSocket(const int socketFD) 
{
    close(socketFD);
    //debug(4,"Socket(%i) closed",socketFD);
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

  //  debug(3,"Connected to %s:%i",ip,port);

    return clientSocket;
}

void sendFile(const int clientSocket ,char* token )
{
	int fd;
	char path[100];
	char writeBuffer[BUFFERSIZE];
	char readBuffer[BUFFERSIZE];
	int readBytes,writeBytes;
	
	strcpy(path,PATH);
	strcat(path,token);
	fd = open(path,O_RDONLY);
	if(fd <= 0)
	{
		printf("FILE NOT FOUND\n");
		return;
	}
	int size = getFilesize(path);
	sprintf(writeBuffer,"OK GETFILE\r\n%d\r\n%d\r\n\r\n",size,2);
	sendLine(clientSocket,writeBuffer);
	printf("YA voy a mandar archivo\n");
	sleep(1);
	while((readBytes = read(fd,readBuffer,BUFFERSIZE))>0) 
	{
		writeBytes = 0;
		while(writeBytes < readBytes) 
		{
			writeBytes += write(clientSocket,readBuffer+writeBytes,readBytes-writeBytes);
		}
	}
	close(fd);
}

int getFilesize(char* path)
{
	struct stat st;
	stat(path, &st);
	int size = st.st_size;
	return size;
}
