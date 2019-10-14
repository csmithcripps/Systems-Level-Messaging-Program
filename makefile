CC = gcc
CFLAGS = -Wall -pthread

all: server client
	rm -f *.o
	@echo Compilation finished!

client: chatClient.o 
	$(CC) $(CFLAGS) -o client chatClient.o 


server: chatServer.o  
	$(CC) $(CFLAGS) -o server chatServer.o  
	
