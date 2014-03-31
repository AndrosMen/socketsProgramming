#include <sys/types.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int findServers()
{
	
}
int main(int args, char *argv[]) 
{
	
   u_int port;
   int server;
   int localerror;
   struct sockaddr_in server_addr;
   int status;
   char *date;

   //Argument Validation
   if(args < 3)
   {
      fprintf(stderr,"Error: Missing Arguments\n");
      fprintf(stderr,"\tUSE: %s [ADDR] [PORT]\n",argv[0]);
      return 1;
    }

    //Initiating socket connection
    server = socket(PF_INET,SOCK_STREAM,0);
    if(server == -1) 
    {
       localerror = errno;
       fprintf(stderr, "Error: %s\n",strerror(localerror));
       return 1;
    }

	port = atoi(argv[2]);
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;	
    status = inet_pton(AF_INET,argv[1],&server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(port);
	//Beginning  connection to the server
    status = connect(server,(struct sockaddr *)&server_addr,sizeof(server_addr));    
    if(status != 0) 
    {
       localerror = errno;
       printf("Error trying to connect  (%s)\n",strerror(localerror));
       return 1;
    }

    printf("Connection Succesful\n");

	//Allocate memory for the date string and read it from the server
    date = (char *)calloc(ELEMENTS,SIZE_ELEMENT);
    status = read(server,date,ELEMENTS);
    printf("Date: %s\n",date);
    
	free(date);
    close(server);
	return 0;
 }
