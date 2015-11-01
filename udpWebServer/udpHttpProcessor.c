/*
 * udpHttpProcessor.c
 *
 *  Created on: Sep 20, 2015
 *      Author: supreeth
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "udpHttpProcessor.h"

const int BUFFER_SIZE = 5 * 1024;
const int MAX_BUFFER_SIZE = 1000;

const int MAX_CONNECTIONS = 10000;
const int TIMEOUT_OF_CONNECTIONS = 10;

/**
 * 	This function essentially processes the HTTP requests(read, parse,read from a file and send response back)
 *
 * 	clientSockFd: is the client socket Fd received after TCP connection was established with the client
 *
 */
void processHttpRequests(int clientSockFd) {
	printf("***processing processHttpRequests for %d\n", clientSockFd);
	char buffer[MAX_BUFFER_SIZE];
	struct httpRequest httpRequestObj;
	struct sockaddr_in client_address;
	socklen_t addr_size;
	char *finalHttpResponse;

	while (1) {
		bzero(buffer, MAX_BUFFER_SIZE);
		printf("Waiting for THE NEXT HTTP REQUEST\n");
		addr_size = sizeof(client_address);
		/* Read from the socket for new HTTP Requests */
		while (recvfrom(clientSockFd, buffer, MAX_BUFFER_SIZE, 0,
				(struct sockaddr *) &client_address, &addr_size) <= 0)
			;
		printf("**Request received is::%s\n", buffer);

		/* Parse the HTTP request */
		httpRequestObj = parseHttpRequest(buffer, httpRequestObj);

		printf("After processGetRequest::\n");

		/*Create the HTTP response by reading the requested file*/
		finalHttpResponse = formulateHttpResponse(clientSockFd, httpRequestObj);

		/*Send the HTTP response back to the client*/
		sendHttpResponse(clientSockFd, client_address, finalHttpResponse);

	}
}

/*
 * This function sends the HTTP response to the client
 *
 * clientsockfd: Client Socket FD
 * finalHttpResponse: HTTP Response to be sent as a string
 *
 * returns: 1 if success 0 if failure
 */
