/* This is the modification of tcp  server
   instead of just sending a single message, the server keeps on sending the message after every 1 second 
   This program implements  multi-processing
   */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h> 
#include <stdlib.h>
#include <arpa/inet.h>
#include "message.h"

#define BACKLOG 1

int main(int argc, char *argv[])
{
	int port, n, i;
	int new_fd;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sa, their_addr, neighbours[2]; 
	char fileBuffer[100];
	int recsize;
	socklen_t fromlen, sin_size;
	memset(&sa, 0, sizeof sa);
	memset (& fileBuffer, 0, sizeof fileBuffer);

	FILE *fp = fopen("config.cfg", "r");
	if(fp == NULL) {
		printf("Server config not found.");
		exit(0);
	} else {
		fscanf(fp, "%d %d", &port, &n);
		int i = 0;
		while(i < n) {
			int neighbourPort = 0;	
			fscanf(fp, "%d", &neighbourPort);
			/*struct data for neighbour connection*/
			neighbours[i].sin_family = AF_INET;
			neighbours[i].sin_addr.s_addr=inet_addr("127.0.0.1");  
			neighbours[i].sin_port = htons(neighbourPort);
			i++;
		}
		fclose(fp);	
	}

	/*struct data for socket binding*/
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr=inet_addr("127.0.0.1");  
	sa.sin_port = htons(port);

	/*to remove error of socket already in use*/
	int yes = 1;
	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
		perror("setsockopt");
		exit(1);
	}

	if (-1 == bind(sock,(struct sockaddr *)&sa, sizeof sa))
	{
		perror("error bind failed");
		close(sock);
		exit(EXIT_FAILURE);
	} 

	if (listen(sock, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("Server: waiting for connections...port no %d\n", port);
	/*set structs for neighbour servers*/
	int s1, s2, fd[n], rv;
	fd_set readfds;
	struct timeval tv;

	FD_ZERO(&readfds);
	tv.tv_sec = 2;
	/*tv.tv_sec = 10;*/
	tv.tv_usec = 500000;

	while(1) { // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sock, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		printf("Server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));

		int bytes;
		message_t receivedMessage;
		if ((bytes = recv(new_fd, &receivedMessage, sizeof (receivedMessage), 0)) == -1) perror("recv");
		else if (bytes == 0) printf ("Connection closed by the server  \n");
		else {
			if (!fork()) { // this is the child process
				close(sock); // child doesn't need the listener
				if(receivedMessage.read_write == 1) {

					FILE *fp;
					int noFile = 1;
					do{
						if ((bytes = recv(new_fd, &fileBuffer, sizeof (fileBuffer), 0)) == -1) 
							perror("Receiving error");
						else if (bytes == 0) {
							printf ("Connection closed by the server.\n");
						} else {
							if (noFile) fp = fopen(receivedMessage.fileName, "w"); 
							noFile = 0;
							fprintf(fp, "%s", fileBuffer);
							printf ("%s\n", fileBuffer); //print file to console
							bzero(fileBuffer, sizeof(fileBuffer));
						}
					}while(bytes > 0);
					if(noFile) printf("File could not be uploaded.");
					else fclose(fp);

				} else {

					FILE *fp = fopen(receivedMessage.fileName, "r");
					if(fp == NULL) {

						int lim = 0;
						for(i = 0; i < n; i++) {
							/*sending file message to all neighbbours after connection*/
							if(!receivedMessage.ttl--) break;
							fd[i]=socket(PF_INET, SOCK_STREAM, 0);
							if(connect(fd[i], (struct sockaddr*)&neighbours[i], sizeof sa) == -1) {
								close(fd[i]);
							} else {
								if(send(fd[i], &receivedMessage, sizeof receivedMessage, 0) == -1){
									close(fd[i]);
								} else {
									FD_SET(fd[i], &readfds);
									lim = fd[i] + 1;
								}
							}
						}
						int neighbourFound = 0;
						rv = select(lim, &readfds, NULL, NULL, &tv);
						if (rv == -1) 
						{
							perror("select"); // error occurred in select()
						} 
						else if (rv == 0) 
						{
							printf("Timeout occurred! No data after %f %f seconds.\n", (float) tv.tv_sec, (float) tv.tv_usec);
						} 
						else 
						{
							// one or both of the descriptors have data
							for(i = 0; i < n; i++) {
								/*check all file descriptors of neighbours*/
								if(FD_ISSET(fd[i], &readfds)) {
									while(bytes = recv(fd[i], fileBuffer, sizeof fileBuffer, 0) > 0) {
										if (send(new_fd, fileBuffer, sizeof fileBuffer, 0) == -1) 
											perror("Sending error");
										bzero(fileBuffer, sizeof(fileBuffer));
										neighbourFound = 1;
									}
									break; 	/*data sent through one socket. dont send repeatedly*/
									/*comment break to improve fault tolerance but increase redundancy*/
								}
							}
						}

						if(!neighbourFound) printf("File not found.\n");
						/*ask other servers for file*/
					} else {
						while(fscanf(fp, "%100c", fileBuffer) != EOF) {
							//send file
							if (send(new_fd, fileBuffer, sizeof fileBuffer, 0) == -1) 
								perror("Sending error");
							bzero(fileBuffer, sizeof(fileBuffer));
						}
						fclose(fp);
						printf("File sent.\n");
					}
				}	
				close (new_fd);  // close the new_fd for  child
				exit (0);
			} // end of child
			close(new_fd); // close new_fd for  parent
		}
	}
}
