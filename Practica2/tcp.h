#ifndef TCP_H
#define TCP_H

int newTCPServerSocket4(const char *ip, const u_short port, const int maxClients);
void closeTCPSocket(const int socketFD);
int waitConnection4(int socket, char *clientIP, u_int *clientPort);
int newTCPClientSocket4(const char *ip, const u_short port);

#endif

