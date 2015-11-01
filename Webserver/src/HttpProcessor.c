/*
 * HttpProcessor.c
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

#include "HttpProcessor.h"

const int BUFFER_SIZE = 5 * 1024;

const int MAX_CONNECTIONS = 10000;
const int TIMEOUT_OF_CONNECTIONS = 10;

int isKeepAliveRequest(struct httpRequest httpRequestObj) {
	if (strstr(httpRequestObj.connectionType, "keep-alive") != NULL) {
		return 1;
	} else {
		return 0;
	}
}

int isFirstKeepAliveRequest(struct httpRequest httpRequestObj) {
	if (isKeepAliveRequest(httpRequestObj) == 1
			&& (httpRequestObj.requestsLeft == MAX_CONNECTIONS
					|| httpRequestObj.requestsLeft == -1)) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * 	This function essentially processes the HTTP requests(read, parse,read from a file and send response back)
 *
 * 	clientSockFd: is the client socket Fd received after TCP connection was established with the client
 *
 */
void processHttpRequests(int clientSockFd) {
	printf("***processing processHttpRequests for %d\n", clientSockFd);
	char *buffer = (char *) calloc(BUFFER_SIZE, sizeof(char));
	struct httpRequest httpRequestObj;
	int firstRequest = 1;
	char *finalHttpResponse;

	while (firstRequest == 1 || httpRequestObj.requestsLeft > 0) {
		bzero(buffer, BUFFER_SIZE);
		printf("Waiting for THE NEXT HTTP REQUEST\n");

		/* Read from the socket for new HTTP Requests */
		while (read(clientSockFd, buffer, BUFFER_SIZE) <= 0)
			;
		printf("**Request received is::%s\n", buffer);

		/* Parse the HTTP request */
		httpRequestObj = parseHttpRequest(buffer, firstRequest, httpRequestObj);

		printf("After processGetRequest::\n");

		/*Create the HTTP response by reading the requested file*/
		finalHttpResponse = formulateHttpResponse(clientSockFd, httpRequestObj);

		/*Send the HTTP response back to the client*/
		sendHttpResponse(clientSockFd, finalHttpResponse);

		/* to check if connection needs to be closed in the next loop */
		if (isKeepAliveRequest(httpRequestObj) == 0) {
			httpRequestObj.requestsLeft = 0;
		} else {
			httpRequestObj.requestsLeft--;
		}
		firstRequest = 0;
	}
	printf("***Closing TCP connection::%d\n", clientSockFd);
	close(clientSockFd);
}

/*
 * This function sends the HTTP response to the client
 *
 * clientsockfd: Client Socket FD
 * finalHttpResponse: HTTP Response to be sent as a string
 *
 * returns: 1 if success 0 if failure
 */
int sendHttpResponse(int clientSockFd, char *finalHttpResponse) {
	int n = 0;
	struct timeval tv;
	struct timezone tz;
	printf("finalHttpResponse length::%lu\n", strlen(finalHttpResponse));
	write(clientSockFd, finalHttpResponse, strlen(finalHttpResponse));
	gettimeofday(&tv, &tz);
	printf("Time of last packet sent %d\n", tv.tv_usec);
	if (n > 0) {
		return 1;
	} else {
		return 0;
	}
}

struct httpRequest parseHttpRequest(char *buffer, int firstRequest,
		struct httpRequest httpRequestObj) {

	char *line;
	char *connectionTypeFound;
	/* get the first token */
	line = strtok(buffer, "\n\n");

	printf("\nFirst Line:: %s\n", line);
	httpRequestObj = processGetRequest(line, httpRequestObj);
	if (firstRequest == 1) {
		httpRequestObj.requestsLeft = -1;
	}

	/* walk through other tokens */
	line = strtok(NULL, "\n");
	while (line != NULL) {
		printf("\nLine:: %s\n", line);
		connectionTypeFound = strstr(line, "Connection:");
		if (connectionTypeFound != NULL) {
			printf("Found Connection Parameter in request...%s\n",
					connectionTypeFound);
			httpRequestObj = processConnectionType(connectionTypeFound,
					httpRequestObj);
		}
		line = strtok(NULL, "\n");
	}

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
	char keepAliveResponseLine[50] = { 0 };
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

			itemsRead = fread(fileContent, fileLength, 1, fp);
			printf("2Total no of items read:%d\n", itemsRead);
		}
		fclose(fp);
	}

	//Merge the response lines and write to the socket
	strcat(keepAliveResponseLine,
			"Connection: Keep-Alive\nKeep-Alive: max=5\n");
	if (isFirstKeepAliveRequest(httpRequestObj) == 1) {
		lengthOfFinal = strlen(responseLine1) + strlen(responseLine2)
				+ strlen(responseLine3) + strlen(keepAliveResponseLine)
				+ strlen(fileContent);
	} else {
		lengthOfFinal = strlen(responseLine1) + strlen(responseLine2)
				+ strlen(responseLine3) + strlen(fileContent);
	}
	printf("Length of Final is %d\n", lengthOfFinal);
	finalResponse = (char *) calloc(lengthOfFinal, sizeof(char));
	strcat(finalResponse, responseLine1);
	strcat(finalResponse, responseLine2);
	if (isFirstKeepAliveRequest(httpRequestObj) == 1) {
		strcat(finalResponse, keepAliveResponseLine);
	}
	strcat(finalResponse, responseLine3);
	strcat(finalResponse, fileContent);

	printf("Request left::%d\n", httpRequestObj.requestsLeft);
	return finalResponse;
}

struct httpRequest processConnectionType(char line[],
		struct httpRequest httpRequestObj) {
	sscanf(line, "Connection: %s", httpRequestObj.connectionType);
	if (isKeepAliveRequest(httpRequestObj) == 1
			&& isFirstKeepAliveRequest(httpRequestObj) == 1) {
		httpRequestObj.requestEndTime = time(NULL) + TIMEOUT_OF_CONNECTIONS;
		httpRequestObj.requestsLeft = MAX_CONNECTIONS;
		httpRequestObj.persistentRequest = 1;
	} else if (isKeepAliveRequest(httpRequestObj) == 0) { //End the connection next time as it is a connection close call from the client
		httpRequestObj.requestsLeft = 1;
	}
	printf("connectionType:%s\n", httpRequestObj.connectionType);
	return httpRequestObj;
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

void connectAndProcessHttp(int sfd) {
	struct sockaddr_in client_addr;
	int clientsockfd;
	socklen_t cliAddrLen;
	cliAddrLen = sizeof(client_addr);
	clientsockfd = accept(sfd, (struct sockaddr *) &client_addr, &cliAddrLen);
	if (clientsockfd < 0) {
		printf("Accept error");
	}
	processHttpRequests(clientsockfd);
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
	sfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sfd < 0) {
		printf("Socket open failure\n");
		exit(0);
	}
	bzero((char *) &server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portNum);

	/**binding socket to the required port**/
	if (bind(sfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		printf("binding error\n");
		exit(0);
	}
	listen(sfd, 5);
	printf("Webserver started successfully\n");
	return sfd;
}
