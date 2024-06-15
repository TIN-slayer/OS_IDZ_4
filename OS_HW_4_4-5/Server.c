#include "DieWithError.c"
#include <pthread.h> /* for POSIX threads */
#include <stdio.h>   /* for printf() and fprintf() */
#include <stdlib.h>  /* for atoi() and exit() */
#include <unistd.h>  /* for close() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */

#define BUFFER_SIZE 11
#define FLOWERS_NUM 10

pthread_mutex_t mutex;
int gardenerCount = 0;
int flowers[BUFFER_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void* ServerThread(void* threadArgs);
void HandleGardenerId(int clntSocket);
void HandleActiveClnt(int clntSocket);


int main(int argc, char* argv[]) {
	int sock;                        /* Socket */
	int clntSock;                  /* Socket descriptor for client */
	struct sockaddr_in servAddr; /* Local address */
	struct sockaddr_in clntAddr; /* Client address */
	unsigned int cliAddrLen;         /* Length of incoming message */
	unsigned short servPort;   /* Server port */
	pthread_t threadID;            /* Thread ID from pthread_create() */
	struct ThreadArgs* threadArgs; /* Pointer to argument structure for thread */
	int garden_block = 0;

	if (argc != 2) /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage:  %s <SERVER PORT>\n", argv[0]);
		exit(1);
	}

	servPort = atoi(argv[1]); /* First arg:  local port */

	/* Create socket for sending/receiving datagrams */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	/* Construct local address structure */
	memset(&servAddr, 0, sizeof(servAddr));   /* Zero out structure */
	servAddr.sin_family = AF_INET;                /* Internet address family */
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	servAddr.sin_port = htons(servPort);      /* Local port */

	/* Bind to the local address */
	if (bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
		DieWithError("bind() failed");

	pthread_mutex_init(&mutex, NULL);


	for (;;) /* run forever */
	{
		/* Identify client type */
		int buffer[BUFFER_SIZE]; /* Buffer for echo string */
		int recvMsgSize; /* Size of received message */

		/* Set the size of the in-out parameter */
		cliAddrLen = sizeof(clntAddr);

		/* Block until receive message from a client */
		if ((recvMsgSize = recvfrom(sock, buffer, sizeof(int) * BUFFER_SIZE, 0,
			(struct sockaddr*)&clntAddr, &cliAddrLen)) < 0)
			DieWithError("recvfrom() failed");

		//printf("Handling client %s\n", inet_ntoa(clntAddr.sin_addr));

		// 0 - Connect Gardener
		// 1 - Request for flowers
		// 2 - Response about flowers
		switch (buffer[0]) {
		case 0:
			/* Send received datagram back to the client */
			if (sendto(sock, (int[]) { ++gardenerCount }, sizeof(int), 0,
				(struct sockaddr*)&clntAddr, sizeof(clntAddr)) != sizeof(int))
				DieWithError("sendto() sent a different number of bytes than expected");
			break;
		case 1:
			if (garden_block == 1) {
				if (sendto(sock, (int[]) { -1 }, sizeof(int), 0,
					(struct sockaddr*)&clntAddr, sizeof(clntAddr)) != sizeof(int))
					DieWithError("sendto() sent a different number of bytes than expected");
			}
			else {
				if (sendto(sock, flowers, sizeof(int) * BUFFER_SIZE, 0,
					(struct sockaddr*)&clntAddr, sizeof(clntAddr)) != sizeof(int) * BUFFER_SIZE)
					DieWithError("sendto() sent a different number of bytes than expected");
				garden_block = 1;
			}
			break;
		case 2:
			for (int i = 1; i < BUFFER_SIZE; ++i) {
				flowers[i] = buffer[i];
				// printf("%d ", flowers[i]);
			}
			// printf("\n");
			garden_block = 0;
			break;
		}
	}
	close(sock);
	/* NOT REACHED */
}
