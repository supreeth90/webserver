/*
 * HttpProcessor.h
 *
 *  Created on: Sep 20, 2015
 *      Author: supreeth
 */

#ifndef HTTPPROCESSOR_H_
#define HTTPPROCESSOR_H_

struct httpRequest{
	char httpRequestType[30];
	char httpRequestLocation[30];
	char httpVersion[30];
	char connectionType[30];
	time_t requestEndTime;
	int requestsLeft;			// -1 initially then set to MAX_CONNECTIONS, 1 for Non-persistent HTTP
	int persistentRequest;    // 0 for non-persistent , 1 for persistent
};

int isKeepAliveRequest(struct httpRequest httpRequestObj);

int isFirstKeepAliveRequest(struct httpRequest httpRequestObj);

void processHttpRequests(int clientSockFd);

int sendHttpResponse(int clientSockFd, char *finalHttpResponse);

struct httpRequest parseHttpRequest(char *buffer, int firstRequest,
		struct httpRequest httpRequestObj);

char *formulateHttpResponse(int clientSockFd, struct httpRequest httpRequestObj);

struct httpRequest processConnectionType(char line[], struct httpRequest httpRequestObj);

struct httpRequest processGetRequest(char line[],struct httpRequest httpRequestObj);

void connectAndProcessHttp(int sfd);

int startServer(int portNum);

#endif /* HTTPPROCESSOR_H_ */
