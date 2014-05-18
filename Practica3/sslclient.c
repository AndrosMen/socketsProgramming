//SSL-Client.c
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>

#define FAIL    -1
char *getCommand(char *buffer, char **args);

int OpenConnection(const char *hostname, int port)
{   int sd;
    struct hostent *host;
    struct sockaddr_in addr;

    if ( (host = gethostbyname(hostname)) == NULL )
    {
        perror(hostname);
        abort();
    }
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);
    if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(sd);
        perror(hostname);
        abort();
    }
    return sd;
}

SSL_CTX* InitCTX(void)
{   
	SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = SSLv23_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    //Obligar a usar SSLv3 y TSLv1
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
    return ctx;
}

int ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;
    int value;

    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        
        if(SSL_get_verify_result(ssl) == X509_V_OK) {
            printf("client verification with SSL_get_verify_result() succeeded.\n");    
            value = 1;            
		} else{
            printf("client verification with SSL_get_verify_result() fail.\n");      
            value = 0;
		}
        
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
        return value;
    }
    else
        printf("No certificates.\n");
        
    return 0;
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


int main(int count, char *strings[])
{   SSL_CTX *ctx;
    int server;
    SSL *ssl;
    char buf[10240];
    int bytes;
    char *hostname, *portnum, *filename;

	int file;

	int localerror;

	char *command;
	char **data;
	char *buffer;

	int filesize, readData;

    if ( count != 4 )
    {
        printf("usage: %s <filename> <hostname> <portnum>\n", strings[0]);
        exit(0);
    }

    SSL_library_init();

	filename=strings[1];
 	hostname=strings[2];
 	portnum=strings[3];

    ctx = InitCTX();
    
    LoadCertificates(ctx, "client.csr", "client.key"); 
    SSL_CTX_load_verify_locations(ctx, "authority.pem", ".");
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	SSL_CTX_set_verify_depth(ctx,1);

    server = OpenConnection(hostname, atoi(portnum));

    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, server);    /* attach the socket descriptor */
   
 if ( SSL_connect(ssl) == FAIL )   /* perform the connection */
        ERR_print_errors_fp(stderr);
    else
    {   
		char *buffer;
		int BUFFERSIZE = 1024;

        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
        if(ShowCerts(ssl))
        {
				buffer = (char *) calloc(1,BUFFERSIZE);
				data = (char **) calloc(1,255*sizeof(char*));
				command = (char *) calloc(1,255);

				//SEND GET
				memset(buffer,0,BUFFERSIZE);
				strcpy(buffer,"GET:");
				strcat(buffer,filename);
				strcat(buffer,"\r\n");

				SSL_write(ssl, buffer, strlen(buffer));
				printf("Sent: %s\n",buffer);

				//Read OK or NOTFOUND
				memset(data,0,255*sizeof(char*));
				memset(command,0,255);
				memset(buffer,0,BUFFERSIZE);

				SSL_read(ssl, buffer, BUFFERSIZE);
				printf("Read: %s\n",buffer);
				command = getCommand(buffer,data);


				if(strcmp(command,"OK")!=0)
				{
				free(command);
				free(data);
				free(buffer);	
				return;
				}

				//SEND OK
				memset(buffer,0,BUFFERSIZE);
				strcpy(buffer,"OK");
				strcat(buffer,"\r\n");

				int sendbytes = SSL_write(ssl, buffer, strlen(buffer));
				printf("Sent: %s\n",buffer);
				printf("Bytes: %d\n",sendbytes);

				//Read Size
				memset(data,0,255*sizeof(char*));
				memset(command,0,255);
				memset(buffer,0,BUFFERSIZE);

				SSL_read(ssl, buffer, BUFFERSIZE);
				printf("Read: %s\n",buffer);
				command = getCommand(buffer,data);

				if(strcmp(command,"SIZE")!=0)
				{
				free(command);
				free(data);
				free(buffer);	
				return;
				}

				filesize = atoi(data[0]);

				//SEND OK
				memset(buffer,0,BUFFERSIZE);
				strcpy(buffer,"OK");
				strcat(buffer,"\r\n");

				SSL_write(ssl, buffer, strlen(buffer));
				printf("Sent: %s\n",buffer);

				//OPEN FILE
				if((file = open(filename+1,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
				{
				localerror = errno;
				fprintf(stderr,"Can't open file for write (%s)\n",strerror(localerror));
				return;
				}	

				readData = 0;
				int total = 0;

				while((readData = SSL_read(ssl, buffer, BUFFERSIZE))>0)
				{	
				total += readData;
				printf("Read: %d\n",total);
				write(file,buffer,readData);
				memset(buffer,0,BUFFERSIZE);

				if(total>=filesize)
				break;
				}

				close(file);

				//SEND OK
				memset(buffer,0,BUFFERSIZE);
				strcpy(buffer,"OK");
				strcat(buffer,"\r\n");

				SSL_write(ssl, buffer, strlen(buffer));
				printf("Sent: %s\n",buffer);

				//Read BYE
				memset(data,0,255*sizeof(char*));
				memset(command,0,255);
				memset(buffer,0,BUFFERSIZE);

				SSL_read(ssl, buffer, BUFFERSIZE);
				printf("Read: %s\n",buffer);
				command = getCommand(buffer,data);

				if(strcmp(command,"BYE")!=0)
				{
				free(command);
				free(data);
				free(buffer);	
				return;
				}

				//SEND BYE
				memset(buffer,0,BUFFERSIZE);
				strcpy(buffer,"BYE");
				strcat(buffer,"\r\n");

				SSL_write(ssl, buffer, strlen(buffer));
				printf("Sent: %s\n",buffer);
		}        /* get any certs */
        
		
        SSL_free(ssl);        /* release connection state */
    }
    close(server);         /* close socket */
    SSL_CTX_free(ctx);        /* release context */
    return 0;
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
