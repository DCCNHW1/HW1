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

#define OPEN_CONN_REQUEST 0xA1
#define OPEN_CONN_REPLY 0xA2
#define AUTH_REQUEST 0xA3
#define AUTH_REPLY 0xA4
#define LIST_REQUEST 0xA5
#define LIST_REPLY 0xA6
#define GET_REQUEST 0xA7
#define GET_REPLY 0xA8
#define FILE_DATA 0xFF
#define PUT_REQUEST 0xA9
#define PUT_REPLY 0xAA
#define QUIT_REQUEST 0xAB
#define QUIT_REPLY 0xAC

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
typedef struct ThreadData {
	char client_ip[INET_ADDRSTRLEN];
	unsigned short client_port;
	int accept_fd;
} client_data_s;

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
bool IsValid(struct message_s message);

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

	if (fp == NULL) {
		perror("Fail to open file");
		exit(1);
	}

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

	client_data_s client_data;
	struct message_s message;

	while (1){
		//printf("Before accept\n");
        if( (accept_fd = accept(fd, (struct sockaddr *) &tmp_addr, &addrlen)) == -1)
		{
			perror("accept()");
			exit(1);
		}
		if (connection_count < MAX_N){
			inet_ntop(AF_INET, &tmp_addr.sin_addr, client_data.client_ip, INET_ADDRSTRLEN); //Put necessary data into a struct 
			client_data.client_port = tmp_addr.sin_port;
			client_data.accept_fd = accept_fd;
			pthread_create(&connections[connection_count], NULL, Client, (void *) &client_data);
			connection_count++;
		} else {
			/*printf("Connection capacity full. Closing..\n");
			close(accept_fd);*/
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

    //Payload = malloc(sizeof(char)*INT_MAX);

    count = read(accept_fd, &message, sizeof(message));
	if (message.type != AUTH_REQUEST) {
		printf("Invalid Message: invalid protocol message type. Connection terminated.\n");
		close(accept_fd);
		connection_count--;
		pthread_exit();
	}
	
	if (count < 1) {
		printf("Error. Connection has been terminated by the other end. Closing.. \n");
		close(accept_fd);
		connection_count--;
		pthread_exit();
	}

	message.length = ntohl(message.length);

	if (!IsValid(message)) {
		close(accept_fd);
		connection_count--;
		pthread_exit();
	}

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);
    TotalBytes = message.length-count;

    Payload = malloc(sizeof(char)*TotalBytes);
	if (Payload == NULL){
		printf("Fatal Error: out of memory. \n");
		exit(1);
	}
	
    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
		if (count < 1) {
			printf("Error. Connection has been terminated by the other end. Closing.. \n");
			close(accept_fd);
			free(Payload);
			connection_count--;
			pthread_exit();
		}
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
	message.type = AUTH_REPLY;
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

    Payload = malloc(sizeof(char)*INT_MAX);
    //strcpy(Payload,"");
	if (Payload == NULL){
		printf("Fatal Error: out of memory. \n");
		exit(1);
	}
	
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
    message.type = LIST_REPLY;
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

  //  Payload = malloc(sizeof(char)*INT_MAX);
	ReadBytes =0;
    WroteBytes=0;

    TotalBytes = length -12;
	
    DirPath = malloc(sizeof(char)*1000);
    RealPath = malloc(sizeof(char)*1000);
    Payload = malloc(sizeof(char)*TotalBytes);
	
	if ((Payload == NULL) || (DirPath == NULL) || (RealPath == NULL)){
		printf("Fatal Error: out of memory. \n");
		exit(1);
	}
	
	
    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
		if (count < 1) {
			printf("Error. Connection has been terminated by the other end. Closing.. \n");
			close(accept_fd);
			free(Payload);
			connection_count--;
			pthread_exit();
		}
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
        message.type = GET_REPLY;
        message.status = 0;
        message.length = 12;

        TotalBytes = message.length;
        WroteBytes = 0;

        message.length = htonl(message.length);

        write(accept_fd, &message, sizeof(message));
    }
    else{                               //File exist
        message.type = GET_REPLY;
        message.status = 1;
        message.length = 12;

        TotalBytes = message.length;
        WroteBytes = 0;

        message.length = htonl(message.length);

        write(accept_fd, &message, sizeof(message));

        strcpy(DirPath,"filedir/");
        strcat(DirPath,Filename);
        File = fopen(DirPath,"r");
		if (File == NULL){
			perror("Opening file error.");
			exit(1);
		}
        /*Check file size*/
        fseek(File,0,SEEK_END);
        FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        message.type = FILE_DATA;
        message.length = 12 + FileSize;

        TotalBytes = 12;
        WroteBytes = 0;

        message.length = htonl(message.length);

        write(accept_fd, &message, sizeof(message));

        Payload = malloc(sizeof(char)*FileSize);
		if (NULL == Payload) {
			printf("Fatal Error: out of memory. Exiting..\n");
			exit(1);
		}
        fread(Payload,1,FileSize,File);

        TotalBytes = FileSize;
        WroteBytes = 0;

        message.length = htonl(message.length);

        while (WroteBytes < TotalBytes){
            count = write(accept_fd, Payload+WroteBytes, TotalBytes-WroteBytes);
            WroteBytes += count;
        }

        fclose(File);

        free(Payload);

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

    Filename = malloc(sizeof(char)*1000);
    Filename2 = malloc(sizeof(char)*1000);

    TotalBytes = length -12;

    Payload = malloc(sizeof(char)*TotalBytes);
	if ((NULL == Payload) || (NULL == Filename) || (NULL == Filename2)) {
		printf("Fatal Error: out of memory. Exiting..\n");
		exit(1);
	}
    //strcpy(Payload,"");

    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
		if (count < 1) {
			printf("Error. Connection has been terminated by the other end. Closing.. \n");
			close(accept_fd);
			free(Payload);
			connection_count--;
			pthread_exit();
		}
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
	message.type = PUT_REPLY;
	message.status = 0x00;
	message.length = 12;

    message.length = htonl(message.length);

    write(accept_fd, &message, sizeof(message));

    count = read(accept_fd, &message, sizeof(message));
	if (message.type != FILE_DATA) {
		printf("Invalid Message: invalid protocol message type. Connection terminated.\n");
		close(accept_fd);
		connection_count--;
		pthread_exit();
	}
	if (count < 1) {
			printf("Error. Connection has been terminated by the other end. Closing.. \n");
			close(accept_fd);
			free(Payload);
			connection_count--;
			pthread_exit();
	}

    message.length = ntohl(message.length);

	if (!IsValid(message)) {
		close(accept_fd);
		free(Payload);
		connection_count--;
		pthread_exit();
	}

    printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    free(Payload);

    FileSize = message.length-12;

    TotalBytes = message.length-12;
    ReadBytes = 0;

    Payload = malloc(sizeof(char)*TotalBytes);
	
	if (NULL == Payload) {
		printf("Fatal Error: out of memory. Exiting..\n");
		exit(1);
	}
	
    while (ReadBytes < TotalBytes){
        count = read(accept_fd, Payload+ReadBytes, TotalBytes-ReadBytes);
		if (count < 1) {
			printf("Error. Connection has been terminated by the other end. Closing.. \n");
			close(accept_fd);
			free(Payload);
			connection_count--;
			pthread_exit();
		}
        ReadBytes +=count;
    }

    strcpy(Filename2,"filedir/");
    strcat(Filename2,Filename);

    Output = fopen(Filename2, "w+");

    fwrite(Payload,1,FileSize,Output);

    printf("File saved.\n");

    fclose(Output);

    free(Payload);

}

