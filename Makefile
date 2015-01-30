CC = gcc
all: myftpclient myftpserver
myftpclient: myftpclient.c
		$(CC) -o $@ $< 
myftpserver: myftpserver.c
		$(CC) -o $@ $<
clean:
	rm myftpclient myftpserver
