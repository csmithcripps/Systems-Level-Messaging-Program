/********************************* Includes **********************************/
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/***************************** Function Inits ********************************/
void commandHandler();
void SUB(char* channelid);
void listChannels();
void UNSUB();
void NEXT(char* channelid);
void LIVEFEED(char* channelid);
void SEND(char* channelid, char* message);
void exitGracefully();



/********************************* Main Code *********************************/
int main(int argc, char* argv[]){

    signal(SIGINT, exitGracefully);
    signal(SIGHUP, exitGracefully);

    printf("<< Started Chat Client >>\n\n");

    while(1){
    }
}


/****************************** Function Defs ********************************/
void exitGracefully(){

    printf("\n\n<< What a Graceful Exit!! >>\n");

    // if (socket_fd != 0){
    //     coord_req_t req;
    //     req.request_type = quit;
    //     send_request(req);
    //     shutdown(socket_fd,SHUT_RDWR);
    //     close(socket_fd);
    // }

    exit(EXIT_SUCCESS);
}
