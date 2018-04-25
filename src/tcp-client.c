#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "message.h"

int main(int argc, char *argv[])
{
	sleep(1); /*client sleep so that server can initialize from script*/
	if (argc != 2)
	{
		printf("usage tcp-client server-ip-address\n");
		exit(1);
	}
	int port;
	int sock;
	struct in_addr addr; 
	struct sockaddr_in sa;
	int bytes, buffer_length;
	if (inet_aton(argv[1], &addr) == 0) {
		fprintf(stderr, "Invalid address\n");
		exit(EXIT_FAILURE);
	}


	sock=socket(PF_INET, SOCK_STREAM, 0);
	if (sock ==-1)
	{
		printf("Error Creating Socket");
		exit(EXIT_FAILURE);
	}

	int choice = 0;
	printf("1- Download file\n");
	printf("2- Upload file(to Server V)\n");
	scanf("%d", &choice);
	if (choice == 1) port = 8000;
	else if (choice == 2) port = 11000;
	else exit(0);

	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(inet_ntoa(addr));
	sa.sin_port = htons(port);
	printf("Connecting to server %s at port no %d \n", inet_ntoa(addr), port);
	if (connect(sock, (struct sockaddr*)&sa, sizeof sa) == -1) {
		close(sock);
		perror("client: connect");
		exit(1);
	}

	printf("Input filename: ");
	char fileBuffer[100];
	scanf("%s", fileBuffer);
	message_t sendMessage;
	strcpy(sendMessage.fileName, fileBuffer);
	sendMessage.ttl = 3;
	sendMessage.read_write = choice-1;

	if(send(sock, &sendMessage, sizeof sendMessage, 0) == -1)
		perror("Filename could not be sent");
	else if(sendMessage.read_write == 0){
		FILE *fp; 
		int noFile = 1;
		do{
			if ((bytes = recv(sock, fileBuffer, sizeof (fileBuffer), 0)) == -1) 
				perror("Receiving error");
			else if (bytes == 0) {
				printf ("Connection closed by the server.\n");
			} else {
				if(noFile) fp = fopen(sendMessage.fileName, "w");
				noFile = 0;
				fprintf(fp, "%s", fileBuffer);
				printf ("%s\n", fileBuffer); //print file to console
				bzero(fileBuffer, sizeof(fileBuffer));
			}
		}while(bytes > 0);
		if(noFile) printf("File not found on server.\n");
		else fclose(fp);
	} else {
		FILE *fp = fopen(sendMessage.fileName, "r"); 
		if(fp == NULL) {
			printf("File not present on client.\n");
		} else {
			while(fscanf(fp, "%100c", fileBuffer) != EOF){
				if (send(sock, fileBuffer, sizeof (fileBuffer), 0) == -1)  {
					perror("Sending error");
				} 
				bzero(fileBuffer, sizeof(fileBuffer));
			}
			fclose(fp);
			printf("File sent.\n");
		}
	}

	close(sock);
	int stopper;
	scanf("%d", &stopper);
	return 0;
}
