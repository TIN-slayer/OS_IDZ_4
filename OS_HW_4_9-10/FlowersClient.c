#include "DieWithError.c"
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <pthread.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <unistd.h>     /* for close() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */

#define BUFFER_SIZE 11
#define FLOWERS_NUM 10 /* Size of receive buffer */
#define TIMEOUT_SECS 2       /* Seconds between retransmits */

void CatchAlarm(int ignored) {}   /* Handler for SIGALRM */

// Процессы жизни цветов
void flower_proc(int id, unsigned short servPort, char* servIP) {
	// Создададим 10 процессов жизни цветка
	if (id < FLOWERS_NUM) {
		int pid = fork();
		if (pid < 0) {
			printf("Can\'t fork\n");
			exit(-1);
		}
		else if (pid == 0) {
			// Child
			flower_proc(id + 1, servPort, servIP);
			return;
		}
	}

	int sock;                      /* Socket descriptor */
	struct sockaddr_in servAddres; /* Echo server address */
	struct sockaddr_in servAddr;   /* Echo server address */
	struct sockaddr_in fromAddr;   /* Source address of echo */
	unsigned int fromSize = sizeof(fromAddr);
	struct sigaction myAction;       /* For setting signal handler */

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
	memset(&servAddr, 0, sizeof(servAddr));       /* Zero out structure */
	servAddr.sin_family = AF_INET;                /* Internet addr family */
	servAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
	servAddr.sin_port = htons(servPort);          /* Server port */

	int buffer[BUFFER_SIZE]; // Буффер, который в дальнеём буду использовать для
	int recvMsgSize = 1;
	// принятия
	// сообщений от сервера

	//......................................................................................

	int flag = 1;
	srand(time(NULL) + id);
	while (flag && recvMsgSize > 0) {
		sleep(rand() % 10 + 1);

		int access = -1;
		// acces to flowers
		while (access == -1) {
			// Cообщаю серверу, что готов к приёму
			if (sendto(sock, (int[]) { 1 }, sizeof(int), 0, (struct sockaddr*)&servAddr,
				sizeof(servAddr)) != sizeof(int))
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

		if (buffer[id] == 0) {
			++buffer[id];
			printf("Flower %d is thirsty\n", id);
		}
		else { // Если shar_list[id] < 0, то цветок полили несколько раз (такого
			// по идее быть не может) и он умирает. Если shar_list[id] > 0, то
			// цветок не полили вовремя и он тоже умирает. В обоих случаях
			// ставим ему -1 (индикатор смерти цветка для садовника)
			buffer[id] = -1;
			printf("Flower %d has died\n", id);
			//// Cообщаю серверу, что цветок умер
			// if (send(sock, (int[]) { 0 }, sizeof(int), 0) != sizeof(int))
			//     DieWithError("send() sent a different number of bytes than
			//     expected");
			flag = 0;
		}

		/*for (int i = 1; i < BUFFER_SIZE; ++i) {
						printf("%d ", buffer[i]);
		}
		printf("\n");*/
		/* Отправляю инфу о цветках */
		buffer[0] = 2;
		if (sendto(sock, buffer, sizeof(int) * BUFFER_SIZE, 0,
			(struct sockaddr*)&servAddr,
			sizeof(servAddr)) != sizeof(int) * BUFFER_SIZE)
			DieWithError("sendto() sent a different number of bytes than expected");
	}
	close(sock);
	exit(0);
}

int main(int argc, char* argv[]) {
	unsigned short servPort; /* Echo server port */
	char* servIP;            /* Server IP address (dotted quad) */

	if (argc != 3) /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n",
			argv[0]);
		exit(1);
	}

	servIP = argv[1];         /* First arg: server IP address (dotted quad) */
	servPort = atoi(argv[2]); /* Use given port */
	flower_proc(1, servPort, servIP);
	return 0;
}
