CC = gcc
CFLAGS = -Wall -pthread

all: server client
	rm -f *.o
	@echo Compilation finished!

client: chatClient.o defs.h
	$(CC) $(CFLAGS) -o client chatClient.o defs.h


server: chatServer.o  defs.h
	$(CC) $(CFLAGS) -o server chatServer.o  defs.h