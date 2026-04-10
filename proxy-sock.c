#define _GNU_SOURCE

/*
 * proxy-sock.c — Biblioteca del lado cliente para el servicio distribuido TCP.
 *
 * Implementa la misma API pública definida en claves.h, pero transporta cada
 * llamada al servidor por una conexión TCP independiente. La dirección y el
 * puerto del servidor se leen de las variables de entorno IP_TUPLAS y
 * PORT_TUPLAS.
 */

#include "claves.h"
#include "protocol_sock.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define RESPONSE_TIMEOUT_SEC 3
#define RESOLVE_POLL_NS 10000000L

static size_t bounded_len(const char *s, size_t max) {
    size_t i = 0;

    while (i < max && s[i] != '\0') {
        ++i;
    }

    return i;
}

static int valid_cstr_255(const char *s) {
    if (s == NULL) {
        return 0;
    }

    return bounded_len(s, KEY_MAX_LEN + 1) <= KEY_MAX_LEN;
}

static int load_server_config(const char **host_out, const char **port_out) {
    const char *host = getenv("IP_TUPLAS");
    const char *port = getenv("PORT_TUPLAS");
    char *endptr;
    long port_number;

    if (host == NULL || port == NULL || host[0] == '\0' || port[0] == '\0') {
        return -2;
    }

    errno = 0;
    port_number = strtol(port, &endptr, 10);
    if (errno != 0 || *endptr != '\0' || port_number < 1 || port_number > 65535) {
        return -2;
    }

    *host_out = host;
    *port_out = port;
    return 0;
}

static int resolve_server_address(const char *host, const char *port, struct addrinfo **results_out) {
    struct gaicb request;
    struct gaicb *requests[1];
    struct timespec start;
    struct timespec now;
    struct timespec sleep_time;
    int status;

    memset(&request, 0, sizeof(request));
    request.ar_name = host;
    request.ar_service = port;

    requests[0] = &request;
    if (getaddrinfo_a(GAI_NOWAIT, requests, 1, NULL) != 0) {
        return -1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        (void)gai_cancel(&request);
        return -1;
    }

    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = RESOLVE_POLL_NS;

    for (;;) {
        status = gai_error(&request);
        if (status == 0) {
            *results_out = request.ar_result;
            return 0;
        }
        if (status != EAI_INPROGRESS) {
            return -1;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            (void)gai_cancel(&request);
            return -1;
        }

        if ((now.tv_sec - start.tv_sec) >= RESPONSE_TIMEOUT_SEC) {
            (void)gai_cancel(&request);
            return -1;
        }

        (void)nanosleep(&sleep_time, NULL);
    }
}

static int set_socket_timeouts(int fd) {
    struct timeval timeout;

    timeout.tv_sec = RESPONSE_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
        return -1;
    }

    return 0;
}

static int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    fd_set write_set;
    fd_set error_set;
    struct timeval timeout;
    int flags;
    int result;
    int so_error = 0;
    socklen_t so_error_len = sizeof(so_error);

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }

    result = connect(fd, addr, addrlen);
    if (result == 0) {
        if (fcntl(fd, F_SETFL, flags) == -1) {
            return -1;
        }
        return set_socket_timeouts(fd);
    }
    if (errno != EINPROGRESS) {
        (void)fcntl(fd, F_SETFL, flags);
        return -1;
    }

    FD_ZERO(&write_set);
    FD_ZERO(&error_set);
    FD_SET(fd, &write_set);
    FD_SET(fd, &error_set);

    timeout.tv_sec = RESPONSE_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    result = select(fd + 1, NULL, &write_set, &error_set, &timeout);
    if (result <= 0) {
        (void)fcntl(fd, F_SETFL, flags);
        return -1;
    }

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len) != 0 || so_error != 0) {
        (void)fcntl(fd, F_SETFL, flags);
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags) == -1) {
        return -1;
    }

    return set_socket_timeouts(fd);
}

