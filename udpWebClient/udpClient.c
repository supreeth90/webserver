/*
 *
 *  Created on: Sep 19, 2015
 *      Author: supreeth
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>

const int SERVER_IP = 1;
const int PORT_NUM = 2;
const int FILE_NAME = 3;

struct fileNameList {
	char fileNames[10][50];
	int noOfFiles;
};

struct serverIpAndSocket {
	int sockfd;
	struct sockaddr_in server_address;
};
int isPersistentRequest(char *persistentArg) {
	if (strcmp(persistentArg, "p") == 0) {
		return 1;
	} else {
		return 0;
	}
}

long calculateTime(struct timeval startTime, struct timeval endTime) {
	long elapsedTime = (endTime.tv_sec * 1000000 + endTime.tv_usec)
			- (startTime.tv_sec * 1000000 + startTime.tv_usec);
	return elapsedTime;
}

/**
 * Creates HTTP request with the filename specified
 */
char* processArgumentsAndCreateHttpRequest(char *argv[], char *fileName) {
	printf("Entered processArgumentsAndCreateHttpRequest\n");
	char requestLine[100] = { 0 };
	char hostLine[100] = { 0 };
	char connectionType[100] = { 0 };
	char *finalRequest;
	int finalRequestLength = 0;
	strcat(requestLine, "GET /");
	strcat(requestLine, fileName);
	strcat(requestLine, " HTTP/1.1\n");

	strcat(hostLine, "Host: ");
	strcat(hostLine, argv[SERVER_IP]);
	strcat(hostLine, ":");
	strcat(hostLine, argv[PORT_NUM]);
	strcat(hostLine, "\n");

	strcat(connectionType, "Connection: close\n");
	finalRequestLength = strlen(requestLine) + strlen(hostLine)
			+ strlen(connectionType);
	finalRequest = (char*) calloc(finalRequestLength, sizeof(char));
	strcat(finalRequest, requestLine);
	strcat(finalRequest, hostLine);
	strcat(finalRequest, connectionType);
	printf("Exit processArgumentsAndCreateHttpRequest\n");
	return finalRequest;
}

/**
 *
 *  Creates socket and connects to the server
 */
struct serverIpAndSocket createSocketAndServerConnection(char *serverAddress,
		char *portNumString) {
	struct hostent *server;
	struct sockaddr_in server_address;
	int sfd;
	int portNum;
	struct serverIpAndSocket serverIpAndSocketObj;
	portNum = atoi(portNumString);
	sfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sfd < 0)
		printf("ERROR opening socket\n");
	server = gethostbyname(serverAddress);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	bcopy((char *) server->h_addr,
	(char *)&server_address.sin_addr.s_addr,
	server->h_length);
	server_address.sin_port = htons(portNum);

	serverIpAndSocketObj.sockfd = sfd;
	serverIpAndSocketObj.server_address = server_address;
	return serverIpAndSocketObj;
}

/**
 * This function is mainly used to read the file specified by fileName and
 * construct the structure with the filenames entered in the file seperated by newline
 */
struct fileNameList splitFileNamesFromTheFile(char *fileName) {
	FILE *fp;
	int i = 0;
	int fileLength = 0;
	char *fileContent;
	char *fileNameTemp;
	struct fileNameList fileNameListObj;

	fileNameListObj.noOfFiles = 0;
	fp = fopen(fileName, "r");
	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		fileLength = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fileContent = calloc(1, fileLength);
		if (fileContent) {
			fread(fileContent, 1, fileLength, fp);
		}
		fclose(fp);
		printf("FileContent:%s\n", fileContent);

		fileNameTemp = strtok(fileContent, "\n");
		while (fileNameTemp != NULL) {
			strcpy(fileNameListObj.fileNames[i], fileNameTemp);
			fileNameListObj.noOfFiles++;
			fileNameTemp = strtok(NULL, "\n");
			i++;
		}
	}
	return fileNameListObj;
}

/**
 *
 * This function mainly sends http requests and receives the file from the http server
 *
 */
void sendHttpRequest(struct serverIpAndSocket serverIpAndSocketObj,
		int fileNumber, struct fileNameList fileNameListObj, char *argv[]) {
	char *finalHttpRequest;
	char buffer[1000];
	int packetNumber = 1;
	int length = 0;
	int n;
	FILE *fp = fopen(fileNameListObj.fileNames[fileNumber], "w");
	finalHttpRequest = processArgumentsAndCreateHttpRequest(argv,
			fileNameListObj.fileNames[fileNumber]);
	printf("*** Final Request being sent:%s\n", finalHttpRequest);
	printf("server_add::%d\n",
			serverIpAndSocketObj.server_address.sin_addr.s_addr);
	printf("server_add_port::%d\n",
			serverIpAndSocketObj.server_address.sin_port);
	printf("server_add_family::%d\n",
			serverIpAndSocketObj.server_address.sin_family);
	bzero(buffer, 1000);
	n = sendto(serverIpAndSocketObj.sockfd, finalHttpRequest,
			strlen(finalHttpRequest), 0, &(serverIpAndSocketObj.server_address),
			sizeof(struct sockaddr_in));
	if (n < 0) {
		printf("ERROR writing to socket\n");
	}
	bzero(buffer, 1000);

	while ((n = recvfrom(serverIpAndSocketObj.sockfd, buffer, 1000, 0, NULL,
	NULL)) > 0) {
		length = length + n;
		if (fp != NULL) {
			fwrite(buffer, 1, n, fp);
		}
		packetNumber++;
		if (n < 1000) { //To check if this is the last packet
			printf("Packet size less than 1000\n");
			break;
		}
		bzero(buffer, 1000);
	}
	fclose(fp);

	printf("***Total Length of content from the server:%d\n", length);
	printf("***Total number of packets received from the server:%d\n",
			packetNumber);
}
int main(int argc, char *argv[]) {
	printf("Starting the client\n");
	struct fileNameList fileNameListObj;
	struct timeval startTime;
	struct timeval endTime;
	long timeElapsed = 0;
	struct serverIpAndSocket serverIpAndSocketObj;
	if (argc != 4) {
		printf(
				"Please provide in this format : <server-ip> <server-port> <file-name>\n");
		exit(1);
	}
	serverIpAndSocketObj = createSocketAndServerConnection(argv[SERVER_IP],
			argv[PORT_NUM]);
	gettimeofday(&startTime, NULL);
	strcpy(fileNameListObj.fileNames[0], argv[FILE_NAME]);
	sendHttpRequest(serverIpAndSocketObj, 0, fileNameListObj, argv);
	gettimeofday(&endTime, NULL);
	timeElapsed = calculateTime(startTime, endTime);

	printf("Total Time taken is %ld\n", timeElapsed);
	close(serverIpAndSocketObj.sockfd);
}
