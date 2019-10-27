#ifndef UTILS_H_
#define UTILS_H_

#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_CHANNELS 256



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
    END,
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
    channel_t channels[NUM_CHANNELS];
    sem_t channel_readers[NUM_CHANNELS];
    sem_t channel_writer_locks[NUM_CHANNELS];
    int readerCnt[NUM_CHANNELS]; 
} sharedMemory_t;



/* Requests Linked List Struct
/* Struct of a single scoreboard entry */
typedef struct queued_request queued_request_t;
struct queued_request{
    serv_req_t request;
    queued_request_t * next;
};




#endif /* UTILS_H_ */