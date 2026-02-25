#include "claves.h"
#include "protocol_mq.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LOCAL_MAX_STR_LEN 255
#define RESPONSE_TIMEOUT_SEC 3

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
    return bounded_len(s, LOCAL_MAX_STR_LEN + 1) <= LOCAL_MAX_STR_LEN;
}

static int call_server(const request_msg_t *req, response_msg_t *res) {
    mqd_t server_q = (mqd_t)-1;
    mqd_t reply_q = (mqd_t)-1;

    struct mq_attr reply_attr;
    memset(&reply_attr, 0, sizeof(reply_attr));
    reply_attr.mq_maxmsg = 10;
    reply_attr.mq_msgsize = sizeof(response_msg_t);

    reply_q = mq_open(req->reply_queue, O_RDONLY | O_CREAT | O_EXCL, 0600, &reply_attr);
    if (reply_q == (mqd_t)-1) {
        return -2;
    }

    server_q = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (server_q == (mqd_t)-1) {
        mq_close(reply_q);
        mq_unlink(req->reply_queue);
        return -2;
    }

    if (mq_send(server_q, (const char *)req, sizeof(*req), 0) == -1) {
        mq_close(server_q);
        mq_close(reply_q);
        mq_unlink(req->reply_queue);
        return -2;
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        mq_close(server_q);
        mq_close(reply_q);
        mq_unlink(req->reply_queue);
        return -2;
    }
    ts.tv_sec += RESPONSE_TIMEOUT_SEC;

    ssize_t n = mq_timedreceive(reply_q, (char *)res, sizeof(*res), NULL, &ts);
    if (n != (ssize_t)sizeof(*res)) {
        mq_close(server_q);
        mq_close(reply_q);
        mq_unlink(req->reply_queue);
        return -2;
    }

    mq_close(server_q);
    mq_close(reply_q);
    mq_unlink(req->reply_queue);
    return 0;
}

static void fill_base_request(request_msg_t *req, int op) {
    memset(req, 0, sizeof(*req));
    req->op = op;
    snprintf(req->reply_queue, sizeof(req->reply_queue), "/claves_cli_%ld", (long)getpid());
}

int destroy(void) {
    request_msg_t req;
    response_msg_t res;

    fill_base_request(&req, OP_DESTROY);
    if (call_server(&req, &res) == -2) {
        return -2;
    }

    return res.status;
}

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > VALUE2_MAX_ELEMS || V_value2 == NULL) {
        return -1;
    }

    request_msg_t req;
    response_msg_t res;

    fill_base_request(&req, OP_SET);
    strcpy(req.key, key);
    strcpy(req.value1, value1);
    req.n_value2 = N_value2;
    memcpy(req.v_value2, V_value2, (size_t)N_value2 * sizeof(float));
    req.value3 = value3;

    if (call_server(&req, &res) == -2) {
        return -2;
    }

    return res.status;
}

int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    if (!valid_cstr_255(key) || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        return -1;
    }

    request_msg_t req;
    response_msg_t res;

    fill_base_request(&req, OP_GET);
    strcpy(req.key, key);

    if (call_server(&req, &res) == -2) {
        return -2;
    }

    if (res.status == 0) {
        strcpy(value1, res.value1);
        *N_value2 = res.n_value2;
        memcpy(V_value2, res.v_value2, (size_t)res.n_value2 * sizeof(float));
        *value3 = res.value3;
    }

    return res.status;
}

int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > VALUE2_MAX_ELEMS || V_value2 == NULL) {
        return -1;
    }

    request_msg_t req;
    response_msg_t res;

    fill_base_request(&req, OP_MODIFY);
    strcpy(req.key, key);
    strcpy(req.value1, value1);
    req.n_value2 = N_value2;
    memcpy(req.v_value2, V_value2, (size_t)N_value2 * sizeof(float));
    req.value3 = value3;

    if (call_server(&req, &res) == -2) {
        return -2;
    }

    return res.status;
}

int delete_key(char *key) {
    if (!valid_cstr_255(key)) {
        return -1;
    }

    request_msg_t req;
    response_msg_t res;

    fill_base_request(&req, OP_DELETE);
    strcpy(req.key, key);

    if (call_server(&req, &res) == -2) {
        return -2;
    }

    return res.status;
}

int exist(char *key) {
    if (!valid_cstr_255(key)) {
        return -1;
    }

    request_msg_t req;
    response_msg_t res;

    fill_base_request(&req, OP_EXIST);
    strcpy(req.key, key);

    if (call_server(&req, &res) == -2) {
        return -2;
    }

    return res.status;
}
