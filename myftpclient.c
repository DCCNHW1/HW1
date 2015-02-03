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
#include <dirent.h>
#include <limits.h>

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
char* FileName;
char* Payload;
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

    strcpy(Payload,"");

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

    strcat(Payload, UserName);
    strcat(Payload, " ");
    strcat(Payload, Password);

	write(fd, &message, sizeof(message));
	write(fd, Payload, strlen(Payload)+1);

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
	

}

void LS(char *Command, char *FileName){
    int count;
    int TotalBytes;
    int ReadBytes =0;
    //char* Payload;
    strcpy(Payload,"");

    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xA5;
	message.status = 0x00;
	message.length = 12;

    message.length = htonl(message.length);

    write(fd, &message, sizeof(message));

    count = read(fd, &message, sizeof(message));
    message.length = ntohl(message.length);

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    TotalBytes = message.length-count;

    while (ReadBytes < TotalBytes){
        count = read(fd, Payload+ReadBytes, TotalBytes-ReadBytes);
        ReadBytes +=count;
    }

    printf("----- file list start -----\n");
    printf("%s",Payload);
    printf("----- file list end -----\n");

    //free(Payload);

}


void Get(char *Command, char *FileName){
    int count;
    int TotalBytes;
    int ReadBytes =0;
    int WroteBytes =0;
    int FileSize;
    FILE* Output;
    //char* Payload;
    strcpy(Payload,"");
    strcpy(Payload,FileName);

    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xA7;
	message.status = 0x00;
	message.length = 12+strlen(Payload)+1;

    message.length = htonl(message.length);

    write(fd, &message, sizeof(message));

    TotalBytes = strlen(Payload)+1;
    WroteBytes = 0;

    while (WroteBytes < TotalBytes){
        count = write(fd, Payload+WroteBytes, TotalBytes-WroteBytes);
        WroteBytes += count;
    }

    count = read(fd, &message, sizeof(message));
    message.length = ntohl(message.length);

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    if (message.status){
        count = read(fd, &message, sizeof(message));
        message.length = ntohl(message.length);

        printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

        FileSize = message.length-12;

        Output = fopen(FileName, "w+");

        TotalBytes = FileSize;
        ReadBytes =0;

        while (ReadBytes < TotalBytes){
            count = read(fd, Payload+ReadBytes, TotalBytes-ReadBytes);
            ReadBytes +=count;
        }
        message.length = ntohl(message.length);

        fwrite(Payload,1,FileSize,Output);

        fclose(Output);
    }
    else{
        printf("ERROR: file not found\n");
    }
}

void Put(char *Command, char *FileName){
    int count;
    int TotalBytes, ReadBytes, WroteBytes;
    int LastPosition;
    int FileSize;
    bool exist = false;
    bool ToCwd = false;
    DIR* dir;
    FILE* File;
    struct dirent* dptr;
    char* RealPath;
    char* DirPath;
    char* PureFilename;
    char* ptr;
    char pwd[1000];

    DirPath = malloc(sizeof(char)*1000);
    RealPath = malloc(sizeof(char)*1000);
    PureFilename = malloc(sizeof(char)*1000);

    realpath(FileName, RealPath);

    /*Find the last '/'*/
    ptr=strchr(RealPath,'/');

    while (ptr!=NULL)
    {
        //printf ("found at %d\n",ptr-RealPath+1);
        LastPosition = ptr-RealPath+1;
        ptr=strchr(ptr+1,'/');
    }

    *(RealPath+LastPosition-1) = 0x00;

    PureFilename = RealPath+LastPosition;

    strcpy(DirPath,RealPath);

    if ((dir = opendir("./")) == NULL)
    {
      printf("ERROR: Couldn't open directory.\n");
      exit(1);
    }

    /*Search inside filedir*/

    while((dptr = readdir(dir)) != NULL)
    {
        if (strcmp(dptr->d_name,".") != 0 && strcmp(dptr->d_name,"..")){
            if (strcmp(dptr->d_name, PureFilename) == 0)
                exist = true;
        }
    }

    /*Check whether the path is pointing to pwd*/


    getcwd(pwd,sizeof(pwd));
    if (strcmp(pwd,DirPath) == 0){
        ToCwd = true;
    }

    if (ToCwd){
        if (exist){
            message.protocol[0] = 0xe3;
            message.protocol[1] = 'm';
            message.protocol[2] = 'y';
            message.protocol[3] = 'f';
            message.protocol[4] = 't';
            message.protocol[5] = 'p';
            message.type = 0xA9;
            message.status = 0x00;

            File = fopen(PureFilename,"r");

                    /*Check file size*/
            fseek(File,0,SEEK_END);
            FileSize = ftell(File);
            fseek(File, 0, SEEK_SET);

            message.length = 12 +strlen(PureFilename)+1;
            message.length = htonl(message.length);

            write(fd, &message, sizeof(message));

            strcpy(Payload,PureFilename);

            TotalBytes = strlen(Payload)+1;
            WroteBytes = 0;

            while (WroteBytes < TotalBytes){
                count = write(fd, Payload+WroteBytes, TotalBytes-WroteBytes);
                WroteBytes += count;
            }

            count = read(fd, &message, sizeof(message));

            message.protocol[0] = 0xe3;
            message.protocol[1] = 'm';
            message.protocol[2] = 'y';
            message.protocol[3] = 'f';
            message.protocol[4] = 't';
            message.protocol[5] = 'p';
            message.type = 0xFF;
            message.status = 0x00;
            message.length = 12+FileSize;
            message.length = htonl(message.length);

            write(fd, &message, sizeof(message));
            strcpy(Payload,"");
            fseek(File, 0, SEEK_SET);

            fread(Payload,1,FileSize, File);

            TotalBytes = FileSize;

            printf("Total: %d Filesize: %d\n", FileSize,FileSize);

            WroteBytes = 0;

            while (WroteBytes < TotalBytes){
                count = write(fd, Payload+WroteBytes, TotalBytes-WroteBytes);
                WroteBytes += count;
            }

        }
        else
            printf("ERROE: the file does not exist.\n");
    }
    else{
        printf("ERROR: the input is not referring to current directory.\n");
    }

}

void Quit(){
    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xAB;
	message.status = 0x00;
	message.length = 12;

    message.length = htonl(message.length);

    send(fd, &message, sizeof(message), 0);
    count = recv(fd, &message, sizeof(message), 0);
	
	message.length = ntohl(message.length);

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    printf("Thank you.\n");

	close(fd);
	exit(0);
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

	Payload = (char* )malloc(sizeof(char)*32767);

	StateNum = Idle;

	//StateNum =  Authed;

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

                break;
            }
            case Authed :{
                Command = strtok(input," ");
                FileName = strtok(NULL," ");

                if (Command != NULL){
                    if ( (strcmp(Command, "ls") !=0) && (strcmp(Command, "put") !=0) && (strcmp(Command, "get") !=0) && (strcmp(Command, "quit") !=0))
                        InvalidInput = true;
                }
                else
                    InvalidInput = true;

                if ((strcmp(Command, "put")==0 ) || (strcmp(Command, "get") ==0))
                    if (FileName == NULL)
                        InvalidInput = true;
            }
            break;
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
            case Authed:{
                if (strcmp(Command, "ls") == 0)
                    LS(Command, FileName);

                if (strcmp(Command, "put") ==0)
                    Put(Command, FileName);

                if (strcmp(Command, "get") ==0)
                    Get(Command, FileName);

                if (strcmp(Command, "quit") ==0)
                    Quit();
	
            }

            break;
            }

        }

    }

}
