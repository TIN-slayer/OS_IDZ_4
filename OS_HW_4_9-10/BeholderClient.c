#include "DieWithError.c"
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <pthread.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <errno.h>      /* for errno and EINTR */
#include <unistd.h>     /* for close() */
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
	unsigned int sleep_time = 100000;

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
	int buffer[BUFFER_SIZE]; // ������, ������� � ������� ���� ������������ ���
	// ��������
	// ��������� �� �������
	int recvMsgSize = 1; /* Size of received message */
	unsigned int fromSize = sizeof(fromAddr);
	while (recvMsgSize > 0) {
		// C������ �������, ��� ����� � �����
		if (sendto(sock, (int[]) { 3 }, sizeof(int), 0, (struct sockaddr*)
			&servAddr, sizeof(servAddr)) != sizeof(int))
			DieWithError("sendto() sent a different number of bytes than expected");

		/* get flowers status */
		alarm(TIMEOUT_SECS);        /* Set the timeout */
		//printf("B");
		while ((recvMsgSize = recvfrom(sock, buffer, sizeof(int) * BUFFER_SIZE, 0,
			(struct sockaddr*)&fromAddr, &fromSize)) < 0) {
			if (errno == EINTR)     /* Alarm went off  */
			{
				close(sock);
				exit(0);
			}
		}

		for (int i = 1; i < BUFFER_SIZE; ++i) {
			printf("%d ", buffer[i]);
		}
		printf("\n");
		usleep(sleep_time);
	}

	close(sock);
	exit(0);
}
