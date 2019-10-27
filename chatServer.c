/********************************* Includes **********************************/
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/shm.h>
#include <time.h> 
#include <semaphore.h>

// Definitions Header File
#include "defs.h"

/**************************** Global Constants *******************************/

#define BACKLOG 10 /* how many pending connections queue will hold */
int sockfd, client_fd;			   /* listen on sock_fd, new connection on client_fd */
int key = 1234;
sharedMemory_t * p_channelList;
int shm_id;
int subbed[NUM_CHANNELS] = {0}; //If the value at subbed[channel_id] == 1, channel is subbed.
int numRead[NUM_CHANNELS] = {0}; // Add 1 if message is opened 
int msgWhenSubbed[NUM_CHANNELS] = {0}; // Set to numRead when client subscribes
pthread_t livefeedThread;
int livefeedON = 0;


/***************************** Function Inits ********************************/
serv_req_t handle_user_reqt(int socket_fd);
void closeServer();
void printClientRequest(serv_req_t request);
void sendResponse(serv_resp_t response);
sharedMemory_t * init_Shared_Memory(int key);
sharedMemory_t * get_Shared_Memory(int key);
void storeMessage(serv_req_t request);
serv_resp_t handle_next_channel(serv_req_t request);
serv_resp_t handle_next(serv_req_t request);
void start_reader(int channel_id);
void rmv_reader(int channel_id);
serv_resp_t handle_next(serv_req_t request);


/********************************* Main Code *********************************/
int main(int argc, char *argv[])
{
	/* Set Up SIGINT (Interrupt Signal or CTRL-C) to execute clean close function */
    signal(SIGINT, closeServer);
    signal(SIGHUP, closeServer);
	
	/* Check Correct Usage */
	if (argc != 2)
	{
		fprintf(stderr, "usage: Server port_number\n");
		exit(1);
	}

	/* Get port number for server to listen on */
	int MYPORT = atoi(argv[1]);

	/* Set up networking */ 
	struct sockaddr_in my_addr;	/* my address information */
	struct sockaddr_in their_addr; /* client's address information */
	socklen_t sin_size;

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

	// Init shared memory
	p_channelList =  init_Shared_Memory(key);

	// Init Variables for critical section solution
	for(int i=0;i<NUM_CHANNELS;i++){   
		if (sem_init(&(p_channelList->channel_readers[i]), 1, 1) ==-1){
            perror("sem_init");
            exit(EXIT_FAILURE);
        }
		if(sem_init(&(p_channelList->channel_writer_locks[i]), 1, 1) ==-1 ){
            perror("sem_init");
            exit(EXIT_FAILURE);
        }
		p_channelList->readerCnt[i] = 0;
		p_channelList->channels[i].lastEdited = time(NULL);
    }

    printf("<< Started Execution of Chat Server >>\n\n");

	/* Main Loop: 	Wait for new connection, setup child process to run new connections
					Repeat! */
	while (1){
		/* Listen for new connection requests (THIS BLOCKS RUNNING FOR PARENT) */
		sin_size = sizeof(struct sockaddr_in);
		if ((client_fd = accept(sockfd, (struct sockaddr *)&their_addr,
							 &sin_size)) == -1)
		{
			perror("accept");
			continue;
		}

		/* Print new connection information */
		printf("## NEW CONNECTION\n##  Client_id: %d\n##  IP: %s\n", client_fd, inet_ntoa(their_addr.sin_addr));

		/* Create Child Process to handle new connection */
		if (!fork())
		{ /* NEW CLIENT HANDLING PROCESS */
			/* Initialise required vars and get shared memory */
			int CONNECTED = 1;
			p_channelList =  get_Shared_Memory(key);

			/* Client Handling Loop 
					Thread pool may be of use here, to take new requests and handle a queue of
					requests concurrently*/

			while(CONNECTED){
				serv_req_t request = handle_user_reqt(client_fd);
				if (request.request_type == BYE){
					CONNECTED = 0;
				}
			}

			printf("<< Connection From %d Closed >>\n", client_fd);
			close(client_fd);
			exit(0);
		}

		client_fd = 0;
	}
}


/****************************** Function Defs ********************************/

