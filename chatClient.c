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


// Definitions Header File
#include "defs.h"

/**************************** Global Constants *******************************/
#define BUFFERSIZE 1024 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30

#define PORT_NO 54321 /* PORT Number */

/* User Connection Point */
int socket_fd = 0;

/***************************** Function Inits ********************************/
serv_req_t commandHandler();
void exitGracefully();
void sendRequest();
void connectWithServer(char* argv[]);
void printFromRecv();
req_t checkRequestType(char req[]);


/********************************* Main Code *********************************/
int main(int argc, char* argv[]){

    signal(SIGINT, exitGracefully);
    signal(SIGHUP, exitGracefully);

	/* If Incorrect Number of input args exit*/
    if (argc != 3){
        printf("\nUsage --> %s [hostname] [port]\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    connectWithServer(argv);

    printf("<< Connected Chat Client Successfully >>\n\n");

    while(1){
        serv_req_t request = commandHandler();
        switch (request.request_type)
        {
        case INVALID:
            printf("\n<< INVALID INPUT >>\n");
            break;

        case BYE:
            exitGracefully();
        
        default:
            sendRequest(request);
            break;
        }
    }

	return 0;
}


/****************************** Function Defs ********************************/



/*
func:       using the console input connect with the server on the given socket
param:      
            argv[]:
                    Input arguments from terminal
*/
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


/*
func:       exit program and close socket.
*/
void exitGracefully(){


    if (socket_fd != 0){
        serv_req_t req;
        req.request_type = BYE;
        sendRequest(req);
        shutdown(socket_fd,SHUT_RDWR);
        close(socket_fd);
    }

    printf("\n\n<< Client-Server Connection Successfully Closed >>\n");
    exit(EXIT_SUCCESS);
}




/*
Func:
        commandHandler()
Desc:
        Take Console input and send requests to server.
*/
serv_req_t commandHandler(){
        serv_req_t request;

        char req[50];
        printf("   --> ");
        scanf("%s", req);

        req_t reqType = checkRequestType(req);

        switch (reqType)
        {
        case BYE:
            request.request_type = BYE;
            break;
        
        default:
            request.request_type = INVALID;
            break;
        }


        return request;
}


req_t checkRequestType(char req[]){
    req_t request_Type;
    if (strcmp(req, "BYE")==0){  
        request_Type = BYE;
    }
    else{
        request_Type = INVALID;
    }
    return request_Type;
}


void printFromRecv(){

}

/*
Func: 
        sendRequest
Desc:
        Send request code to Server 

*/
void sendRequest(serv_req_t request) {

    if (send(socket_fd, &request, sizeof(serv_req_t), PF_UNSPEC) == -1){
        perror("Sending request");
    }

    //Needs to wait for response


}


