//SSL-Server.c
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include "openssl/ssl.h"
#include "openssl/err.h"


#define FAIL    -1

char *getCommand(char *buffer, char **args);
off_t fsize(const char *filename);

int OpenListener(int port)
{   int sd;
    struct sockaddr_in addr;

    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if ( bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 10) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }
    return sd;
}

SSL_CTX* InitServerCTX(void)
{   SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = SSLv23_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
 /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

int ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;
    int value;

    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Client certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
     
		if(SSL_get_verify_result(ssl) == X509_V_OK) {
            printf("client verification with SSL_get_verify_result() succeeded.\n");  
            value = 1;              
		} else{
            printf("client verification with SSL_get_verify_result() fail.\n");      
            value = 0;
		}
		
        free(line);
        X509_free(cert);
        return value;
    }
    else
        printf("No certificates.\n");
        
    return 0;
}

void Servlet(SSL* ssl) /* Serve the connection -- threadable */
{   char buf[1024];
    char reply[1024];
    int sd, bytes;
    const char* HTMLecho="<html><body><pre>%s</pre></body></html>\n\n";
    char *filename;
	int file;
	int localerror;

	char *command;
	char **data;
	char *buffer;

	int fileSize;
	int BUFFERSIZE = 1024;

    if ( SSL_accept(ssl) == FAIL )     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else
    {
        if(ShowCerts(ssl))        /* get any certificates */
        {
			filename = (char *) calloc(1,255);

			buffer = (char *) calloc(1,BUFFERSIZE);
			data = (char **) calloc(1,255*sizeof(char*));
			command = (char *) calloc(1,255);

			//READ GET
			memset(data,0,255*sizeof(char*));
			memset(command,0,255);
			memset(buffer,0,BUFFERSIZE);

			SSL_read(ssl, buffer, BUFFERSIZE);
			printf("Read: %s\n",buffer);
			command = getCommand(buffer,data);


			if(strcmp(command,"GET")!=0)
			{
			free(filename);
			free(command);
			free(data);
			free(buffer);
			return;
			}

			strcpy(filename,data[0]);


			//OPEN FILE
			if((file = open(filename,O_RDONLY)) == -1)
			{
			//SEND NOT FOUND
			localerror = errno;
			fprintf(stderr,"Can't open filename (%s)",strerror(localerror));

			memset(buffer,0,BUFFERSIZE);
			strcpy(buffer,"NOT FOUND\r\n");

			SSL_write(ssl, buffer, strlen(buffer));
			printf("Sent: %s\n",buffer);

			free(filename);
			free(command);
			free(data);
			free(buffer);	
			return;
			}
			else
			{
			//SEND OK

			memset(buffer,0,BUFFERSIZE);
			strcpy(buffer,"OK\r\n");

			SSL_write(ssl, buffer, strlen(buffer));
			printf("Sent: %s\n",buffer);
			}

			//READ OK
			memset(data,0,255*sizeof(char*));
			memset(command,0,255);
			memset(buffer,0,BUFFERSIZE);
			int readbytes = SSL_read(ssl, buffer, BUFFERSIZE);
			printf("Read: %s\n",buffer);
			printf("Bytes: %d\n",readbytes);
			command = getCommand(buffer,data);

			if(strcmp(command,"OK")!=0)
			{
			free(filename);
			free(command);
			free(data);
			free(buffer);	
			return;
			}

			//SEND SIZE
			fileSize = fsize(filename);

			memset(buffer,0,BUFFERSIZE);
			snprintf(buffer, BUFFERSIZE, "SIZE:%i\r\n", fileSize);

			SSL_write(ssl, buffer, strlen(buffer));
			printf("Sent: %s\n",buffer);

			//READ OK
			memset(data,0,255*sizeof(char*));
			memset(command,0,255);
			memset(buffer,0,BUFFERSIZE);

			SSL_read(ssl, buffer, BUFFERSIZE);
			printf("Read: %s\n",buffer);
			command = getCommand(buffer,data);


			if(strcmp(command,"OK")!=0)
			{
			free(filename);
			free(command);
			free(data);
			free(buffer);	
			return;
			}

			//SEND FILE
			memset(buffer,0,BUFFERSIZE);
			readbytes = 0;
			int total = 0;

			while((readbytes = read(file,buffer,BUFFERSIZE))>0)
			{	
			total += readbytes;
			SSL_write(ssl, buffer, readbytes);
			printf("Sent: %d\n",total);
			memset(buffer,0,BUFFERSIZE);	
			}	

			close(file);

			//READ OK
			memset(data,0,255*sizeof(char*));
			memset(command,0,255);
			memset(buffer,0,BUFFERSIZE);

			SSL_read(ssl, buffer, BUFFERSIZE);
			printf("Read: %s\n",buffer);
			command = getCommand(buffer,data);


			if(strcmp(command,"OK")!=0)
			{
			free(filename);
			free(command);
			free(data);
			free(buffer);	
			return;
			}

			//SEND BYE
			memset(buffer,0,BUFFERSIZE);
			strcpy(buffer,"BYE\r\n");

			SSL_write(ssl, buffer, strlen(buffer));
			printf("Sent: %s\n",buffer);

			//READ BYE
			memset(data,0,255*sizeof(char*));
			memset(command,0,255);
			memset(buffer,0,BUFFERSIZE);

			SSL_read(ssl, buffer, BUFFERSIZE);
			printf("Read: %s\n",buffer);
			command = getCommand(buffer,data);


			if(strcmp(command,"BYE")!=0)
			{
			free(filename);
			free(command);
			free(data);
			free(buffer);	
			return;
			}

			free(filename);
			free(command);
			free(data);
			free(buffer);
		}
    }
    sd = SSL_get_fd(ssl);       /* get socket connection */
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
}

int main(int count, char *strings[])
{   SSL_CTX *ctx;
    int server;
    char *portnum;

    if ( count != 2 )
    {
        printf("Usage: %s <portnum>\n", strings[0]);
        exit(0);
    }

    SSL_library_init();

    portnum = strings[1];

    ctx = InitServerCTX();        /* initialize SSL */

    LoadCertificates(ctx, "device.csr", "device.key"); /* load certs */

	SSL_CTX_load_verify_locations(ctx, "authority.pem", ".");
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	SSL_CTX_set_verify_depth(ctx,1);

    server = OpenListener(atoi(portnum));    /* create server socket */

    while (1)
    {   struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;

        int client = accept(server, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        printf("Connection: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, client);      /* set connection socket to SSL state */
        Servlet(ssl);         /* service connection */
    }
    close(server);          /* close server socket */
    SSL_CTX_free(ctx);         /* release context */
}

char *getCommand(char *buffer, char **args)
{
    char *pch;
    char *command;
    int cont=0;

    command = (char *)calloc (255,1);

    pch = strtok (buffer," :\r\n");

    while (pch != NULL)
	{
		if(strlen(command) == 0)
		{
			strcpy(command, pch);
		}
		else if(args[cont] == NULL || strlen(args[cont]) == 0)
		{	
			if(args[cont] == NULL)
				args[cont] = (char *)calloc (255,1);
				
			strcpy(args[cont], pch);
			cont++;
		}	

		pch = strtok(NULL, " :\r\n");
	}

    return command;
}


off_t fsize(const char *filename)
{
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}
