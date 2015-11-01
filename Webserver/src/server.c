/*
 * server.c
 *
 *  Created on: Sep 3, 2015
 *      Author: supreeth
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>

#include "HttpProcessor.h"

const int MAX_THREADS=100;

/** MAIN function to start the server **/
int main(int argc, char *argv[])
{
     int sfd, clientsockfd;
     int i=0;
     pthread_t threadArray[MAX_THREADS];
     int threadStatus;
     socklen_t cliAddrLen;
     struct sockaddr_in client_addr;
	int portnum = 8080;

	if (argc < 2) {
		printf("Please provide a port number");
		exit(1);
	}
	if (argv[1] != NULL) {
		portnum = atoi(argv[1]);
	}

     //Start the server
     sfd=startServer(portnum);


     cliAddrLen = sizeof(client_addr);

     for(i=0;i<MAX_THREADS;i++) {
    	 printf("waiting for new Request %d\n",i+1);

    	 /** Starting to look for client **/
    	 clientsockfd = accept(sfd,
    	                  (struct sockaddr *) &client_addr,
    	                  &cliAddrLen);

    	 /** Client Found, creating a new thread to process the requests **/
    	 printf("Creating thread %d\n",i+1);
		if ((threadStatus = pthread_create((threadArray + i), NULL,
				&processHttpRequests, clientsockfd))) {
			printf("Thread creation failed: %d\n", threadStatus);
		}

     }

     if (clientsockfd < 0) {
          printf("Accept error\n");
     }

     close(sfd);
     return 0;
}





