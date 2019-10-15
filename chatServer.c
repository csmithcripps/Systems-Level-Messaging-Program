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


/**************************** Global Constants *******************************/
#define ARRAY_SIZE 30 /* Size of array to receive */

#define BACKLOG 10 /* how many pending connections queue will hold */

#define RETURNED_ERROR -1
int sockfd, new_fd;			   /* listen on sock_fd, new connection on new_fd */

/***************************** Function Inits ********************************/
int *Receive_Array_Int_Data(int new_fd, int size);
void closeServer();



/********************************* Main Code *********************************/
int main(int argc, char *argv[])
{
    signal(SIGINT, closeServer);
    signal(SIGHUP, closeServer);
	
    
	struct sockaddr_in my_addr;	/* my address information */
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;
	int i = 0;

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

	printf("server starts listnening ...\n");

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
		printf("server: got connection from %s\n",
			   inet_ntoa(their_addr.sin_addr));
		if (!fork())
		{ /* this is the child process */
			int CONNECTED = 1;

			while(CONNECTED){
				/* Call method to recieve array data - Uncomment this line once function implemented */
				int *results = Receive_Array_Int_Data(new_fd, ARRAY_SIZE);

			}


			close(new_fd);
			exit(0);
		}
	}
}


/****************************** Function Defs ********************************/
int *Receive_Array_Int_Data(int socket_identifier, int size)
{
	int *arrayData = malloc(sizeof(int) * size);
	int spinVal;
	for (int i = 0; i < size; i++)
	{
		if (recv(socket_identifier, &spinVal, sizeof(int), 0) == -1)
		{
			// continue;
			perror("Receiving Data");
		}
		arrayData[i] = ntohl(spinVal);
	}
	return arrayData;
}


void closeServer(){
    printf("\nServer is shutting down now...\n");
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    exit(EXIT_SUCCESS);
}