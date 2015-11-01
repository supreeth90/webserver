/*
 * udpHttpServer.c
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
#include <pthread.h>
#include <time.h>

#include "udpHttpProcessor.h"

const int MAX_THREADS=100;

/** MAIN function to start the server **/
int main(int argc, char *argv[]) {
	int sfd = 0;
	int portnum = 8080;

	if (argc < 2) {
		printf("Please provide a port number");
		exit(1);
	}
	if (argv[1] != NULL) {
		portnum = atoi(argv[1]);
	}

	//Start the server
	sfd = startServer(portnum);

	processHttpRequests(sfd);

	close(sfd);
	return 0;
}


