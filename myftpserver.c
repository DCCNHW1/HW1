/*SERVER SIDE*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef int bool;
#define true 1
#define false 0

struct message_s {
    unsigned char protocol[6]; /* protocol magic number (6 bytes) */
    unsigned char type; /* type (1 byte) */
    unsigned char status; /* status (1 byte) */
    unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

struct record_s {
    unsigned char UserName[32];
    unsigned char Password[32];
};

struct record_s Record[200];

void ReadRecords(){

}

void OpenConnection(unsigned short port)
{
	int fd, accept_fd, count, buf, client_count;
	struct sockaddr_in addr, tmp_addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);
	struct message_s messages;

	fd = socket(AF_INET, SOCK_STREAM, 0);		// Create a TCP Socket

	if(fd == -1)
	{
		perror("socket()");
		exit(1);
	}

    memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	// After the setup has been done, invoke bind()

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, reuseaddr_len);

	if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	{
		perror("bind()");
		exit(1);
	}

	// Switch to listen mode by invoking listen()

	if( listen(fd, 1024) == -1 )
	{
		perror("listen()");
		exit(1);
	}

	while (1){
        if( (accept_fd = accept(fd, (struct sockaddr *) &tmp_addr, &addrlen)) == -1)
		{
			perror("accept()");
			exit(1);
		}

		//client_count++;

		// Read from network, buf is 4 bytes.
		// Since the opposite side sends 4 bytes of data,
		// "count" should report 4 bytes.
		count = read(accept_fd, &messages, sizeof(messages));

		printf("%d %u\n",count,&messages.type);

        close(fd);
	}
}

int main(int argc, char **argv){

	unsigned short port;

    if(argc != 2)
	{
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	ReadRecords();

	port = atoi(argv[1]);

    OpenConnection(port);
}
