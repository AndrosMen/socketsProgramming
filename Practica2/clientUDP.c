#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[])
 {
	

	int udpSocket;
	struct sockaddr_in udpServer, udpClient;

	socklen_t addrlen = sizeof(udpClient);
	char buffer[255];
	char ip[17];
	u_short clientPort;


	int status;

	//Creamos el Socket
	udpSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(udpSocket == -1)
	 {
		fprintf(stderr,"Can't create UDP Socket");
		return 1;
	}

	udpServer.sin_family = AF_INET;
	inet_pton(AF_INET,BROADCAST_IFACE	,&udpServer.sin_addr.s_addr);
	udpServer.sin_port = htons(BROADCAST_DEFAULT_PORT);

	status = bind(udpSocket, (struct sockaddr*)&udpServer,sizeof(udpServer));

	if(status != 0) 
	{
		fprintf(stderr,"Can't bind");
	}


	while(1) 
	{
		bzero(buffer,255);
		status = recvfrom(udpSocket, buffer, 255, 0, (struct sockaddr*)&udpClient, &addrlen );

		inet_ntop(AF_INET,&(udpClient.sin_addr),ip,INET_ADDRSTRLEN);
		clientPort = ntohs(udpClient.sin_port);

		printf("Recibimos: [%s:%i] %s\n",ip,clientPort,buffer);

		fflush(stdout);
	}

	close(udpSocket);

	return 0;

}
