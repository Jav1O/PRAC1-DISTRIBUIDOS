#ifndef PROTOCOL_MQ_H
#define PROTOCOL_MQ_H

#include "claves.h"

#define KEY_MAX_LEN 256
#define VALUE1_MAX_LEN 256
#define VALUE2_MAX_ELEMS 32
#define REPLY_QUEUE_MAX_LEN 64

#define SERVER_QUEUE_NAME "/claves_srv"

typedef enum {
    OP_DESTROY = 1,
    OP_SET = 2,
    OP_GET = 3,
    OP_MODIFY = 4,
    OP_DELETE = 5,
    OP_EXIST = 6
} op_code_t;

typedef struct {
    int op;
    char reply_queue[REPLY_QUEUE_MAX_LEN];

    char key[KEY_MAX_LEN];
    char value1[VALUE1_MAX_LEN];
    int n_value2;
    float v_value2[VALUE2_MAX_ELEMS];
    struct Paquete value3;
} request_msg_t;

typedef struct {
    int status;

    char value1[VALUE1_MAX_LEN];
    int n_value2;
    float v_value2[VALUE2_MAX_ELEMS];
    struct Paquete value3;
} response_msg_t;

#endif
