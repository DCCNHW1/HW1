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

enum STATES {Idle, Opened, Authed};

struct message_s {
    unsigned char protocol[6]; /* protocol magic number (6 bytes) */
    unsigned char type; /* type (1 byte) */
    unsigned char status; /* status (1 byte) */
    unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__ ((packed));

struct record_s {
    char UserName[32];
    char Password[32];
};

struct record_s Record[200];
unsigned int NumRecord;
char UserNamePassword[200];

int fd;
struct sockaddr_in addr, tmp_addr;
unsigned int addrlen = sizeof(struct sockaddr_in);
int reuseaddr = 1;
socklen_t reuseaddr_len;

unsigned int ReadRecords(){
    FILE* fp = fopen("access.txt","r");

    unsigned int i = 0;
    int c ;

    while (1){
        c = fscanf(fp,"%s %s",  (char *)Record[i].UserName, (char *)Record[i].Password);
        i++;
        if (c<=0)
            break;
    }

    fclose(fp);

    return (i-1);
}

void Initialisation(unsigned short port){

//    enum STATES ClientState = Idle;
//	struct message_s message;

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

    reuseaddr_len = sizeof(reuseaddr);
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
}

int OpenConnection()
{
    int count ;
    int accept_fd;

	struct message_s message;

	while (1){
        if( (accept_fd = accept(fd, (struct sockaddr *) &tmp_addr, &addrlen)) == -1)
		{
			perror("accept()");
			exit(1);
		}

		count = read(accept_fd, &message, sizeof(message));

        message.length = ntohl(message.length);

        /*Check the message*/

		printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

        message.protocol[0] = 0xe3;
        message.protocol[1] = 'm';
        message.protocol[2] = 'y';
        message.protocol[3] = 'f';
        message.protocol[4] = 't';
        message.protocol[5] = 'p';
        message.type = 0xA2;
        message.status = 0x00;
        message.length = 12;

		message.length = htonl(message.length);

		write(accept_fd, &message, sizeof(message));

        //close(accept_fd);
        return accept_fd;
	}
}

void Authentication(int accept_fd){
    int count,i;
    int length;
    struct message_s message;
    bool match = false;
    char *UserName;
    char *Password;

    count = read(accept_fd, &message, sizeof(message));
    message.length = ntohl(message.length);

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    length = read(accept_fd, &UserNamePassword, sizeof(UserNamePassword));

    printf("length:%d UserNamePassword:%s\n",length, UserNamePassword);

    UserName = UserNamePassword;
    printf("User:%s\n", UserName);
    Password = (UserNamePassword+strlen(UserNamePassword)+1);
    printf("Pass:%s\n", Password);

    for (i=0;i<NumRecord;i++){
        if (strcmp(UserName, Record[i].UserName) == 0)
            if (strcmp(Password, Record[i].Password) == 0)
                match = true;
    }

    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xA4;
	message.length = 12;

    if (match){
        message.status = 1;
    }
    else{
        message.status = 0;
    }

    message.length = htonl(message.length);
	write(accept_fd, &message, sizeof(message));

	if (!match)
        close(accept_fd);

}

int main(int argc, char **argv){

	unsigned short port;

	int accept_fd;

    if(argc != 2)
	{
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	NumRecord = ReadRecords();

	port = atoi(argv[1]);

	Initialisation(port);

    while (1){
        accept_fd = OpenConnection();
        Authentication(accept_fd);
    }

}
