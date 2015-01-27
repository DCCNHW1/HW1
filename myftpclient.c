/*CLIENT SIDE*/

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

enum STATES {Idle, Opened, Authed};

struct message_s {
    unsigned char protocol[6]; /* protocol magic number (6 bytes) */
    unsigned char type; /* type (1 byte) */
    unsigned char status; /* status (1 byte) */
    unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

char input[255];
char* Command;
char* Ip;
char* Port;
char* UserName;
char* Password;
char* token;
enum STATES StateNum;
bool InvalidInput;

void OpenConnection(in_addr_t ip, unsigned short port){
    int buf;
	int fd;
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);
	struct message_s message;

	message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xA1;
	message.status = 0x00;
	message.length = 12;


	fd = socket(AF_INET, SOCK_STREAM, 0);

	if(fd == -1)
	{
		perror("socket()");
		exit(1);
	}


	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )		// connect to the destintation
	{
		perror("connect()");
		exit(1);
	}

	write(fd, &message, sizeof(message));
}

void SyntaxError(){
    switch (StateNum){
        case Idle:
            printf("Usage: open [IP address] [port number]\n");
        break;

        case Opened:
            printf("Usage: auth [username] [password]\n");
        break;

        case Authed:
            printf("Usage: ls\nget [filename]\nput [filename]\nquit\n");
    }
}

int main(){
	in_addr_t ip;
	unsigned short port;

	StateNum = Idle;

    while (1){
        InvalidInput = false;
        printf("Client> ");

        fgets(input,255,stdin);
        input[strlen(input)-1] ='\0';

        switch (StateNum){
            case Idle :{
                Command = strtok(input," ");
                Ip = strtok(NULL," ");
                Port = strtok(NULL," ");

                if ( Command != NULL){
                    if (strcmp(Command, "open") != 0)
                        InvalidInput = true;
                }
                else
                    InvalidInput = true;


                if ( Ip != NULL){
                    if ((ip = inet_addr(Ip)) == -1)
                        InvalidInput = true;
                }
                else
                    InvalidInput = true;

                if ( Port != NULL){
                    if ( (port = atoi(Port)) <= 0)
                        InvalidInput = true;
                }
                else
                    InvalidInput = true;

                break;
            }
            case Opened :{
                Command = strtok(input," ");
                UserName = strtok(NULL," ");
                Password = strtok(NULL," ");

                if (strcmp(Command, "auth") != 0)
                    InvalidInput = true;

                if ((ip = inet_addr(Ip)) == -1)
                    InvalidInput = true;

                if ( (port = atoi(Port)) <= 0)
                    InvalidInput = true;
                break;
            }
            case Authed :{
                break;
            }
        }

        if (InvalidInput)
            SyntaxError();
        else{
            OpenConnection(ip,port);
        }



    }

}
