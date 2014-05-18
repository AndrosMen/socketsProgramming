#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "defaults.h"
#include <fcntl.h>
#include <errno.h>

#define FILEPATH "/home/andros/tst/"

typedef struct servers
{
	int puerto;
	int numArchivos;
	char ip[18]; 
	struct servers *next;
}servers;

int setBroadcastSocket(int *bSock, struct sockaddr_in *brAddr );
int seekServers(servers *head);
void parseReceived(char *buff,servers *list);
void displayServers(servers *head);
int createTCPConnection(int puerto, char *ip);
int sendLine(const int clientSocket, const char* writeBuffer);

int main()
{
	servers *head = malloc (sizeof(servers));
	servers *iterator;
	int opcion;
	int status;
	int i,j;
	short flag = 1;
	char message[BUFFERSIZE];
	char respaldo[BUFFERSIZE];
	char resp[1000];
	char command[60];
	char arg[256];
	char firstByte[4],lastByte[4];
	int send = 0;
	int readBytes = 0,writeBytes;
	char *token;
	char *ptr;
	int filesize;
	int fd;
	char *cmdString = (char *) calloc(10,1);
	
	head->next = NULL;
	status = seekServers(head);
	if(status != 0 || head->puerto == 0) //Asumiendo que puerto 0 nunca se usa
	{
		printf("No servers received\n");
		return 1;
	}
	displayServers(head);
	
	return 0;
}

int seekServers(servers *head)
{
	servers *list = NULL;
	servers *tmp = malloc(sizeof(servers));
	int bcSock;
    struct sockaddr_in broadcastAddr,udpServer,udpClient;
    char buffer[500];
    char ip[20];
    socklen_t addrlen = sizeof(udpClient);
    u_short clientPort;
    
    char *sendString;
    unsigned int sendStringLen;

	int status;
	tmp->next = NULL; //No me acuerdo si era necesario
    
 
    sendString = "Hello from: Andros\n\r\n\r"; //Nombre del server hardcoded

	status = setBroadcastSocket(&bcSock,&broadcastAddr);
	
	if(status != 1)
	{
		fprintf(stderr,"Can't create Broadcast Socket");
		return 1;
	
	}	
    sendStringLen = strlen(sendString);
	
	bzero(buffer,500);
	status = sendto(bcSock,sendString,sendStringLen,0,(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
	printf("Send %i bytes to brodcast addr\n",status);
	
	status = recvfrom(bcSock, buffer, MAXBYTESREAD, 0, (struct sockaddr*)&udpServer, &addrlen );
	//while(status != 0)  //Solo lo hace 1 vez, tengo que buscar como recibir mas de 1 host
	//{
		inet_ntop(AF_INET,&(udpClient.sin_addr),ip,INET_ADDRSTRLEN);
		clientPort = ntohs(udpClient.sin_port);
		printf("Recibimos: [%s:%i]\n",ip,clientPort);
		fflush(stdout);
		
		parseReceived(buffer,tmp);
		
		head->puerto = tmp->puerto;
		head->numArchivos = tmp->numArchivos;
		head->next = malloc(sizeof(servers));
		strcpy(head->ip,ip);
		list = head->next;
	
		//printf("Esperando mas receives\n");
		//status = recvfrom(udpSocket, buffer, MAXBYTESREAD, 0, (struct sockaddr*)&udpServer, &addrlen );
		//printf("Aun hay otro server\n");
	//}
		
	close(bcSock);
	return 0;
}

int setBroadcastSocket(int *bSock, struct sockaddr_in *broadcastAddr )
{
	int broadcastPermission = 1;
	char *broadcastIP;
	unsigned short broadcastPort;
	int status;
	
	broadcastIP = "255.255.255.255"; /* First arg: broadcast IP address */
    broadcastPort = UDP_DEFAULT_PORT; /* Second arg: broadcast port */
	
	*bSock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(*bSock == -1)
	 {
		fprintf(stderr,"Can't create Socket");
		return 0;
	}
	
	status = setsockopt(*bSock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission));
    if(status == -1) 
    {
		fprintf(stderr,"Can't set Brodcast Option");
		return 0;
    }
    
    memset(broadcastAddr, 0, sizeof(*broadcastAddr));
    (*broadcastAddr).sin_family = AF_INET;
    inet_pton(AF_INET,broadcastIP,&(*broadcastAddr).sin_addr.s_addr);
    (*broadcastAddr).sin_port = htons(broadcastPort);
    return 1;
}

void parseReceived(char *buff,servers *list)
{
	char *token;
	token = strtok(buff," \n\r");
	printf("%s\n",token);
	
	while(strcmp(token,"Puerto:")!=0)
	{
		token = strtok(NULL," \n\r");	
		//printf("En 1 while %s\n",token);
		
	}
	token = strtok(NULL," \n\r");	
	list->puerto = atoi(token);
	
	while(strcmp(token,"Archivos:")!=0)
	{
		token = strtok(NULL," \n\r");
		//printf("En 2 while %s\n",token);
	}

	token = strtok(NULL," \n\r");	
	
	list->numArchivos = atoi(token);
	list->next = NULL;
	
}

void displayServers(servers *head)
{
	servers *list = head;
	int i = 1;
	while(list->next !=NULL && list->puerto != 0)
	{
		printf("%d. Port: %d\nTotal Files: %d\n\n",i,list->puerto,list->numArchivos);
		list = list->next;
		i++;
	}	
}

int createTCPConnection(int puerto,char* ip)
{
	int server;
	int localerror;
	struct sockaddr_in server_addr;
	int status;
	
	server = socket(PF_INET,SOCK_STREAM,0);
    if(server == -1) 
    {
       localerror = errno;
       fprintf(stderr, "Error: %s\n",strerror(localerror));
       return 0;
    }

    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;	
    status = inet_pton(AF_INET,ip,&server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(puerto);
	//Beginning  connection to the server
    status = connect(server,(struct sockaddr *)&server_addr,sizeof(server_addr));    
    if(status != 0) 
    {
       localerror = errno;
       printf("Error trying to connect  (%s)\n",strerror(localerror));
       return 0;
    }

    printf("Connection Succesful\n");
	return server;
}

/*int readData ()
{
	int fd;
	u_int readBytes = 0;
	fd = open(FILEPATH,O_RDONLY);
	while((readBytes = read(fd,readBuffer,BUFFERSIZE))>0) 
	{
		writeBytes = 0;
		while(writeBytes < readBytes) 
		{
			writeBytes += write(clientSocket,readBuffer+writeBytes,readBytes-writeBytes);
		}
	}
}*/

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
