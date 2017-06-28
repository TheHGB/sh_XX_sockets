CC = gcc
CFLAGS = -Wall -g

all: nameServerApp
	
nameServerApp: 
	$(CC) $(CFLAGS) nameServer.c common.c -o nameServer
	$(CC) $(CFLAGS) nameClient.c common.c -o nameClient

clean:
	rm *.o nameServer nameClient
	