/*
Func:       Wait for request from client
			Take this request and perform the relevant action
Param:      
            int socket_fd:
                    The socket to listen on.
*/
serv_req_t handle_user_reqt(int socket_fd){
    serv_req_t request;
	
    if (recv(socket_fd, &request, sizeof(serv_req_t),PF_UNSPEC) == -1){
        perror("Receiving user coord request");
    }

	printClientRequest(request);
	printf("!!!!!\n");

	serv_resp_t response;
	
	switch (request.request_type)
	{
	case UNSUB:
        if ( request.channel_id < 0 || request.channel_id > 255 ){
            response.type = PRINT;
            snprintf(response.message_text, 1000, "Invalid channel: %d.", request.channel_id);
        }      
        else if ( subbed[request.channel_id] == 0 ){
            response.type = PRINT;
            snprintf(response.message_text, 1000, "Not subscribed to channel %d.", request.channel_id);
        }
        else{
            subbed[request.channel_id] = 0;
            response.type = PRINT;
            snprintf(response.message_text, 1000, "Unsubscribed from channel %d.", request.channel_id);
        }
		break;

	case BYE:
		response.type = PRINT;
		strcpy(response.message_text, "Server Connection Closed");
		break;

	// case LIVEFEED:
	// 	if(!livefeedON){
	// 		response.type = PRINT;
	// 		strcpy(response.message_text, "Live feed is already active.");
	// 		break;
	// 	}
	// 	start_livefeed_channel(request);
	
	case SEND:
		storeMessage(request);
		response.type = PRINT;
		strcpy(response.message_text, "Message Received");
		break;

	case CHANNELS:

		serv_resp_t response;
		for ( i = 0; i <= 256; i ++ )
		{
			if ( subbed[i] == 1 )
			{
				int numSent = p_channelList -> channel[i].numMsgs;
				int numNotRead = numSent - numRead[i];

				response.type = PRINT;
				snprintf( response.message_text, 1000,"Channel  %d: %d messages, Number of read messages: %d, Number of unread messages %d", i, numSent, numRead, numNotRead );
				sendResponse(response);
			}
		}
		
		response.type = END;
		sendResponse(response);
		break;
		
	case SUB:
        if ( request. channel_id < 0 || request.channel_id > 255 ){
            response.type = PRINT;
            snprintf(response.message_text, 1000, "Invalid channel: %d.", request.channel_id);
		}
        else if ( subbed[request.channel_id] == 1 ){
            response.type = PRINT;
            snprintf(response.message_text, 1000, "Already subscribed to channel  %d.", request.channel_id);
		}
        else {
            subbed[request.channel_id] = 1;
			msgWhenSubbed[request.channel_id] = p_channelList->channels[request.channel_id].numMsgs;
            response.type = PRINT;
            snprintf(response.message_text, 1000, "Subscribed to channel %d.", request.channel_id);
		}
		break;

	case NEXT_CHANNEL:
		start_reader(request.channel_id);
		response = handle_next_channel(request);
		rmv_reader(request.channel_id);
		break;

	case NEXT:
		response = handle_next(request);		
		break;

	default:
		response.type = PRINT;
		strcpy(response.message_text, "SERVER COULD NOT PASS COMMAND");
		printf("<< Invalid Request Received From %d >>\n", client_fd);
		break;
	}
	printf("##  FINISHED HANDLING\n");
	sendResponse(response);
	printf("##    Response sent to client %d\n",client_fd);

	printf("!!!!!\n");
    return request;
}

/*
Func:       Exit program, closing all connections,
*/
void closeServer(){
	switch (client_fd)
	{
	case 0:
		for(int i=0;i<NUM_CHANNELS;i++){   
		if (sem_destroy(&(p_channelList->channel_readers[i])) ==-1 ||
			sem_destroy(&(p_channelList->channel_writer_locks[i])) ==-1 ){
            perror("sem_destroy");
            exit(EXIT_FAILURE);
        }
		}
		printf("\n<< Closing Server-Parent >>\n");
		shmctl(shm_id, IPC_RMID, NULL);
		break;
	
	default:
		printf("\n<< Closed Connection With id: %d >>\n", client_fd);
		
		shutdown(client_fd, SHUT_RDWR);
		close(client_fd);
		break;
	}
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    exit(EXIT_SUCCESS);
}


