/********************************* Includes **********************************/
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/shm.h>


// Definitions Header File
#include "defs.h"


/**************************** Global Constants *******************************/
#define ARRAY_SIZE 30 /* Size of array to receive */

#define BACKLOG 10 /* how many pending connections queue will hold */

#define RETURNED_ERROR -1
int sockfd, new_fd;			   /* listen on sock_fd, new connection on new_fd */
int key = 5432;

/***************************** Function Inits ********************************/
serv_req_t handle_user_reqt(int socket_fd);
void closeServer();
void printClientRequest(serv_req_t request);
void sendResponse(serv_resp_t response);
sharedMemory_t * init_Shared_Memory(int key);
sharedMemory_t * get_Shared_Memory(int key);


/********************************* Main Code *********************************/
int main(int argc, char *argv[])
{
    signal(SIGINT, closeServer);
    signal(SIGHUP, closeServer);
	
    
	struct sockaddr_in my_addr;	/* my address information */
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;

	/* Get port number for server to listen on */

	if (argc != 2)
	{
		fprintf(stderr, "usage: Server port_number\n");
		exit(1);
	}
	int MYPORT = atoi(argv[1]);

	/* generate the socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	/* generate the end point */
	my_addr.sin_family = AF_INET;			  /* host byte order */
	my_addr.sin_port = htons(MYPORT);		  /* short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY;	 /* auto-fill with my IP */
	/* bzero(&(my_addr.sin_zero), 8);   ZJL*/ /* zero the rest of the struct */

	/* bind the socket to the end point */
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		exit(1);
	}

	/* start listnening */
	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	sharedMemory_t * p_channelList =  init_Shared_Memory(key);

	

    printf("<< Started Execution of Chat Server >>\n\n");

	/* repeat: accept, send, close the connection */
	/* for every accepted connection, use a sepetate process or thread to serve it */
	while (1)
	{ /* main accept() loop */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
							 &sin_size)) == -1)
		{
			perror("accept");
			continue;
		}
		printf("## NEW CONNECTION\n##  Client_id: %d\n##  IP: %s\n", new_fd, inet_ntoa(their_addr.sin_addr));
		if (!fork())
		{ /* this is the child process */
			int CONNECTED = 1;
			sharedMemory_t * p_channelList =  get_Shared_Memory(key);

			while(CONNECTED){
				/* Call method to recieve array data - Uncomment this line once function implemented */
				serv_req_t request = handle_user_reqt(new_fd);
				if (request.request_type == BYE){
					CONNECTED = 0;
				}
			}

			printf("<< Connection From %d Closed >>\n", new_fd);
			close(new_fd);
			exit(0);
		}

		new_fd = 0;
	}
}


/****************************** Function Defs ********************************/
serv_req_t handle_user_reqt(int socket_fd){
    serv_req_t request;

    if (recv(socket_fd, &request, sizeof(serv_req_t),PF_UNSPEC) == -1){
        perror("Receiving user coord request");
    }
	printClientRequest(request);

	serv_resp_t response;
	
	switch (request.request_type)
	{
	case BYE:
		response.type = PRINT;
		strcpy(response.message_text, "Server Connection Closed");
		break;
	
	default:
		printf("<< Invalid Request Received From %d >>\n", new_fd);
		break;
	}
	sendResponse(response);
    return request;
}


void closeServer(){
	switch (new_fd)
	{
	case 0:
		printf("\n<< Closing Server-Parent >>\n");
		break;
	
	default:
		printf("\n<< Closed Connection With id: %d >>\n", new_fd);
		serv_resp_t closeClient;
		closeClient.type = CLOSE;
		sendResponse(closeClient);
		break;
	}
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    exit(EXIT_SUCCESS);
}



void printClientRequest(serv_req_t request){
	printf("\n## CLIENT REQUEST\n##  Client_id: %d\n", new_fd);
	switch (request.request_type)
	{
	case BYE:
		printf("##  Req_Type: BYE\n");
		printf("##  Action: Close Connection With This Client\n");
		break;
	
	default:
		printf("##  Req_Type: Unknown\n");
		printf("##  Action: This Request Code is INVALID or Not Yet Fully Implemented\n");
		break;
	}
	printf("\n");
}


void sendResponse(serv_resp_t response) {

    if (send(new_fd, &response, sizeof(serv_resp_t), PF_UNSPEC) == -1){
        perror("Sending request");
    }

}




sharedMemory_t * init_Shared_Memory(int key){
	sharedMemory_t * p_ChannelList = malloc(sizeof(sharedMemory_t));
	int id;

	for (int i=0; i<255; i++){
		p_ChannelList->channels[i].numMsgs = 0;
	}

	if ((id = shmget(key,sizeof(sharedMemory_t), IPC_CREAT | 0666)) < 0)
    {
        perror("SHMGET");
        exit(1);
    }


    if((p_ChannelList = shmat(id, 0, 0)) == (sharedMemory_t *) -1)
    {
        perror("SHMAT");
        exit(1);
	}

	return p_ChannelList;
}


sharedMemory_t * get_Shared_Memory(int key){
	sharedMemory_t * p_ChannelList = malloc(sizeof(sharedMemory_t));
	int id;

	id = shmget(key,sizeof(sharedMemory_t), IPC_CREAT | 0644);
    p_ChannelList = shmat(id, 0, 0);

	return p_ChannelList;
}