void Quit(int accept_fd){
    struct message_s message;
    message.protocol[0] = 0xe3;
	message.protocol[1] = 'm';
	message.protocol[2] = 'y';
	message.protocol[3] = 'f';
	message.protocol[4] = 't';
	message.protocol[5] = 'p';
	message.type = QUIT_REPLY;
	message.status = 0x00;
	message.length = 12;

    message.length = htonl(message.length);

    send(accept_fd, &message, sizeof(message), 0);

	
	close(accept_fd);
}

void MainLoop(int accept_fd){
    struct message_s message;
    int count;
    int TotalBytes, ReadBytes, WroteBytes;
    int i;
    unsigned char Command;
    //char* Payload;
    bool EndThread = false;

    //Payload = malloc(sizeof(char)*12);
    //strcpy(Payload,"");

    while (1){
        //strcpy(Payload,"");
        ReadBytes = 0;
        WroteBytes = 0;

        count = read(accept_fd, &message, sizeof(message));
		if (count < 1) {
			printf("Error. Connection has been terminated by the other end. Closing.. \n");
			close(accept_fd);
			connection_count--;
			pthread_exit();
		}

		message.length = ntohl(message.length);

		if (!IsValid(message)) {
			close(accept_fd);
			connection_count--;
			pthread_exit();
		}

        printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

        Command = message.type;

        switch (Command){
            case LIST_REQUEST :{
                LS(accept_fd);
            }
            break;
            case GET_REQUEST :{
                Get(accept_fd, message.length);
            }
            break;
            case PUT_REQUEST :{
                Put(accept_fd, message.length);
            }
            break;
            case QUIT_REQUEST :{
                Quit(accept_fd);
                EndThread = true;
				//pthread_exit();//Close the connection.
            }
            break;
			default :{
				printf("Invalid Message: invalid protocol message type. Connection terminated.\n");
				close(accept_fd);
				connection_count--;
				pthread_exit();
			} 
        }

        if (EndThread)
			break;

    }

	//free(Payload);
	//pthread_exit();
    /*Maybe you should end the thread at here.*/

}

