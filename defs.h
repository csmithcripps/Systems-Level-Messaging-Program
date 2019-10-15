#ifndef UTILS_H_
#define UTILS_H_


typedef enum {
    SUB,
    CHANNELS,
    UNSUB,
    NEXT,
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

#endif /* UTILS_H_ */