#include "claves.h"
#include "protocol_mq.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

typedef struct {
    request_msg_t req;
} worker_arg_t;

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
}

static void fill_response_error(response_msg_t *res, int status) {
    memset(res, 0, sizeof(*res));
    res->status = status;
}

static void process_request(const request_msg_t *req, response_msg_t *res) {
    memset(res, 0, sizeof(*res));

    switch (req->op) {
        case OP_DESTROY:
            res->status = destroy();
            break;

        case OP_SET:
            res->status = set_value((char *)req->key,
                                    (char *)req->value1,
                                    req->n_value2,
                                    (float *)req->v_value2,
                                    req->value3);
            break;

        case OP_GET:
            res->status = get_value((char *)req->key,
                                    res->value1,
                                    &res->n_value2,
                                    res->v_value2,
                                    &res->value3);
            break;

        case OP_MODIFY:
            res->status = modify_value((char *)req->key,
                                       (char *)req->value1,
                                       req->n_value2,
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

static void *worker_main(void *arg) {
    worker_arg_t *w = (worker_arg_t *)arg;
    response_msg_t res;

    process_request(&w->req, &res);

    mqd_t client_q = mq_open(w->req.reply_queue, O_WRONLY);
    if (client_q != (mqd_t)-1) {
        (void)mq_send(client_q, (const char *)&res, sizeof(res), 0);
        (void)mq_close(client_q);
    }

    free(w);
    return NULL;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGTERM, &sa, NULL);

    (void)mq_unlink(SERVER_QUEUE_NAME);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(request_msg_t);

    mqd_t server_q = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &attr);
    if (server_q == (mqd_t)-1) {
        perror("mq_open servidor");
        return 1;
    }

    printf("Servidor MQ activo en %s\n", SERVER_QUEUE_NAME);

    while (g_running) {
        request_msg_t req;
        struct timespec ts;

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            continue;
        }
        ts.tv_sec += 1;

        ssize_t n = mq_timedreceive(server_q, (char *)&req, sizeof(req), NULL, &ts);
        if (n == -1) {
            if (errno == ETIMEDOUT || errno == EINTR) {
                continue;
            }
            perror("mq_receive servidor");
            continue;
        }

        if (n != (ssize_t)sizeof(req)) {
            continue;
        }

        worker_arg_t *w = (worker_arg_t *)malloc(sizeof(worker_arg_t));
        if (w == NULL) {
            response_msg_t res;
            fill_response_error(&res, -1);
            mqd_t client_q = mq_open(req.reply_queue, O_WRONLY);
            if (client_q != (mqd_t)-1) {
                (void)mq_send(client_q, (const char *)&res, sizeof(res), 0);
                (void)mq_close(client_q);
            }
            continue;
        }

        w->req = req;

        pthread_t tid;
        if (pthread_create(&tid, NULL, worker_main, w) != 0) {
            response_msg_t res;
            fill_response_error(&res, -1);
            mqd_t client_q = mq_open(req.reply_queue, O_WRONLY);
            if (client_q != (mqd_t)-1) {
                (void)mq_send(client_q, (const char *)&res, sizeof(res), 0);
                (void)mq_close(client_q);
            }
            free(w);
            continue;
        }

        (void)pthread_detach(tid);
    }

    (void)mq_close(server_q);
    (void)mq_unlink(SERVER_QUEUE_NAME);
    printf("Servidor MQ detenido\n");

    return 0;
}