void *Client(void *client_data_sp){
	int count ;
    int accept_fd;
	bool match;
	char client_ip[INET_ADDRSTRLEN];
	unsigned short client_port;
	struct message_s message;

	accept_fd = ((client_data_s *) client_data_sp)->accept_fd; //extract data 
	strcpy(client_ip, ((client_data_s *) client_data_sp)->client_ip);
	client_port = ((client_data_s *) client_data_sp)->client_port;
	
	printf("New client: address %s, port %d\n", client_ip, client_port);
	printf("Clients count = %d\n", connection_count);
	printf("Accept_fd = %d\n", accept_fd);
	
	count = read(accept_fd, &message, sizeof(message));

	/*Check the message*/
	if (message.type != OPEN_CONN_REQUEST) {
		printf("Invalid Message: invalid protocol message type. Connection terminated.\n");
		close(accept_fd);
		connection_count--;
		pthread_exit();
	}
	
	if (count < 1) {
			printf("Error. Connection has been terminated by the other end. Closing.. \n");
			close(accept_fd);
			connection_count--;
			pthread_exit();
	}

	message.length = ntohl(message.length);

	if (!IsValid(message)) {
		close(accept_fd);
		connection_count--;
		pthread_exit();
	}


	printf("count:%d proto:%s type:%u status:%u length:%d\n",count,message.protocol, message.type, message.status, message.length);

    message.protocol[0] = 0xe3;
    message.protocol[1] = 'm';
    message.protocol[2] = 'y';
    message.protocol[3] = 'f';
    message.protocol[4] = 't';
    message.protocol[5] = 'p';
    message.type = OPEN_CONN_REPLY;
    message.status = 0x00;
    message.length = 12;
	message.length = htonl(message.length);

	write(accept_fd, &message, sizeof(message));

	match = Authentication(accept_fd);
    if (match)
		MainLoop(accept_fd);
		
	printf("Client disconnected: address %s, port %d\n", client_ip, client_port);
	connection_count--;
	pthread_exit();
	return;
}

bool IsValid(struct message_s message){
	unsigned char stdprot[6] = {0xe3, 'm', 'y', 'f', 't', 'p'};
	int i;
	for (i = 0; i < 6; i++)
		if (message.protocol[i] != stdprot[i]){ //Validate protocol header
			printf("Invalid Message: invalid protocol header. Connection terminated.\n");
			return false;
		}
	if (((message.type < OPEN_CONN_REQUEST) || (message.type > QUIT_REPLY)) //Validate message type
		&& (message.type != FILE_DATA)){
		printf("Invalid Message: invalid protocol message type. Connection terminated.\n");
		return false;
	}
	if (message.length < 12 || message.length > INT_MAX) { //Validate message length
		printf("Invalid Message: invalid message length field. Connection terminated\n");
		return false;
	}
	if ( ((message.type == OPEN_CONN_REQUEST) || (message.type == LIST_REQUEST) || (message.type == QUIT_REQUEST))
		&& (message.length != 12)){ //Validate message length according to type
		printf("Invalid Message: invalid message length field. Connection terminated\n");
		return false;
	}
	return true;
}

