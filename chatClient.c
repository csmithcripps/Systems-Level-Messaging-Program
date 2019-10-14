/********************************* Includes **********************************/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

/**************************** Global Constants *******************************/
#define BUFFERSIZE 1024 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30

#define PORT_NO 54321 /* PORT Number */

/* User Connection Point */
int socket_fd = 0;

/***************************** Function Inits ********************************/
void commandHandler();
void exitGracefully();
void sendRequest();
void connectWithServer(char* argv[]);



/********************************* Main Code *********************************/
int main(int argc, char* argv[]){

    signal(SIGINT, exitGracefully);
    signal(SIGHUP, exitGracefully);

	/* If Incorrect Number of input args */
    if (argc != 3){
        printf("\nUsage --> %s [hostname] [port]\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    connectWithServer(argv);

    printf("<< Connected Chat Client Successfully >>\n\n");

    while(1){
        // commandHandler();
    }

	return 0;
}


/****************************** Function Defs ********************************/

void connectWithServer(char* argv[]){
	struct hostent *host;
	struct sockaddr_in serverAddr; /* connector's address information */

	int PORT = atoi(argv[2]);

	if ((host=gethostbyname(argv[1])) == NULL) {  /* get the host info */
		herror("gethostbyname");
		exit(1);
	}

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}


	serverAddr.sin_family = AF_INET;      /* host byte order */
	serverAddr.sin_port = htons(PORT);    /* short, network byte order */
	serverAddr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(serverAddr.sin_zero), 8);     /* zero the rest of the struct */

    if (connect(socket_fd, (struct sockaddr *) &serverAddr, sizeof(struct sockaddr_in)) == -1){
        printf("<< Server Not Responding. Please check connection details. >>\n");
        exit(EXIT_FAILURE);
    }

}

void exitGracefully(){


    if (socket_fd != 0){
        // coord_req_t req;
        // req.request_type = quit;
        // sendRequest(req);
        shutdown(socket_fd,SHUT_RDWR);
        close(socket_fd);
    }

    printf("\n\n<< Client-Server Connection Successfully Closed >>\n");
    exit(EXIT_SUCCESS);
}

void commandHandler(){
}



	

/*
Function: 
    sendRequest
Description:
    Send request code to Server 

*/
// void sendRequest(int socket_id, int requestType) {
// 	int i;
// 	uint32_t sendVal;
// 	for (i = 0; i < ARRAY_SIZE; i++) {
// 		sendVal = htonl(myArray[i]);
// 		send(socket_id, &sendVal, sizeof(uint32_t), 0);
// 		printf("Array[%d] = %d\n", i, myArray[i]);
// 	}
// 	fflush(stdout);
// }


