/*
 * proxy-mq.c — Biblioteca del lado cliente (libproxyclaves.so).
 *
 * Implementa la misma API que claves.c, pero en lugar de operar localmente
 * envía cada petición al servidor (servidor-mq) a través de colas POSIX.
 *
 * Flujo por cada llamada a la API:
 *   1. Validación local de parámetros (→ -1 sin contactar al servidor).
 *   2. Creación de una cola de respuesta temporal (nombre único por PID+ns+seq).
 *   3. Envío de request_msg_t a la cola del servidor.
 *   4. Espera de response_msg_t con timeout (RESPONSE_TIMEOUT_SEC).
 *   5. Limpieza de la cola temporal.
 *   Si cualquier paso de comunicación falla, se devuelve -2.
 */

#include "claves.h"
#include "protocol_mq.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LOCAL_MAX_STR_LEN 255   /* Límite local para validar sin ir al servidor. */
#define RESPONSE_TIMEOUT_SEC 3  /* Segundos máximos de espera de respuesta.       */

/* Contador atómico para generar nombres de cola únicos dentro del proceso. */
static atomic_uint g_req_seq = 0;

/* ---------- Funciones auxiliares ---------- */

/* Longitud acotada: evita recorrer más allá de 'max' bytes. */
static size_t bounded_len(const char *s, size_t max) {
    size_t i = 0;
    while (i < max && s[i] != '\0') {
        ++i;
    }
    return i;
}

/* Valida que la cadena no sea NULL y tenga ≤ 255 caracteres. */
static int valid_cstr_255(const char *s) {
    if (s == NULL) {
        return 0;
    }
    return bounded_len(s, LOCAL_MAX_STR_LEN + 1) <= LOCAL_MAX_STR_LEN;
}

/*
 * call_server: envía una petición al servidor y espera la respuesta.
 * Crea una cola temporal exclusiva para recibir la respuesta.
 * Devuelve 0 si la comunicación fue exitosa, -2 en caso de error.
 */
static int call_server(const request_msg_t *req, response_msg_t *res) {
    mqd_t server_q = (mqd_t)-1;
    mqd_t reply_q  = (mqd_t)-1;

    struct mq_attr reply_attr;
    memset(&reply_attr, 0, sizeof(reply_attr));
    reply_attr.mq_maxmsg = 10;
    reply_attr.mq_msgsize = sizeof(response_msg_t);

    /* Crear la cola de respuesta exclusiva para este cliente/petición. */
    reply_q = mq_open(req->reply_queue, O_RDONLY | O_CREAT | O_EXCL, 0600, &reply_attr);
    if (reply_q == (mqd_t)-1) {
        return -2;
    }

    /* Abrir la cola del servidor para enviarle la petición. */
    server_q = mq_open(SERVER_QUEUE_NAME, O_WRONLY);
    if (server_q == (mqd_t)-1) {
        mq_close(reply_q);
        mq_unlink(req->reply_queue);
        return -2;
    }

    /* Enviar la petición al servidor. */
    if (mq_send(server_q, (const char *)req, sizeof(*req), 0) == -1) {
        mq_close(server_q);
        mq_close(reply_q);
        mq_unlink(req->reply_queue);
        return -2;
    }

    /* Esperar la respuesta con timeout para no bloquear indefinidamente. */
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

/*
 * fill_base_request: inicializa los campos comunes de la petición,
 * incluyendo un nombre de cola de respuesta único basado en PID,
 * nanosegundos actuales y un contador atómico secuencial.
 */
static void fill_base_request(request_msg_t *req, int op) {
    struct timespec ts;
    unsigned int seq;

    memset(req, 0, sizeof(*req));
    req->op = op;

    seq = atomic_fetch_add_explicit(&g_req_seq, 1U, memory_order_relaxed);
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        ts.tv_nsec = 0;
    }

    (void)snprintf(req->reply_queue,
                   sizeof(req->reply_queue),
                   "/clv_%ld_%ld_%u",
                   (long)getpid(),
                   (long)ts.tv_nsec,
                   seq);
}

/* ---------- Implementación de la API pública (lado proxy) ---------- */

int destroy(void) {
    request_msg_t req;
    response_msg_t res;

    fill_base_request(&req, OP_DESTROY);
    if (call_server(&req, &res) == -2) {
        return -2;
    }

    return res.status;
}

/* set_value: valida parámetros localmente (-1) y luego envía al servidor. */
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validación local: se devuelve -1 sin contactar al servidor. */
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

/* get_value: solo envía la clave; recibe los datos en la respuesta. */
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

    /* Copiar los datos de la respuesta solo si la operación fue exitosa. */
    if (res.status == 0) {
        strcpy(value1, res.value1);
        *N_value2 = res.n_value2;
        memcpy(V_value2, res.v_value2, (size_t)res.n_value2 * sizeof(float));
        *value3 = res.value3;
    }

    return res.status;
}

/* modify_value: valida parámetros localmente y envía la modificación. */
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validación local idéntica a set_value. */
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

/* delete_key: valida la clave y solicita el borrado al servidor. */
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

/* exist: comprueba existencia de la clave en el servidor. */
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
