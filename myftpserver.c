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
#include <dirent.h>
#include <limits.h>

typedef int bool;
#define true 1
#define false 0

#define MAX_N 5

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

int fd;
struct sockaddr_in addr, tmp_addr;
unsigned int addrlen = sizeof(struct sockaddr_in);
int reuseaddr = 1;
socklen_t reuseaddr_len;

pthread_t connections[MAX_N];
int connection_count;

//Function Prototypes
unsigned int ReadRecords();
void Initialisation(unsigned short port);
int OpenConnection();
bool Authentication(int accept_fd);
void LS(int accept_fd);
void Get(int accept_fd, unsigned int length);
void Put(int accept_fd, unsigned int length);
void Quit(int accept_fd);
void MainLoop(int accept_fd);
void *Client(void *accept_fdp);

int main(int argc, char **argv){

	unsigned short port;
    bool match = false;
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
        /*match = Authentication(accept_fd);
        if (match)
            MainLoop(accept_fd);*/
    }
	
	close(fd);
	
	return 0;
}

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
	
	connection_count = 0;
	
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
		printf("Before accept\n");
        if( (accept_fd = accept(fd, (struct sockaddr *) &tmp_addr, &addrlen)) == -1)
		{
			perror("accept()");
			exit(1);
		}
		if (connection_count < MAX_N){
			pthread_create(&connections[connection_count], NULL, Client, (void *) &accept_fd);
			connection_count++;
		}
		
        return accept_fd;
	}
}

bool Authentication(int accept_fd){
    int count,i;
    int length;
    int TotalBytes;
    int ReadBytes = 0;
    struct message_s message;
    bool match = false;
    char *UserName;
    char *Password;
    char *Payload;

    Payload = malloc(sizeof(char)*32767);

    count = read(accept_fd, &message, sizeof(message));
    message.length = ntohl(message.length);

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    TotalBytes = message.length-count;

    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
        ReadBytes +=count;
        printf("ReadBytes:%d UserNamePassword:%s\n",ReadBytes, Payload);
    }

    UserName= strtok(Payload," ");
    Password = strtok(NULL," ");

    printf("User:%s\n", UserName);
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

    free(Payload);

    message.length = htonl(message.length);
	write(accept_fd, &message, sizeof(message));

	if (!match){
        close(accept_fd);
        return false;
	}
	else
        return true;


}

void LS(int accept_fd){
    struct message_s message;
    int count;
    int TotalBytes, ReadBytes, WroteBytes;
    DIR* dir;
    struct dirent* ptr;
    int i;
    char* Payload;

    Payload = malloc(sizeof(char)*32767);
    strcpy(Payload,"");

    ReadBytes = 0;
    WroteBytes = 0;

    if ((dir = opendir("./filedir")) == NULL)
    {
      printf("ERROR: Couldn't open directory.\n");
      exit(1);
    }

    while((ptr = readdir(dir)) != NULL)
    {
        if (strcmp(ptr->d_name,".") != 0 && strcmp(ptr->d_name,"..")){
            strcat(Payload,ptr->d_name);
            strcat(Payload,"\n");
        }
    }

    message.protocol[0] = 0xe3;
    message.protocol[1] = 'm';
    message.protocol[2] = 'y';
    message.protocol[3] = 'f';
    message.protocol[4] = 't';
    message.protocol[5] = 'p';
    message.type = 0xA6;
    message.length = 12 + strlen(Payload)+1;

    printf("%s %d\n", Payload, strlen(Payload));

    message.length = htonl(message.length);
    write(accept_fd, &message, sizeof(message));

    TotalBytes = strlen(Payload)+1;

    while (WroteBytes < TotalBytes){
        count = write(accept_fd, Payload+WroteBytes, TotalBytes-WroteBytes);
        WroteBytes += count;
    }

    closedir(dir);
    free(Payload);

}

