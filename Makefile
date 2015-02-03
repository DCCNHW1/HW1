CC = gcc
all: myftpclient myftpserver
myftpclient: myftpclient.c
		$(CC) -g -o $@ $< -lpthread
myftpserver: myftpserver.c
		$(CC) -g -o $@ $< -lpthread
clean:
	rm myftpclient myftpserver
