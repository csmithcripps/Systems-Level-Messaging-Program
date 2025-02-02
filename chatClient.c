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
#include <ctype.h>


// Definitions Header File
#include "defs.h"

/**************************** Global Constants *******************************/
#define BUFFERSIZE 1024 /* max number of bytes we can get at once */

#define ARRAY_SIZE 30

#define PORT_NO 54321 /* PORT Number */

#define QUEUE_SIZE 10

/* User Connection Point */
int socket_fd = 0;

pthread_t p_thread;

pthread_mutex_t request_mutex;

queued_request_t * request_queue_head = NULL;

/***************************** Function Inits ********************************/
serv_req_t commandHandler();
void exitGracefully();
void sendRequest();
void connectWithServer(char* argv[]);
void printFromRecv();
req_t checkRequestType(char req[]);
void handleResponse(serv_resp_t response);
void printHelp();
serv_req_t handle_next();
void handle_request_loop(void * data);
void add_request(serv_req_t * request);
serv_req_t * get_req();
void rmv_req();


/********************************* Main Code *********************************/
int main(int argc, char* argv[]){
    int i;
    /* Set up the program to exit gracefully on SIGINT (CTRL-C) */
    signal(SIGINT, exitGracefully);
    signal(SIGHUP, exitGracefully);

	/* If Incorrect Number of input args exit*/
    if (argc != 3){
        printf("\nUsage --> %s [hostname] [port]\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int thread_ids[QUEUE_SIZE];
    /* Thread attributes */
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    int pthread_id = 1;
    /* Create threads */
    pthread_create(&p_thread, &attr, (void*) handle_request_loop, &pthread_id);
    

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
        serv_req_t * p_request = (serv_req_t *) malloc(sizeof(serv_req_t));
        char req[50];
        scanf("%s", req);

        request.request_type = checkRequestType(req);

        switch (request.request_type)
        {
        case HELP:
            printHelp();
            return request;
            break;

        case SUB:
            scanf("%d",&request.channel_id);
            break;

        case UNSUB:
            scanf("%d",&request.channel_id);
            break;
        
        case CHANNELS:
            break;

        case NEXT:
            request = handle_next();
            break;

        case SEND:
            scanf("%d",&request.channel_id);
            scanf("%[^\n]s",request.message_text);
            break;
        
        case BYE:
            exitGracefully();
            break;

        case INVALID:
            printf("\n<< INVALID INPUT >>\n");
            return request;
            break;
        
        default:
            break;
        }
        p_request->request_type = request.request_type;
        strcpy(p_request->message_text, request.message_text);
        p_request->channel_id = request.channel_id;

        add_request(p_request);

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
    else if (strcmp(req, "CHANNELS")==0){
        request_Type = CHANNELS;
    }
    else if (strcmp(req, "HELP")==0){
        request_Type = HELP;
    }
    else if (strcmp(req, "NEXT")==0){
        request_Type = NEXT;
    }
    else if (strcmp(req, "UNSUB")==0){
        request_Type = UNSUB;
    }
    else if (strcmp(req, "LIVEFEED")==0){
        request_Type = LIVEFEED;
    }
    else if (strcmp(req, "SEND")==0){
        request_Type = SEND;
    }
    else if (strcmp(req, "SUB")==0){
        request_Type = SUB;
    }
    else{
        request_Type = INVALID;
    }
    return request_Type;
}


void printFromRecv(){
    int PRINTING = 1;
    serv_resp_t response;
    while(PRINTING){
        if (recv(socket_fd, &response, sizeof(serv_resp_t), PF_UNSPEC) == -1){
            perror("Receiving response");
        }
        switch (response.type)
        {
        case END:
            PRINTING = 0;
            break;
        
        case CLOSE:
            printf("## SERVER --> CLOSE CONNECTION");
            exitGracefully();
            break;

        default:
            printf("\n%s\n", response.message_text);
            break;
        }
    }
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
    printf("\n## AVAILABLE COMMANDS");
    printf("\n##  SUB <channel_id>\n        --> Subscribe to channel[channel_id].");
    printf("\n##  CHANNELS\n        --> List stats on all subscribed channels.");
    printf("\n##  UNSUB <channel_id>\n        --> Unsubscribe to channel[channel_id].");
    printf("\n##  NEXT <channel_id>\n        --> Show next unread message on channel[channel_id]");
    printf("\n          --> If no channel_id is input, show next unread message from any subbed channel");
    printf("\n##  LIVEFEED <channel_id>\n        --> Display all unread messages on channel[channel_id], then display new messages as they come");
    printf("\n          --> If no channel_id is input, all subbed channels are displayed");
    printf("\n##  SEND <channel_id> <message>\n        --> SEND message to the server.");
    printf("\n##  BYE\n        --> Close connection and exit.");
    
    printf("\n\n");
}


serv_req_t handle_next(){

    int num = 1;
    int temp;
    serv_req_t request;
    if (!fgets(request.message_text, 1024, stdin))
    {
        // reading input failed:
        request.request_type = INVALID;
        return request;
    }
    // have some input, convert it to integer:
    char *endptr;
    errno = 0; // reset error number
    temp = strtol(request.message_text, &endptr, 10);
    if (errno == ERANGE)
    {
        num = 0;
    }
    else if (endptr == request.message_text)
    {
        // no character was read
        num = 0;
    }
    else if (*endptr && *endptr != '\n')
    {
        // *endptr is neither end of string nor newline,
        // so we didn't convert the *whole* input
        num = 0;
    }
    else
    {
        num = 1;
    }
    if(num == 1){
        request.request_type = NEXT_CHANNEL;
        request.channel_id = temp;
    }
    else{
        request.request_type = NEXT;
    }
    return request;
}


void handle_request_loop(void * data){
    
    int thread_id = *((int *)data);
    serv_req_t *request;


    while(1){
        if (request_queue_head != NULL){
            pthread_mutex_lock(&request_mutex);
            request = get_req();
            pthread_mutex_unlock(&request_mutex);

            if (request){
                handle_req(*request);
                rmv_req();
            }
        }
    }
}

void handle_req(serv_req_t request){
    switch (request.request_type)
    {
    case CHANNELS:
        sendRequest(request);
        printFromRecv();
        break;
    
    default:
        sendRequest(request);
        break;
    }
}




void add_request(serv_req_t * request){
    queued_request_t *new_node = (queued_request_t*)malloc(sizeof(queued_request_t));
    if(new_node == NULL)
    {
        return NULL;
    }

    pthread_mutex_lock(&request_mutex);
    new_node->request.channel_id = request->channel_id;
    new_node->request.request_type = request->request_type;
    strcpy(new_node->request.message_text, request->message_text);
    new_node->next= request_queue_head;

    request_queue_head = new_node;
    pthread_mutex_unlock(&request_mutex);
}


serv_req_t * get_req(){
    queued_request_t *cursor = request_queue_head;
    while(cursor!=NULL)
    {
        if(cursor->next == NULL)
            return &(cursor->request);
        cursor = cursor->next;
    }
    return NULL;
}

void rmv_req() {
    queued_request_t *previous = NULL;
    queued_request_t *current = request_queue_head;
    while (current != NULL) {
        if (current->next == NULL) {
            if (previous == NULL)  // first item in list
                request_queue_head = NULL;
            else 
                previous->next = current->next;
            
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
    
    // name not found
    return;
}
