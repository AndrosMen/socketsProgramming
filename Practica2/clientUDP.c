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
	int tcpsocket;
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
	printf("Selecciona el numero de la opcion [1,2,3,...]\n");
	scanf("%d",&opcion);
	iterator = head;
	for(i =1;i<opcion;i++)
	{
		iterator = iterator->next;
	}
	printf("%d %s\n",iterator->puerto,iterator->ip);
	//PREGUNTAR AL PROFE QUE PEDO CON EL IP AQUI, ESTA HARDCODED
	tcpsocket = createTCPConnection(iterator->puerto,CONFIG_LISENT_IFACE);
	
	if(tcpsocket == 0)
	{
		printf("No TCP connection\n");
		return 1;
	}
	
	while(1)
	{
		bzero(message,MAXBYTESREAD);
		bzero(arg,256);
		fflush(stdin);
		printf("Introduce un commando:\n");
		scanf("%s",command);
		strcat(message,command);
		strcat(message,"\r\n");
		//sprintf(message,"%s\r\n",command);
		if(strcmp(command,"GETFILE")==0 || strcmp(command,"GETFILESIZE") ==0 || strcmp(command,"GETFILEPART")==0) 
		{
			printf("Enter file name\n");
			scanf("%s",arg);
			strcat(message,arg);
			strcat(message,"\r\n");
			if(strcmp(command,"GETFILEPART")==0 )
			{
				strcat(message,"\r\n");
				scanf("%s",firstByte);
				strcat(message,firstByte);
				strcat(message,"\r\n");
				scanf("%s",lastByte);
				strcat(message,lastByte);
				strcat(message,"\r\n");
			}
		}
		strcat(message,"\r\n");
		i = 0;
		bzero(respaldo,MAXBYTESREAD);
		printf("Mande %s\n",message);
		send = sendLine(tcpsocket,message);
		//printf("Voy a leer\n");
		bzero(message,MAXBYTESREAD);
		bzero(resp,1000);
		while((readBytes = read(tcpsocket,message,BUFFERSIZE))>0 )
		{
			//readBytes = read(tcpsocket,message,BUFFERSIZE);
			strcpy(respaldo,message);
			//printf("Lei %s\n",message);
			strcpy(resp,message);
		
			token = strtok(message," \n\r");//obtengo el OK
			token = strtok(NULL," \n\r"); //obtengo el cmnd que recibio el server
			printf("Primer token = %s\n",token);
			if(strcmp(token,"PING") == 0)
			{
				//printf("Me sali de PING\n");
				break;
				
			}
			else if(strcmp(token,"FILELIST") == 0 )
			{
				token = strtok(NULL," \n\r"); //Obtengo la cantidad de archivos
				printf("Segundo token = %s\n",token);
				i = atoi(token);
				cmdString = (char *) realloc(cmdString,strlen(cmdString)+readBytes+1);
				strncat(cmdString,respaldo,readBytes);
				ptr = cmdString+(strlen(cmdString)-4);
				printf("PTR%s\n",ptr);
				if(strcmp(ptr,"\r\n\r\n")==0) 
				{	
					*ptr = '\0';
					break;
				}
				else
				{
					printf("LLegue a segundo read\n");
					while((readBytes = read(tcpsocket,message,BUFFERSIZE))>0) 
					{
						cmdString = (char *) realloc(cmdString,strlen(cmdString)+readBytes+1);
						strncat(cmdString,message,readBytes);
						ptr = cmdString+(strlen(cmdString)-4);
						if(strcmp(ptr,"\r\n\r\n")==0) 
						{	
							*ptr = '\0';
							break;
						}
					}
					//break;
				}	
				/*for(j=0;j<i;j++)
				{
					readBytes = read(tcpsocket,message,BUFFERSIZE);
					if(readBytes >0)
					{
						printf("LEI ALGO\n");
						strcat(resp,message);
					}
				}*/
			
			}
			else if(strcmp(token,"GETFILE") == 0 )
			{
				token = strtok(NULL," \n\r"); //Obtengo tamaño de archivo
				filesize = atoi(token);
				printf("TAMANO %d\n",filesize);
				char path[100];
				strcpy(path,FILEPATH);
				strcat(path,arg);
				printf("%s\n",path);
				fd = open(path, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
				if(fd <= 0)
				{
					printf("Cant create file\n");
					break;
				}
				token = strtok(NULL," \n\r");//Obtengo MD5
				readBytes = read(tcpsocket,message,BUFFERSIZE);
				while(readBytes < filesize)
				{
					printf("Lei Algo %d %d\n",readBytes,filesize);
					writeBytes = 0;
					while(writeBytes < readBytes) 
					{
						printf("Escribiendo\n");
						writeBytes += write(fd,message+writeBytes,readBytes-writeBytes);
					}
					readBytes = read(tcpsocket,message,BUFFERSIZE);
				}
				printf("Lei Algo %d %d\n",readBytes,filesize);
				writeBytes = 0;
				while(writeBytes < readBytes) 
				{
					printf("Escribiendo\n");
					writeBytes += write(fd,message+writeBytes,readBytes-writeBytes);
				}
				close(fd);
				break;
			}
		}	
		
		printf("%s\n",resp);
		bzero(resp,1000);
	}
	close(tcpsocket);
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
