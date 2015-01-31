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
char  UserNamePassword[65];
char* token;
enum STATES StateNum;
bool InvalidInput;

int buf;
int fd;
unsigned int count;
struct sockaddr_in addr;
unsigned int addrlen = sizeof(struct sockaddr_in);
struct message_s message;

int OpenConnection(in_addr_t ip, unsigned short port){

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
		return 0;
	}


	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )		// connect to the destintation
	{
		printf("Server connection rejected.\n");
		return 0;
	}
	else {
        message.length = htonl(message.length);
        //printf("%d",sizeof(unsigned int) );

        write(fd, &message, sizeof(message));

        count = read(fd, &message, sizeof(message));

        message.length = ntohl(message.length);

        /*Check the message*/
        if (count = 12)
            printf("Server connection accepted.\n");
        else
            printf("Server connection rejected.\n");
        //printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);
        StateNum++;

        return 1;
	}


}

void Authentication(){

    int i = 0;

    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xA3;
	message.status = 0x00;
	message.length = 12+strlen(UserName)+strlen(Password)+2;

    message.length = htonl(message.length);
	//printf("%d",sizeof(unsigned int) );

    for (i= 0 ; i<strlen(UserName);i++){
        UserNamePassword[i] = *(UserName+i);
    }

    i++;
    UserNamePassword[i] = 0x32;

    for (i = 0; i<strlen(Password); i++){
        UserNamePassword[strlen(UserName)+1+i] = *(Password+i);   }



	write(fd, &message, sizeof(message));
	write(fd, &UserNamePassword, sizeof(UserNamePassword));

    count = read(fd, &message, sizeof(message));
    message.length = ntohl(message.length);

    if (message.status == 1){
        printf("Authentication granted.\n");
        StateNum++;
    }
    else{
        printf("ERROR: Authentication rejected. Connection closed.\n");
        StateNum--;
    }
	
	memset(UserNamePassword, 0, sizeof(UserNamePassword));

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

                if (Command != NULL){
                    if (strcmp(Command, "auth") != 0)
                        InvalidInput = true;
                }
                else
                    InvalidInput = true;

                if (UserName == NULL)
                    InvalidInput = true;

                if (Password == NULL)
                    InvalidInput = true;


//                if ((ip = inet_addr(Ip)) == -1)
//                    InvalidInput = true;
//
//                if ( (port = atoi(Port)) <= 0)
//                    InvalidInput = true;
                break;
            }
            case Authed :{
                break;
            }
        }

        if (InvalidInput)
            SyntaxError();
        else{
            switch (StateNum){
            case Idle:
                OpenConnection(ip,port);
            break;
            case Opened:
                Authentication();
            break;
            }

        }



    }

}
