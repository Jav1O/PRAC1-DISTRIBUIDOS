/*
 * servidor-sock.c — Servidor concurrente del servicio de tuplas sobre TCP.
 *
 * El proceso principal escucha en el puerto indicado por línea de comandos y
 * crea un hilo desacoplado por cada conexión entrante. Cada conexión contiene
 * exactamente una petición y una respuesta.
 */

#include "claves.h"
#include "protocol_sock.h"

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;
static int g_listen_fd = -1;

typedef struct {
    int client_fd;
} worker_arg_t;

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
    if (g_listen_fd != -1) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }
}

static void fill_error_response(protocol_response_t *res, int status) {
    memset(res, 0, sizeof(*res));
    res->status = status;
}

static void process_request(const protocol_request_t *req, protocol_response_t *res) {
    memset(res, 0, sizeof(*res));

    switch (req->op) {
        case OP_DESTROY:
            res->status = destroy();
            break;

        case OP_SET:
            res->status = set_value((char *)req->key,
                                    (char *)req->value1,
                                    (int)req->n_value2,
                                    (float *)req->v_value2,
                                    req->value3);
            break;

        case OP_GET:
            res->status = get_value((char *)req->key,
                                    res->value1,
                                    (int *)&res->n_value2,
                                    res->v_value2,
                                    &res->value3);
            if (res->status == 0) {
                res->value1_len = (uint32_t)strlen(res->value1);
            }
            break;

        case OP_MODIFY:
            res->status = modify_value((char *)req->key,
                                       (char *)req->value1,
                                       (int)req->n_value2,
                                       (float *)req->v_value2,
                                       req->value3);
            break;

        case OP_DELETE:
            res->status = delete_key((char *)req->key);
            break;

        case OP_EXIST:
            res->status = exist((char *)req->key);
            break;

        default:
            res->status = -1;
            break;
    }
}

static int setup_listener(const char *port) {
    struct addrinfo hints;
    struct addrinfo *results = NULL;
    struct addrinfo *it;
    int fd = -1;
    int reuse = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &results) != 0) {
        return -1;
    }

    for (it = results; it != NULL; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd == -1) {
            continue;
        }

        (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        if (bind(fd, it->ai_addr, it->ai_addrlen) == 0 && listen(fd, 16) == 0) {
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(results);
    return fd;
}

static void *worker_main(void *arg) {
    worker_arg_t *worker = (worker_arg_t *)arg;
    protocol_request_t req;
    protocol_response_t res;

    if (protocol_recv_request(worker->client_fd, &req) == 0) {
        printf("[srv] peticion=%s key=%s\n", protocol_op_to_string(req.op), req.key);
        fflush(stdout);

        process_request(&req, &res);
        if (protocol_send_response(worker->client_fd, &res) != 0) {
            /* Si el cliente ya cerró la conexión no hay recuperación útil. */
        }
    } else {
        fill_error_response(&res, -1);
        (void)protocol_send_response(worker->client_fd, &res);
    }

    close(worker->client_fd);
    free(worker);
    return NULL;
}

int main(int argc, char **argv) {
    struct sigaction sa;
    struct sigaction sa_pipe;
    char *endptr;
    long port_number;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <PUERTO>\n", argv[0]);
        return 1;
    }

    errno = 0;
    port_number = strtol(argv[1], &endptr, 10);
    if (errno != 0 || *endptr != '\0' || port_number < 1 || port_number > 65535) {
        fprintf(stderr, "Puerto invalido: %s\n", argv[1]);
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGTERM, &sa, NULL);

    memset(&sa_pipe, 0, sizeof(sa_pipe));
    sa_pipe.sa_handler = SIG_IGN;
    sigemptyset(&sa_pipe.sa_mask);
    (void)sigaction(SIGPIPE, &sa_pipe, NULL);

    g_listen_fd = setup_listener(argv[1]);
    if (g_listen_fd == -1) {
        perror("socket/bind/listen");
        return 1;
    }

    printf("Servidor TCP activo en puerto %ld\n", port_number);

    while (g_running) {
        int client_fd;
        worker_arg_t *worker;
        pthread_t tid;

        client_fd = accept(g_listen_fd, NULL, NULL);
        if (client_fd == -1) {
            if (!g_running) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        worker = (worker_arg_t *)malloc(sizeof(worker_arg_t));
        if (worker == NULL) {
            close(client_fd);
            continue;
        }

        worker->client_fd = client_fd;

        if (pthread_create(&tid, NULL, worker_main, worker) != 0) {
            close(client_fd);
            free(worker);
            continue;
        }

        (void)pthread_detach(tid);
    }

    if (g_listen_fd != -1) {
        close(g_listen_fd);
    }

    printf("Servidor TCP detenido\n");
    return 0;
}