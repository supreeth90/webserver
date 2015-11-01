/*
 * client.c
 *
 *  Created on: Sep 16, 2015
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
const int CONNECTION_TYPE = 3;
const int FILE_NAME = 4;

const int BUFFER_SIZE = 1448;

struct fileNameList {
	char fileNames[10][50];
	int noOfFiles;
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
char* processArgumentsAndCreateHttpRequest(char *argv[], char *fileName,
		char *connection, int isLastRequest) {
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

	if (isPersistentRequest(connection) == 1 && isLastRequest != 1) {
		strcat(connectionType, "Connection: keep-alive\n");
	} else {
		strcat(connectionType, "Connection: close\n");
	}
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
int createSocketAndServerConnection(char *serverAddress, char *portNumString) {
	struct hostent *server;
	struct sockaddr_in server_address;
	int sfd;
	int portNum;

	portNum = atoi(portNumString);
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
		printf("ERROR opening socket\n");
	server = gethostbyname(serverAddress);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &server_address.sin_addr.s_addr,
			server->h_length);
	server_address.sin_port = htons(portNum);
	if (connect(sfd, (struct sockaddr *) &server_address,
			sizeof(server_address)) < 0)
		printf("ERROR connecting\n");
	return sfd;
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
void sendHttpRequest(int sockfd, int fileNumber,
		struct fileNameList fileNameListObj, char *argv[], int isLastRequest) {
	char *finalHttpRequest;
	char *buffer = (char *) calloc(BUFFER_SIZE, sizeof(char));
	int number = 0;
	long n;
	int length = 0;
	FILE *fp = fopen(fileNameListObj.fileNames[fileNumber], "w");
	finalHttpRequest = processArgumentsAndCreateHttpRequest(argv,
			fileNameListObj.fileNames[fileNumber], argv[CONNECTION_TYPE],
			isLastRequest);
	printf("*** Final Request being sent:%s\n", finalHttpRequest);
	bzero(buffer, BUFFER_SIZE);
	n = write(sockfd, finalHttpRequest, strlen(finalHttpRequest));
	if (n < 0) {
		printf("ERROR writing to socket\n");
	}
	bzero(buffer, BUFFER_SIZE);

	while ((n = read(sockfd, buffer, BUFFER_SIZE)) > 0) {
		length = length + n;
		if (fp != NULL) {
			fwrite(buffer, 1, n, fp);
		}
		number++;
		if (n < BUFFER_SIZE) {
			printf("End of the content\n");
			break;
		}
		bzero(buffer, BUFFER_SIZE);
	}
	fclose(fp);
	if (n < 0) {
		printf("ERROR reading from socket\n");
	}
	printf("***Total Length of content from the server:%d\n", length);
	printf("***Total number of times received from the server:%d\n", number);
}

int main(int argc, char *argv[]) {
	printf("Starting the client\n");
	int sfd;
	int i = 0;
	struct timeval startTime;
	struct timeval endTime;
	long timeElapsed = 0;
	struct fileNameList fileNameListObj;
	if (argc != 5) {
		printf(
				"Please provide in this format : <server-ip> <server-port> <connection-type> <file-name>\n");
		exit(1);
	}
	sfd = createSocketAndServerConnection(argv[SERVER_IP], argv[PORT_NUM]);

	gettimeofday(&startTime, NULL);

	if (isPersistentRequest(argv[CONNECTION_TYPE]) == 1) {
		fileNameListObj = splitFileNamesFromTheFile(argv[FILE_NAME]);
		while (i < fileNameListObj.noOfFiles) {
			if (i == fileNameListObj.noOfFiles - 1) {
				sendHttpRequest(sfd, i, fileNameListObj, argv, 1);
			} else {
				sendHttpRequest(sfd, i, fileNameListObj, argv, 0);
			}
			i++;
		}
	} else {
		strcpy(fileNameListObj.fileNames[0], argv[FILE_NAME]);
		sendHttpRequest(sfd, 0, fileNameListObj, argv, 0);
	}

	gettimeofday(&endTime, NULL);
	timeElapsed = calculateTime(startTime, endTime);

	printf("Total Time taken is %ld\n", timeElapsed);

	close(sfd);
}