void Get(int accept_fd, unsigned int length){
    struct message_s message;
    int count;
    int TotalBytes, ReadBytes, WroteBytes;
    int LastPosition;
    int FileSize;
    char* Payload;
    bool exist = false;
    DIR* dir;
    FILE* File;
    struct dirent* dptr;
    char* RealPath;
    char* DirPath;
    char* Filename;
    char* ptr;
    char pwd[1000];

    Payload = malloc(sizeof(char)*32767);
    DirPath = malloc(sizeof(char)*1000);
    RealPath = malloc(sizeof(char)*1000);
    strcpy(Payload,"");
    ReadBytes =0;
    WroteBytes=0;

    TotalBytes = length -12;

    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
        ReadBytes +=count;
    }

    printf("Filename:%s\n", Payload);

    realpath(Payload, RealPath);

    /*Find the last '/'*/
    ptr=strchr(RealPath,'/');

    while (ptr!=NULL)
    {
        //printf ("found at %d\n",ptr-RealPath+1);
        LastPosition = ptr-RealPath+1;
        ptr=strchr(ptr+1,'/');
    }

    *(RealPath+LastPosition-1) = 0x00;

    Filename = RealPath+LastPosition;

    strcpy(DirPath,RealPath);

    if ((dir = opendir("./filedir")) == NULL)
    {
      printf("ERROR: Couldn't open directory.\n");
      exit(1);
    }

    /*Search inside filedir*/

    while((dptr = readdir(dir)) != NULL)
    {
        if (strcmp(dptr->d_name,".") != 0 && strcmp(dptr->d_name,"..")){
            if (strcmp(dptr->d_name, Filename) == 0)
                exist = true;
        }
    }

    /*Check whether the path is pointing to pwd*/

    if (exist){
        getcwd(pwd,sizeof(pwd));
        if (strcmp(pwd,DirPath) != 0)
            exist = false;
    }

    message.protocol[0] = 0xe3;
    message.protocol[1] = 'm';
    message.protocol[2] = 'y';
    message.protocol[3] = 'f';
    message.protocol[4] = 't';
    message.protocol[5] = 'p';

    if (!exist){                        //File does not exist
        message.type = 0xA8;
        message.status = 0;
        message.length = 12;

        TotalBytes = message.length;
        WroteBytes = 0;

        message.length = htonl(message.length);

        write(accept_fd, &message, sizeof(message));
    }
    else{                               //File exist
        message.type = 0xA8;
        message.status = 1;
        message.length = 12;

        TotalBytes = message.length;
        WroteBytes = 0;

        message.length = htonl(message.length);

        write(accept_fd, &message, sizeof(message));

        strcpy(DirPath,"filedir/");
        strcat(DirPath,Filename);
        File = fopen(DirPath,"r");

        /*Check file size*/
        fseek(File,0,SEEK_END);
        FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        message.type = 0xFF;
        message.length = 12 + FileSize;

        TotalBytes = message.length;
        WroteBytes = 0;

        message.length = htonl(message.length);

        write(accept_fd, &message, sizeof(message));

        fread(Payload,1,FileSize,File);

        TotalBytes = FileSize;
        WroteBytes = 0;

        message.length = htonl(message.length);

        while (WroteBytes < TotalBytes){
            count = write(accept_fd, Payload+WroteBytes, TotalBytes-WroteBytes);
            WroteBytes += count;
        }

        fclose(File);

    }
}

void Put(int accept_fd, unsigned int length){
    struct message_s message;
    int count;
    int TotalBytes;
    int ReadBytes =0;
    int WroteBytes =0;
    int FileSize;
    char* Filename;
    char* Filename2;
    FILE* Output;
    char* Payload;

    Payload = malloc(sizeof(char)*32767);
    strcpy(Payload,"");

    Filename = malloc(sizeof(char)*1000);
    Filename2 = malloc(sizeof(char)*1000);

    TotalBytes = length -12;

    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
        ReadBytes +=count;
    }

    strcpy(Filename,Payload);

    printf("Filename:%s\n", Filename);

    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xAA;
	message.status = 0x00;
	message.length = 12;

    message.length = htonl(message.length);

    write(accept_fd, &message, sizeof(message));

    count = read(accept_fd, &message, sizeof(message));

    message.length = ntohl(message.length);

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    strcpy(Payload,"");

    FileSize = message.length-12;

    TotalBytes = message.length-12;
    ReadBytes = 0;

    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
        ReadBytes +=count;
    }

    strcpy(Filename2,"filedir/");
    strcat(Filename2,Filename);

    Output = fopen(Filename2, "w+");

    fwrite(Payload,1,FileSize,Output);

    printf("File saved.\n");

    fclose(Output);

}

void Quit(int accept_fd){
    struct message_s message;
    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = 0xAC;
	message.status = 0x00;
	message.length = 12;

    message.length = htonl(message.length);
	
    send(fd, &message, sizeof(message), MSG_NOSIGNAL);
	close(accept_fd);
}

void MainLoop(int accept_fd){
    struct message_s message;
    int count;
    int TotalBytes, ReadBytes, WroteBytes;
    int i;
    unsigned char Command;
    char* Payload;
    bool EndThread = false;

    Payload = malloc(sizeof(char)*32767);
    strcpy(Payload,"");

    while (1){
        strcpy(Payload,"");
        ReadBytes = 0;
        WroteBytes = 0;

        count = read(accept_fd, &message, sizeof(message));
        message.length = ntohl(message.length);

        printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

        Command = message.type;

        switch (Command){
            case 0xA5 :{
                LS(accept_fd);
            }
            break;
            case 0xA7 :{
                Get(accept_fd, message.length);
            }
            break;
            case 0xA9 :{
                Put(accept_fd, message.length);
            }
            break;
            case 0xAB :{
                Quit(accept_fd);
                EndThread = true;
				//pthread_exit();//Close the connection.
            }
            break;
        }

        if (EndThread)
			break;

    }

    free(Payload);
	
	pthread_exit();
    /*Maybe you should end the thread at here.*/

}

void *Client(void *accept_fdp){
	int count ;
    int accept_fd;
	bool match;
	
	struct message_s message;
	
	if (accept_fdp == NULL)
		exit(0);
		
	accept_fd = *((int *)accept_fdp);
	printf("Accept_fd = %d\n", accept_fd);
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
	
	match = Authentication(accept_fd);
    if (match)
		MainLoop(accept_fd);
		
	return;
}
