#ifndef UTILS_H_
#define UTILS_H_

#include <time.h> 


/* Requests from client */
typedef enum {
    HELP,
    SUB,
    CHANNELS,
    UNSUB,
    NEXT,
    NEXT_CHANNEL,
    LIVEFEED,
    LIVEFEED_CHANNEL,
    SEND,
    BYE,
    INVALID
} req_t;

typedef struct {
    req_t request_type;
    int channel_id;
    char message_text[1024];
} serv_req_t;


/* Responses from server */
typedef enum {
    PRINT,
    CLOSE
} resp_t;

typedef struct {
    resp_t type;
    int channel_id;
    char message_text[1024];
} serv_resp_t;



/* Channel Shared Memory */
typedef struct msg{
    int channel_id;
    int client_id;
    char message_text[1024];
} msg_t;

typedef struct channel{
    int channel_id;
    time_t lastEdited;
    long int numMsgs;
    msg_t messages[1000];
} channel_t;

typedef struct SharedMemoryType{
    channel_t channels[256];
} sharedMemory_t;






#endif /* UTILS_H_ */