/*
Func:       Print a standard representation of a request from the client
Param:      
            serv_req_t request:
                    A struct containing the data sent by the client.
*/
void printClientRequest(serv_req_t request){
	printf("\n## CLIENT REQUEST\n##  Client_id: %d\n", client_fd);
	switch (request.request_type)
	{
	case BYE:
		printf("##  Req_Type: BYE\n");
		printf("##    Action: Close Connection With This Client\n");
		break;

	case SEND:
		printf("##  Req_Type: SEND\n");
		printf("##    Channel_id: %d\n", request.channel_id);
		printf("##    Message: %s\n", request.message_text);
		printf("##    Action: Store Message Sent into channel\n");
		break;
	
	case SUB:
		printf("##  Req_Type: SUB\n");
		printf("##    Channel_id: %d\n", request.channel_id);
		printf("##    Action: Change Subbed[%d] to 1\n", request.channel_id);
		break;

	case CHANNELS:
		printf("##  Req_Type: CHANNELS\n");
		printf("##    Action: Show all channels user is subscribed too\n");
		break;
		
	case UNSUB:
		printf("##  Req_Type: UNSUB\n");
		printf("##    Channel_id: %d\n", request.channel_id);
		printf("##    Action: Change Subbed[%d] to 0\n", request.channel_id);
		break;
			
	case NEXT_CHANNEL:
		printf("##  Req_Type: NEXT_CHANNEL\n");
		printf("##    Channel_id: %d\n", request.channel_id);
		printf("##    Action: Send next unread message from channel %d\n", request.channel_id);
		break;
		
	case NEXT:
		printf("##  Req_Type: NEXT\n");
		printf("##    Action: Send next unread message on subbed channels\n");
		break;
		
	case LIVEFEED_CHANNEL:
		printf("##  Req_Type: LIVEFEED_CHANNEL\n");
		printf("##    Channel_id: %d\n", request.channel_id);
		printf("##    Action: Start sending all unread and new messages on channel %d\n", request.channel_id);
		break;
		
	case LIVEFEED:
		printf("##  Req_Type: LIVEFEED\n");
		printf("##    Action: Start sending all unread and new messages on subbed channels\n");
		break;
								
	default:
		printf("##  Req_Type: Unknown\n");
		printf("##    Channel_id: %d\n", request.channel_id);
		printf("##    Message: %s\n", request.message_text);
		printf("##    Action: No description written for this request type\n");
		break;
	}
	printf("\n");
}



/*
Func:       Send a response to the client (found at client_fd, which is globally available and unique
			to this child process).
Param:      
            serv_resp_t response:
                    The response struct to send (May be of type PRINT or CLOSE)
*/
void sendResponse(serv_resp_t response) {

    if (send(client_fd, &response, sizeof(serv_resp_t), PF_UNSPEC) == -1){
        perror("Sending request");
    }

}



/*
Func:       Initialises the Shared Memory (shm) segment (USED BY PARENT PROCESS)
			Shared memory is used to store the messages which will be viewed and editted
			by all child processes.
Param:      
            int key:
                    The identifier for the shm segment
Ret:		
			sharedMemory_t * p_ChannelList:
					A pointer to the shared memory object
*/
sharedMemory_t * init_Shared_Memory(int key){
	sharedMemory_t * p_ChannelList = malloc(sizeof(sharedMemory_t));

	for (int i=0; i<255; i++){
		p_ChannelList->channels[i].numMsgs = 0;
	}

	if ((shm_id = shmget(key,sizeof(sharedMemory_t), IPC_CREAT | 0666)) < 0)
    {
        perror("SHMGET");
        exit(1);
    }


    if((p_ChannelList = shmat(shm_id, 0, 0)) == (sharedMemory_t *) -1)
    {
        perror("SHMAT");
        exit(1);
	}

	return p_ChannelList;
}


/*
Func:       Gets and/or attaches the Shared Memory (shm) segment (USED BY CHILD PROCESSES)
			Shared memory is used to store the messages which will be viewed and editted
			by all child processes.
Param:      
            int key:
                    The identifier for the shm segment
*/
sharedMemory_t * get_Shared_Memory(int key){
	sharedMemory_t * p_ChannelList = malloc(sizeof(sharedMemory_t));

	shm_id = shmget(key,sizeof(sharedMemory_t), IPC_CREAT | 0644);
    p_ChannelList = shmat(shm_id, 0, 0);

	return p_ChannelList;
}


/*
Func:       Store messages sent into shared memory:
				This is the writer (readers-writers problem)
				Waits on the writer lock before writing
Param:      
            serv_req_t request:
                    The client request -- Including the channel and message text
*/
void storeMessage(serv_req_t request){
	msg_t newMsg;
	newMsg.channel_id = request.channel_id;
	newMsg.client_id = client_fd;
	strcpy(newMsg.message_text, request.message_text);

	sem_wait(&(p_channelList->channel_writer_locks[request.channel_id]));
	p_channelList->channels[request.channel_id].messages[p_channelList->channels[request.channel_id].numMsgs] = newMsg;
	p_channelList->channels[request.channel_id].numMsgs += 1;
	p_channelList->channels[request.channel_id].lastEdited = time(NULL);
	sem_post(&(p_channelList->channel_writer_locks[request.channel_id]));
	
}

