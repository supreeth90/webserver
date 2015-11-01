/*
 * udpHttpProcessor.h
 *
 *  Created on: Sep 20, 2015
 *      Author: supreeth
 */

#ifndef UDPHTTPPROCESSOR_H_
#define UDPHTTPPROCESSOR_H_

struct httpRequest{
	char httpRequestType[30];
	char httpRequestLocation[30];
	char httpVersion[30];
	char connectionType[30];
	time_t requestEndTime;
	int requestsLeft;			// -1 initially then set to MAX_CONNECTIONS, 1 for Non-persistent HTTP
	int persistentRequest;    // 0 for non-persistent , 1 for persistent
};

void processHttpRequests(int clientSockFd);

int sendHttpResponse(int clientSockFd, struct sockaddr_in client_address,
		char *finalHttpResponse);

struct httpRequest parseHttpRequest(char *buffer,
		struct httpRequest httpRequestObj);

char *formulateHttpResponse(int clientSockFd, struct httpRequest httpRequestObj);


struct httpRequest processGetRequest(char line[],struct httpRequest httpRequestObj);

int startServer(int portNum);



#endif /* UDPHTTPPROCESSOR_H_ */
