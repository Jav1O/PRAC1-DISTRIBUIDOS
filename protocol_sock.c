#include "protocol_sock.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    uint32_t op;
    uint32_t key_len;
    uint32_t value1_len;
    uint32_t n_value2;
    int32_t x;
    int32_t y;
    int32_t z;
} wire_request_header_t;

typedef struct {
    int32_t status;
    uint32_t value1_len;
    uint32_t n_value2;
    int32_t x;
    int32_t y;
    int32_t z;
} wire_response_header_t;

static int send_all(int fd, const void *buffer, size_t length) {
    const unsigned char *ptr = (const unsigned char *)buffer;
    size_t sent = 0;
    int flags = 0;

#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif

    while (sent < length) {
        ssize_t n = send(fd, ptr + sent, length - sent, flags);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        sent += (size_t)n;
    }

    return 0;
}

static int recv_all(int fd, void *buffer, size_t length) {
    unsigned char *ptr = (unsigned char *)buffer;
    size_t received = 0;

    while (received < length) {
        ssize_t n = recv(fd, ptr + received, length - received, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        received += (size_t)n;
    }

    return 0;
}

static uint32_t float_to_network(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return htonl(bits);
}

static float network_to_float(uint32_t value) {
    uint32_t host_bits = ntohl(value);
    float result;
    memcpy(&result, &host_bits, sizeof(result));
    return result;
}

static int send_float_array(int fd, const float *values, uint32_t count) {
    uint32_t wire_value;

    for (uint32_t i = 0; i < count; ++i) {
        wire_value = float_to_network(values[i]);
        if (send_all(fd, &wire_value, sizeof(wire_value)) != 0) {
            return -1;
        }
    }

    return 0;
}

static int recv_float_array(int fd, float *values, uint32_t count) {
    uint32_t wire_value;

    for (uint32_t i = 0; i < count; ++i) {
        if (recv_all(fd, &wire_value, sizeof(wire_value)) != 0) {
            return -1;
        }
        values[i] = network_to_float(wire_value);
    }

    return 0;
}

const char *protocol_op_to_string(int op) {
    switch (op) {
        case OP_DESTROY:
            return "destroy";
        case OP_SET:
            return "set_value";
        case OP_GET:
            return "get_value";
        case OP_MODIFY:
            return "modify_value";
        case OP_DELETE:
            return "delete_key";
        case OP_EXIST:
            return "exist";
        default:
            return "op_desconocida";
    }
}

int protocol_send_request(int fd, const protocol_request_t *req) {
    wire_request_header_t header;

    header.op = htonl((uint32_t)req->op);
    header.key_len = htonl(req->key_len);
    header.value1_len = htonl(req->value1_len);
    header.n_value2 = htonl(req->n_value2);
    header.x = htonl((uint32_t)req->value3.x);
    header.y = htonl((uint32_t)req->value3.y);
    header.z = htonl((uint32_t)req->value3.z);

    if (send_all(fd, &header, sizeof(header)) != 0) {
        return -1;
    }
    if (req->key_len > 0 && send_all(fd, req->key, req->key_len) != 0) {
        return -1;
    }
    if (req->value1_len > 0 && send_all(fd, req->value1, req->value1_len) != 0) {
        return -1;
    }
    if (req->n_value2 > 0 && send_float_array(fd, req->v_value2, req->n_value2) != 0) {
        return -1;
    }

    return 0;
}

int protocol_recv_request(int fd, protocol_request_t *req) {
    wire_request_header_t header;

    memset(req, 0, sizeof(*req));

    if (recv_all(fd, &header, sizeof(header)) != 0) {
        return -1;
    }

    req->op = (int)ntohl(header.op);
    req->key_len = ntohl(header.key_len);
    req->value1_len = ntohl(header.value1_len);
    req->n_value2 = ntohl(header.n_value2);
    req->value3.x = (int32_t)ntohl((uint32_t)header.x);
    req->value3.y = (int32_t)ntohl((uint32_t)header.y);
    req->value3.z = (int32_t)ntohl((uint32_t)header.z);

    if (req->key_len > KEY_MAX_LEN || req->value1_len > VALUE1_MAX_LEN || req->n_value2 > VALUE2_MAX_ELEMS) {
        return -1;
    }

    if (req->key_len > 0) {
        if (recv_all(fd, req->key, req->key_len) != 0) {
            return -1;
        }
    }
    req->key[req->key_len] = '\0';

    if (req->value1_len > 0) {
        if (recv_all(fd, req->value1, req->value1_len) != 0) {
            return -1;
        }
    }
    req->value1[req->value1_len] = '\0';

    if (req->n_value2 > 0) {
        if (recv_float_array(fd, req->v_value2, req->n_value2) != 0) {
            return -1;
        }
    }

    return 0;
}

int protocol_send_response(int fd, const protocol_response_t *res) {
    wire_response_header_t header;

    header.status = htonl((uint32_t)res->status);
    header.value1_len = htonl(res->value1_len);
    header.n_value2 = htonl(res->n_value2);
    header.x = htonl((uint32_t)res->value3.x);
    header.y = htonl((uint32_t)res->value3.y);
    header.z = htonl((uint32_t)res->value3.z);

    if (send_all(fd, &header, sizeof(header)) != 0) {
        return -1;
    }
    if (res->value1_len > 0 && send_all(fd, res->value1, res->value1_len) != 0) {
        return -1;
    }
    if (res->n_value2 > 0 && send_float_array(fd, res->v_value2, res->n_value2) != 0) {
        return -1;
    }

    return 0;
}

int protocol_recv_response(int fd, protocol_response_t *res) {
    wire_response_header_t header;

    memset(res, 0, sizeof(*res));

    if (recv_all(fd, &header, sizeof(header)) != 0) {
        return -1;
    }

    res->status = (int32_t)ntohl((uint32_t)header.status);
    res->value1_len = ntohl(header.value1_len);
    res->n_value2 = ntohl(header.n_value2);
    res->value3.x = (int32_t)ntohl((uint32_t)header.x);
    res->value3.y = (int32_t)ntohl((uint32_t)header.y);
    res->value3.z = (int32_t)ntohl((uint32_t)header.z);

    if (res->value1_len > VALUE1_MAX_LEN || res->n_value2 > VALUE2_MAX_ELEMS) {
        return -1;
    }

    if (res->value1_len > 0) {
        if (recv_all(fd, res->value1, res->value1_len) != 0) {
            return -1;
        }
    }
    res->value1[res->value1_len] = '\0';

    if (res->n_value2 > 0) {
        if (recv_float_array(fd, res->v_value2, res->n_value2) != 0) {
            return -1;
        }
    }

    return 0;
}