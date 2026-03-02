/*
 * servidor-mq.c — Servidor concurrente del servicio de tuplas.
 *
 * Modelo de concurrencia: hilos bajo demanda (on-demand threads).
 *   - El hilo principal lee peticiones de la cola del servidor.
 *   - Por cada petición se crea un hilo trabajador (pthread_create + pthread_detach)
 *     que procesa la operación y envía la respuesta al cliente.
 *
 * Justificación: los hilos bajo demanda ofrecen simplicidad de implementación
 * y buen rendimiento para cargas moderadas.  El acceso concurrente a los datos
 * está protegido por el mutex global de claves.c, lo que garantiza atomicidad.
 *
 * Señales: SIGINT y SIGTERM provocan una parada limpia del servidor.
 */

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

/* Flag atómico para parada controlada con señales. */
static volatile sig_atomic_t g_running = 1;

/* Argumento que se pasa a cada hilo trabajador (copia de la petición). */
typedef struct {
    request_msg_t req;
} worker_arg_t;

/* Handler de señales: desactiva el bucle principal. */
static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
}

/* Rellena una respuesta de error genérica. */
static void fill_response_error(response_msg_t *res, int status) {
    memset(res, 0, sizeof(*res));
    res->status = status;
}

/*
 * process_request: despacha la operación solicitada invocando la función
 * correspondiente de la API (libclaves.so).  El mutex de claves.c protege
 * el acceso a la tabla hash de forma atómica.
 */
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
            res->status = -1;   /* Operación desconocida. */
            break;
    }
}

/*
 * worker_main: función ejecutada por cada hilo trabajador.
 *   1. Procesa la petición (operación de la API).
 *   2. Abre la cola de respuesta del cliente y envía el resultado.
 *   3. Libera la memoria del argumento.
 */
static void *worker_main(void *arg) {
    worker_arg_t *w = (worker_arg_t *)arg;
    response_msg_t res;

    process_request(&w->req, &res);

    /* Enviar la respuesta a la cola temporal del cliente. */
    mqd_t client_q = mq_open(w->req.reply_queue, O_WRONLY);
    if (client_q != (mqd_t)-1) {
        (void)mq_send(client_q, (const char *)&res, sizeof(res), 0);
        (void)mq_close(client_q);
    }

    free(w);
    return NULL;
}

/* ---------- Punto de entrada del servidor ---------- */
int main(void) {
    /* Registro de señales para parada limpia (SIGINT / SIGTERM). */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGTERM, &sa, NULL);

    /* Eliminar la cola previa (si existe) y crearla de nuevo. */
    (void)mq_unlink(SERVER_QUEUE_NAME);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg  = 10;                    /* Máx. mensajes en cola.  */
    attr.mq_msgsize = sizeof(request_msg_t);  /* Tamaño de cada mensaje. */

    mqd_t server_q = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, 0666, &attr);
    if (server_q == (mqd_t)-1) {
        perror("mq_open servidor");
        return 1;
    }

    printf("Servidor MQ activo en %s\n", SERVER_QUEUE_NAME);

    /* Bucle principal: recibir peticiones y lanzar hilos trabajadores. */
    while (g_running) {
        request_msg_t req;
        struct timespec ts;

        /* Recepción con timeout de 1s para comprobar señales periódicamente. */
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

        /* Copiar la petición en memoria dinámica para el hilo. */
        worker_arg_t *w = (worker_arg_t *)malloc(sizeof(worker_arg_t));
        if (w == NULL) {
            /* Sin memoria: responder error al cliente directamente. */
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

        /* Crear hilo trabajador (detached: no hace falta join). */
        pthread_t tid;
        if (pthread_create(&tid, NULL, worker_main, w) != 0) {
            /* Fallo al crear hilo: responder error y liberar memoria. */
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

        (void)pthread_detach(tid);  /* El hilo se libera al terminar. */
    }

    /* Limpieza: cerrar y eliminar la cola del servidor. */
    (void)mq_close(server_q);
    (void)mq_unlink(SERVER_QUEUE_NAME);
    printf("Servidor MQ detenido\n");

    return 0;
}
