#ifndef PROTOCOL_SOCK_H
#define PROTOCOL_SOCK_H

#include "claves.h"

#include <stdint.h>

#define KEY_MAX_LEN 255
#define VALUE1_MAX_LEN 255
#define VALUE2_MAX_ELEMS 32

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
    uint32_t key_len;
    uint32_t value1_len;
    uint32_t n_value2;
    char key[KEY_MAX_LEN + 1];
    char value1[VALUE1_MAX_LEN + 1];
    float v_value2[VALUE2_MAX_ELEMS];
    struct Paquete value3;
} protocol_request_t;

typedef struct {
    int status;
    uint32_t value1_len;
    uint32_t n_value2;
    char value1[VALUE1_MAX_LEN + 1];
    float v_value2[VALUE2_MAX_ELEMS];
    struct Paquete value3;
} protocol_response_t;

const char *protocol_op_to_string(int op);

int protocol_send_request(int fd, const protocol_request_t *req);
int protocol_recv_request(int fd, protocol_request_t *req);
int protocol_send_response(int fd, const protocol_response_t *res);
int protocol_recv_response(int fd, protocol_response_t *res);

#endif