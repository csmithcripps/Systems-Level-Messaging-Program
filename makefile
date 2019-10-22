CC = gcc
CFLAGS = -Wall -pthread -pedantic

all: server client
	rm -f *.o
	@echo Compilation finished!

client: chatClient.o defs.h
	$(CC) $(CFLAGS) -o client chatClient.o defs.h


server: chatServer.o  defs.h
	$(CC) $(CFLAGS) -o server chatServer.o  defs.h

clean:
	rm -f all *.o 

.PHONY: all clean 