/*
Func:       Handle the NEXT_CHANNEL request:
				Check if the channel is valid
				send the next unread message on the channel
Param:      
            serv_req_t request:
                    The client request
Ret:		
			serv_resp_t response:
					The response to be sent to the client.
*/
serv_resp_t handle_next_channel(serv_req_t request){
	serv_resp_t response;
	//If channel doesnt exist print message
	if ( request.channel_id < 0 || request.channel_id > NUM_CHANNELS )
	{
		response.type = PRINT;
		snprintf( response.message_text, 1000, "Invalid channel: %d.", request.channel_id );
	}

	//If client not subscribbed print message
	else if ( subbed[request.channel_id] == 0 )
	{
		response.type = PRINT;
		snprintf( response.message_text, 1000, "Not subscribed to channel %d.", request.channel_id );
	}

	//Else retrive first message available from time client joined and display 
	else 
	{
		int currentmessage = msgWhenSubbed[request.channel_id] + numRead[request.channel_id];
		channel_t tempchannel = p_channelList -> channels[request.channel_id];
		msg_t tempmessage = tempchannel.messages[currentmessage];

		response.type = PRINT;
		snprintf( response.message_text, 1024, "%s", tempmessage.message_text );

		numRead[request.channel_id]++;
	}
	return response;
}


/*
Func:       Handle the NEXT command:
				Find the most recently editted channel that is subscribed to
				Send the NEXT message on that channel to the client
Param:      
            serv_req_t request:
                    The client request
Ret:		
			serv_resp_t response:
					The response to be sent to the client.
*/
serv_resp_t handle_next(serv_req_t request){
	serv_resp_t response;
	response.type = PRINT;

	/* FIND THE LAST EDITED SUBBED CHANNEL */
	int numSubbed = 0;
	for (int i = 0; i < NUM_CHANNELS; i++) numSubbed += subbed[i];
	if (!numSubbed){
		strcpy(response.message_text, "No Channels Subbed");
		printf("##  CLIENT IS NOT SUBBED TO ANY CHANNELS\n");
		return response;
	}


	// printf("FINDING FIRST CHANNEL\n");
	request.channel_id = 500;
	for (int i=0; i<NUM_CHANNELS; i++){
		if (!subbed[i]) continue;
		if (p_channelList->channels[i].numMsgs <=(msgWhenSubbed[i] + numRead[i])) continue;
		if (subbed[i]){
			request.channel_id = i;
			break;
		}	
	}
	
	double changeInTime;
	for (int i = request.channel_id; i<NUM_CHANNELS; i++){
		if (!subbed[i]) continue;
		if (p_channelList->channels[i].numMsgs <=(msgWhenSubbed[i] + numRead[i])) continue;
		changeInTime = difftime(p_channelList->channels[i].lastEdited, p_channelList->channels[request.channel_id].lastEdited);
		if (changeInTime > 0)
			request.channel_id = i;
	}
	// printf("LAST EDITED WAS %d", request.channel_id);
	if(request.channel_id == 500){
		strcpy(response.message_text, "No New Messages to show");
		return response;
	}

	start_reader(request.channel_id);
	/* Get Message from channel */

	int currentmessage = msgWhenSubbed[request.channel_id] + numRead[request.channel_id];
	channel_t tempchannel = p_channelList -> channels[request.channel_id];
	msg_t tempmessage = tempchannel.messages[currentmessage];

	snprintf( response.message_text, 1024, "%d: %s", request.channel_id, tempmessage.message_text );

	numRead[request.channel_id]++;

	rmv_reader(request.channel_id);
	return response;
}

/*
Func:       Run code for reader to enter the critical section
				ensure that at no point is there a reader and a writer in the
				critical section at the same time
Param:      
            int kchannel_id:
					The channel no. in use.
					Identify which channel to lock.
*/
void start_reader(int channel_id){    
   // Reader wants to enter the critical section
   sem_wait(&(p_channelList->channel_readers[channel_id]));

   // Increase Reader count for channel_id
   (p_channelList->readerCnt[channel_id])++;

   // ensure that no writers can enter this critical section
   // when at least 1 reader is reading 
   if (p_channelList->readerCnt[channel_id]==1)     
      sem_wait(&(p_channelList->channel_writer_locks[channel_id]));                    

   // Allow multiple readers to enter the critical section
   sem_post(&(p_channelList->channel_readers[channel_id]));     
   return;     
}


/*
Func:       Run code for reader to exit the critical section
				release lock on writers if this is the last reader in the 
				section.
Param:      
            int channel_id:
                    the channel number of the channel in use
					used to identify which channel to unlock
*/
void rmv_reader(int channel_id){
	sem_wait(&(p_channelList->channel_readers[channel_id]));// a reader wants to leave

	p_channelList->readerCnt[channel_id]--;

	// that is, no reader is left in the critical section,
	if (p_channelList->readerCnt[channel_id] == 0)
		sem_post(&(p_channelList->channel_writer_locks[channel_id]));// writers can enter

	sem_post(&(p_channelList->channel_readers[channel_id])); // reader leaves
	return;
}