int sendHttpResponse(int clientSockFd, struct sockaddr_in client_address,
		char *finalHttpResponse) {
	struct timeval tv;
	struct timezone tz;
	printf("clientSockFd ::%d\n", clientSockFd);
	printf("client addr ::%d\n", client_address.sin_addr.s_addr);
	printf("client port ::%d\n", client_address.sin_port);
	printf("finalHttpResponse length::%lu\n", strlen(finalHttpResponse));

	int i = 1;
	int totalLength = strlen(finalHttpResponse);
	int noOfPackets = totalLength / MAX_BUFFER_SIZE;
	int lengthLeft;
	printf("No of packets:%d... totalLength:%d\n", noOfPackets, totalLength);
	while (i <= (noOfPackets + 1)) {
		lengthLeft = totalLength - ((i - 1) * MAX_BUFFER_SIZE);
		if (lengthLeft < MAX_BUFFER_SIZE) {
			sendto(clientSockFd,
					finalHttpResponse + ((i - 1) * MAX_BUFFER_SIZE), lengthLeft,
					0, &client_address, sizeof(client_address));
			printf("Packet number:%d ,Offset :%d, length:%d\n", i,
					(i - 1) * MAX_BUFFER_SIZE, lengthLeft);
		} else {
			sendto(clientSockFd,
					finalHttpResponse + ((i - 1) * MAX_BUFFER_SIZE),
					MAX_BUFFER_SIZE, 0, &client_address,
					sizeof(client_address));
			printf("Packet number:%d ,Offset :%d, length:%d\n", i,
					(i - 1) * MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
		}
		i++;
	}
	gettimeofday(&tv, &tz);
	printf("HttpResponse send success..\n");
	printf("Time of last packet sent %d\n", tv.tv_usec);
	return 1;
}

long calculateTime(struct timeval startTime, struct timeval endTime) {
	long elapsedTime = (endTime.tv_sec * 1000000 + endTime.tv_usec)
			- (startTime.tv_sec * 1000000 + startTime.tv_usec);
	return elapsedTime;
}

struct httpRequest parseHttpRequest(char *buffer,
		struct httpRequest httpRequestObj) {

	char *line;

	/* get the first token */
	line = strtok(buffer, "\n\n");

	printf("\nFirst Line:: %s\n", line);
	httpRequestObj = processGetRequest(line, httpRequestObj);

	return httpRequestObj;
}

/**
 * This function reads the requested file from Filesystem and formulates HTTP 200, 404 and 400 based on the cases
 *
 * clientSockFd: Client socket FD
 * httpRequestObj: Http request details
 *
 * returns: final HTTP response to be sent
 */
char *formulateHttpResponse(int clientSockFd, struct httpRequest httpRequestObj) {
	FILE *fp;
	printf("Entering formulateHttpResponse::\n");
	int itemsRead = 0;
	char responseLine1[50] = { 0 };
	char responseLine2[50] = { 0 };
	char responseLine3[30] = { 0 };
	int fileLength = 0;
	char fileLengthString[20];
	char *fileContent;
	char *finalResponse;
	int lenTmp = strlen(httpRequestObj.httpRequestLocation);
	char *location;
	int lengthOfFinal = 0;
	printf("Location String: %s and length:%d start: %s\n",
			httpRequestObj.httpRequestLocation, lenTmp,
			httpRequestObj.httpRequestLocation + 1);
	location = strndup(httpRequestObj.httpRequestLocation + 1, lenTmp); //Used to truncate / from the file path
	printf("opening %s\n", location);
	fp = fopen(location, "r");
	printf("done opening\n");
	if (strstr(httpRequestObj.httpRequestType, "INVALID") != NULL) { //HTTP 400 case
		strcat(responseLine1, "HTTP/1.1 400 Bad Request\n");
		strcat(responseLine3, "Content-Length: 5\n\n");
		fileContent = calloc(1, 5);
		strcat(fileContent, "Error");
	} else if (fp == NULL) {
		printf("Error opening the file\n");
		strcat(responseLine1, "HTTP/1.1 404 Not Found\n");
		strcat(responseLine2, "Content-Type: text/html; charset=utf-8\n");
		strcat(responseLine3, "Content-Length: 9\n\n");
		fileContent = calloc(1, 9);
		strcat(fileContent, "Not found");
	} else {	//HTTP 200 case
		strcat(responseLine1, "HTTP/1.1 200 OK\n");
		strcat(responseLine2, "Content-Type: text/html; charset=utf-8\n");
		fseek(fp, 0, SEEK_END);
		fileLength = ftell(fp);
		printf("File Length:%d\n", fileLength);
		sprintf(fileLengthString, "%d", fileLength);
		strcat(responseLine3, "Content-Length: ");
		strcat(responseLine3, fileLengthString);
		strcat(responseLine3, "\n\n");
		fseek(fp, 0, SEEK_SET);
		fileContent = (char *) calloc(fileLength, sizeof(char));
		if (fileContent) {
			itemsRead = fread(fileContent, fileLength, 1, fp);
			printf("Total no of items read:%d\n", itemsRead);
		}
		fclose(fp);
	}

	//Merge the response lines and write to the socket
	lengthOfFinal = strlen(responseLine1) + strlen(responseLine2)
			+ strlen(responseLine3) + strlen(fileContent);
	printf("**8\n");
	printf("Length of Final is %d\n", lengthOfFinal);
	finalResponse = (char *) calloc(lengthOfFinal, sizeof(char));
	strcat(finalResponse, responseLine1);
	strcat(finalResponse, responseLine2);
	strcat(finalResponse, responseLine3);
	strcat(finalResponse, fileContent);

	return finalResponse;
}

struct httpRequest processGetRequest(char line[],
		struct httpRequest httpRequestObj) {

	sscanf(line, "%s %s %s", httpRequestObj.httpRequestType,
			httpRequestObj.httpRequestLocation, httpRequestObj.httpVersion);

	if ((strstr(httpRequestObj.httpRequestType, "GET") == NULL)
			|| (strstr(httpRequestObj.httpRequestLocation, "/") == NULL)
			|| !((strstr(httpRequestObj.httpVersion, "HTTP/1.0") != NULL)
					|| (strstr(httpRequestObj.httpVersion, "HTTP/1.1") != NULL))) {
		strcpy(httpRequestObj.httpRequestType, "INVALID");
	}

	printf("httpRequestType:%s\n", httpRequestObj.httpRequestType);
	printf("httpRequestLocation:%s\n", httpRequestObj.httpRequestLocation);
	printf("httpVersion:%s\n", httpRequestObj.httpVersion);

	return httpRequestObj;
}

/**
 *  This function creates a new socket and binds the port number and returns the socket file descriptor
 *
 *  portNum: The port number to which the server should listen to
 *
 *  returns: socket file descriptor
 */
int startServer(int portNum) {
	int sfd;
	struct sockaddr_in server_addr;
	printf("Starting the webserver... port:%d\n", portNum);
	sfd = socket(PF_INET, SOCK_DGRAM, 0);

	if (sfd < 0) {
		printf("Socket open failure\n");
		exit(0);
	}
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(portNum);

	/**binding socket to the required port**/
	if (bind(sfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		printf("binding error");
		exit(0);
	}
	printf("**Server Binding set to :%d\n", server_addr.sin_addr.s_addr);
	printf("**Server Binding set to port:%d\n", server_addr.sin_port);
	printf("**Server Binding set to family:%d\n", server_addr.sin_family);
	printf("UDP Webserver started successfully\n");
	return sfd;
}