static int connect_to_server(void) {
    const char *host;
    const char *port;
    struct addrinfo *results = NULL;
    struct addrinfo *it;
    int fd = -1;

    if (load_server_config(&host, &port) != 0) {
        return -1;
    }

    if (resolve_server_address(host, port, &results) != 0) {
        return -1;
    }

    for (it = results; it != NULL; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd == -1) {
            continue;
        }

        if (connect_with_timeout(fd, it->ai_addr, it->ai_addrlen) == 0) {
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(results);
    return fd;
}

static int call_server(const protocol_request_t *req, protocol_response_t *res) {
    int fd = connect_to_server();

    if (fd == -1) {
        return -2;
    }

    if (protocol_send_request(fd, req) != 0) {
        close(fd);
        return -2;
    }

    if (protocol_recv_response(fd, res) != 0) {
        close(fd);
        return -2;
    }

    close(fd);
    return 0;
}

static void init_request(protocol_request_t *req, int op) {
    memset(req, 0, sizeof(*req));
    req->op = op;
}

int destroy(void) {
    protocol_request_t req;
    protocol_response_t res;

    init_request(&req, OP_DESTROY);
    if (call_server(&req, &res) != 0) {
        return -2;
    }

    return res.status;
}

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    protocol_request_t req;
    protocol_response_t res;

    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > VALUE2_MAX_ELEMS || V_value2 == NULL) {
        return -1;
    }

    init_request(&req, OP_SET);
    req.key_len = (uint32_t)strlen(key);
    req.value1_len = (uint32_t)strlen(value1);
    req.n_value2 = (uint32_t)N_value2;
    memcpy(req.key, key, req.key_len + 1);
    memcpy(req.value1, value1, req.value1_len + 1);
    memcpy(req.v_value2, V_value2, (size_t)N_value2 * sizeof(float));
    req.value3 = value3;

    if (call_server(&req, &res) != 0) {
        return -2;
    }

    return res.status;
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    protocol_request_t req;
    protocol_response_t res;

    if (!valid_cstr_255(key) || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        return -1;
    }

    init_request(&req, OP_GET);
    req.key_len = (uint32_t)strlen(key);
    memcpy(req.key, key, req.key_len + 1);

    if (call_server(&req, &res) != 0) {
        return -2;
    }

    if (res.status == 0) {
        strcpy(value1, res.value1);
        *N_value2 = (int)res.n_value2;
        memcpy(V_value2, res.v_value2, (size_t)res.n_value2 * sizeof(float));
        *value3 = res.value3;
    }

    return res.status;
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    protocol_request_t req;
    protocol_response_t res;

    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > VALUE2_MAX_ELEMS || V_value2 == NULL) {
        return -1;
    }

    init_request(&req, OP_MODIFY);
    req.key_len = (uint32_t)strlen(key);
    req.value1_len = (uint32_t)strlen(value1);
    req.n_value2 = (uint32_t)N_value2;
    memcpy(req.key, key, req.key_len + 1);
    memcpy(req.value1, value1, req.value1_len + 1);
    memcpy(req.v_value2, V_value2, (size_t)N_value2 * sizeof(float));
    req.value3 = value3;

    if (call_server(&req, &res) != 0) {
        return -2;
    }

    return res.status;
}

int delete_key(char *key) {
    protocol_request_t req;
    protocol_response_t res;

    if (!valid_cstr_255(key)) {
        return -1;
    }

    init_request(&req, OP_DELETE);
    req.key_len = (uint32_t)strlen(key);
    memcpy(req.key, key, req.key_len + 1);

    if (call_server(&req, &res) != 0) {
        return -2;
    }

    return res.status;
}

int exist(char *key) {
    protocol_request_t req;
    protocol_response_t res;

    if (!valid_cstr_255(key)) {
        return -1;
    }

    init_request(&req, OP_EXIST);
    req.key_len = (uint32_t)strlen(key);
    memcpy(req.key, key, req.key_len + 1);

    if (call_server(&req, &res) != 0) {
        return -2;
    }

    return res.status;
}