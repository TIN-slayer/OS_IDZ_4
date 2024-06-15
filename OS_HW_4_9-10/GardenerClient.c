#include "DieWithError.c"
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <pthread.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <unistd.h>     /* for close() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */

#define BUFFER_SIZE 11
#define FLOWERS_NUM 10 /* Size of receive buffer */
#define TIMEOUT_SECS 2       /* Seconds between retransmits */

void CatchAlarm(int ignored) {}   /* Handler for SIGALRM */

int main(int argc, char* argv[]) {
	int sock;                      /* Socket descriptor */
	struct sockaddr_in servAddr;   /* Echo server address */
	struct sockaddr_in fromAddr;     /* Source address of echo */
	unsigned short servPort;       /* Echo server port */
	char* servIP;                  /* Server IP address (dotted quad) */
	int bytesRcvd, totalBytesRcvd; /* Bytes read in single recv()
									  and total bytes read */
	struct sigaction myAction;       /* For setting signal handler */

	if (argc != 3) /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n",
			argv[0]);
		exit(1);
	}

	servIP = argv[1];         /* First arg: server IP address (dotted quad) */
	servPort = atoi(argv[2]); /* Use given port */

	/* Set signal handler for alarm signal */
	myAction.sa_handler = CatchAlarm;
	if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
		DieWithError("sigfillset() failed");
	myAction.sa_flags = 0;
	if (sigaction(SIGALRM, &myAction, 0) < 0)
		DieWithError("sigaction() failed for SIGALRM");

	/* Create a datagram/UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	/* Construct the server address structure */
	memset(&servAddr, 0, sizeof(servAddr));    /* Zero out structure */
	servAddr.sin_family = AF_INET;                 /* Internet addr family */
	servAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
	servAddr.sin_port = htons(servPort);     /* Server port */

	/* SEND CLNT TYPE TO GET IDENTIFIED BY SERVER */
	if (sendto(sock, (int[]) { 0 }, sizeof(int), 0, (struct sockaddr*)
		&servAddr, sizeof(servAddr)) != sizeof(int))
		DieWithError("sendto() sent a different number of bytes than expected");

	/* get gardener id */
	int buffer[BUFFER_SIZE]; // Буффер, который в дальнеём буду использовать для
	// принятия
	// сообщений от сервера
	int recvMsgSize; /* Size of received message */
	unsigned int fromSize = sizeof(fromAddr);
	if ((recvMsgSize = recvfrom(sock, buffer, sizeof(int), 0,
		(struct sockaddr*)&fromAddr, &fromSize)) < 0)
		DieWithError("recvfrom() failed");

	//......................................................................................

	int gardener_id = buffer[0];
	printf("Gardener %d\n", gardener_id);
	while (recvMsgSize > 0) {
		int dead_flowers_num = 0;

		int access = -1;
		// acces to flowers
		while (access == -1) {
			// Cообщаю серверу, что готов к приёму
			if (sendto(sock, (int[]) { 1 }, sizeof(int), 0, (struct sockaddr*)
				&servAddr, sizeof(servAddr)) != sizeof(int))
				DieWithError("sendto() sent a different number of bytes than expected");

			alarm(TIMEOUT_SECS);        /* Set the timeout */
			/* get flowers status */
			while ((recvMsgSize = recvfrom(sock, buffer, sizeof(int) * BUFFER_SIZE, 0,
				(struct sockaddr*)&fromAddr, &fromSize)) < 0) {
				if (errno == EINTR)     /* Alarm went off  */
				{
					close(sock);
					exit(0);
				}
			}
			access = buffer[0];
		}

		/*for (int i = 1; i < BUFFER_SIZE; ++i) {
			printf("%d ", buffer[i]);
		}
		printf("\n");*/

		int poured = 0;
		// Польём 1 рандомный голодный цветок
		for (int i = 0, j = rand() % FLOWERS_NUM; i < FLOWERS_NUM;
			++i, j = (j + 1) % FLOWERS_NUM) {
			int flower_id = j + 1;
			if (buffer[flower_id] > 0) {
				--buffer[flower_id];
				++poured;
				printf("Gardener %d has poured flower %d\n", gardener_id, flower_id);
				break;
			}
			else if (buffer[flower_id] < 0) {
				++dead_flowers_num;
			}
		}
		if (dead_flowers_num == FLOWERS_NUM) {
			printf("Gardener %d is fired because all flowers have died\n",
				gardener_id);
			break;
		}
		else if (!poured) {
			printf("No work for Gardener %d\n", gardener_id);
		}

		/*for (int i = 1; i < BUFFER_SIZE; ++i) {
			printf("%d ", buffer[i]);
		}
		printf("\n");*/
		/* Отправляю инфу о цветках */
		buffer[0] = 2;
		if (sendto(sock, buffer, sizeof(int) * BUFFER_SIZE, 0, (struct sockaddr*)
			&servAddr, sizeof(servAddr)) != sizeof(int) * BUFFER_SIZE)
			DieWithError("sendto() sent a different number of bytes than expected");
		// Садовник идёт на отдых
		sleep(rand() % 2 + 1);
	}
	// Cообщаю серверу, что меня уволили
	/*if (send(sock, (int[]) { 0 }, sizeof(int), 0) != sizeof(int))
		DieWithError("send() sent a different number of bytes than expected");*/

	close(sock);
	exit(0);
}
