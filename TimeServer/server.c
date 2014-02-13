#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define MAX_IN_QUEUE 5
#define ELEMENTS 26
#define SIZE_ELEMENTS 1

int main(int args, char *argv[]) 
{

	u_int port;
	int server;
	int client;
	int localerror;
	struct sockaddr_in myAddr;
	struct sockaddr_in cliente_addr;
	socklen_t clienteLen;	
	int status;
	char *date;
	time_t timer;
	struct tm* tm_info;


    //Argument validation
	if(args < 2) 
	{
	    fprintf(stderr,"Error: Missing Arguments\n");
		fprintf(stderr,"\tUSE: %s [PORT]\n",argv[0]);
		return 1;
	}

	port = atoi(argv[1]);
	if(port < 1 || port > 65535) 
	{
		fprintf(stderr,"Port %i out of range (1-65535)\n",port);
		return 1;
	}

	//Initiate socket connection
	server = socket(PF_INET,SOCK_STREAM,0);
	if(server == -1) 
	{
		localerror = errno;
		fprintf(stderr, "Error: %s\n",strerror(localerror));
		return 1;
	}

	//Bind the specific port
	bzero(&myAddr,sizeof(myAddr)); //Deprecated, change to memset
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddr.sin_port = htons(port);

	status = bind(server, (struct sockaddr *)&myAddr, sizeof(myAddr));
	if(status != 0) 	
	{
		localerror = errno;
		fprintf(stderr,"Can't Bind Port: %s\n",strerror(localerror));
		close(server);
		return 1;
	}

    
	//Start listening 
	status = listen(server,MAX_IN_QUEUE);
	if(status == -1) 
	{
		localerror = errno;
		fprintf(stderr,"Can't listen on socket(%s)\n",strerror(localerror));
		close(server);
		return 1;
	}

	date = (char *)calloc(ELEMENTS,SIZE_ELEMENTS);

	//Wait infinitely for a connection 
	while(1)
	{
		bzero(&cliente_addr,sizeof(cliente_addr));
		client = accept(server,(struct sockaddr *)&cliente_addr,&clienteLen);
		if(client == -1) 
		{
			localerror = errno;
			fprintf(stderr,"Error acepting conection %s\n",strerror(localerror));
			close(server);
			return 1;
		}

		bzero(date,ELEMENTS*SIZE_ELEMENTS);
		time(&timer);
		tm_info = localtime(&timer);
		strftime(date,25,"%Y/%m/%d  %H:%M:%S", tm_info);

        //Future work:compensate for those seconds (maybe minutes) spent 
		//on sending the actual date to the client
		status=write(client,date,strlen(date));  												
		if(status != strlen(date)) 
		{
			fprintf(stderr,"Error: Sent only %i of %lu bytes\n",status,strlen(date));
			return 1;
		}	

		//free(date);
		close(client);
	}
	free(date);
	close(server);

	return 0;
}
