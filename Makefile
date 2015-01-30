CC = gcc
all: myftpclient myftpserver
myftpclient: myftpclient.c
		$(CC) -g -o $@ $< 
myftpserver: myftpserver.c
		$(CC) -g -o $@ $<
clean:
	rm myftpclient myftpserver
