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
void handleResponse(serv_resp_t response);
void printHelp();


/********************************* Main Code *********************************/
int main(int argc, char* argv[]){

    /* Set up the program to exit gracefully on SIGINT (CTRL-C) */
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
    }

	return 0;
}


/****************************** Function Defs ********************************/



/*
Func:       using the console input connect with the server on the given socket
Param:      
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
Func:       exit program and close socket.
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
Func:       Create server requests (serv_req_t) from user inputs
Return:
            The request made by the user
*/
serv_req_t commandHandler(){
        serv_req_t request;

        char req[50];
        printf("\n --> ");
        scanf("%s", req);

        request.request_type = checkRequestType(req);

        switch (request.request_type)
        {
        case HELP:
            printHelp();
            break;

        case BYE:
            exitGracefully();
        
        case INVALID:
            printf("\n<< INVALID INPUT >>\n");
            break;
        
        default:
            sendRequest(request);
            break;
        }

        return request;
}

/*
Func:       Take user string and convert to request type code (req_t)
Param:
            req[]:
                    User input str
Return:
            request_Type:
                    (req_t enum) to denote the request code.
*/
req_t checkRequestType(char req[]){
    req_t request_Type;
    if (strcmp(req, "BYE")==0){  
        request_Type = BYE;
    }
    else if (strcmp(req, "HELP")==0){
        request_Type = HELP;
    }
    else{
        request_Type = INVALID;
    }
    return request_Type;
}


void printFromRecv(){

}

/*
Func:       Send request to the server and wait for responses
Param:      
            serv_req_t:
                    The server request to be sent.
*/
void sendRequest(serv_req_t request) {

    if (send(socket_fd, &request, sizeof(serv_req_t), PF_UNSPEC) == -1){
        perror("Sending request");
    }

    serv_resp_t response;
    if (recv(socket_fd, &response, sizeof(serv_resp_t), PF_UNSPEC) == -1){
        perror("Receiving response");
    }
    handleResponse(response);
}


/*
Func:       Take action on responses from the server.
Param:      
            serv_resp_t:
                    details on the server response.
*/
void handleResponse(serv_resp_t response){
	switch (response.type)
	{
	case CLOSE:
        printf("## SERVER --> CLOSE CONNECTION");
        exitGracefully();
		break;
	
	default:
        printf("\n%s\n", response.message_text);
		break;
	}
	printf("\n");
}

/*
Func:       Print information on available commands.
*/
void printHelp(){
    printf("## AVAILABLE COMMANDS\n");
    printf("##  BYE --> Close connection and exit.");
    
    printf("\n\n");
}