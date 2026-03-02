/*
 * protocol_mq.h — Protocolo de comunicación cliente-servidor por colas POSIX.
 *
 * Define las estructuras de mensaje (petición y respuesta) y las constantes
 * compartidas entre proxy-mq.c (lado cliente) y servidor-mq.c (lado servidor).
 *
 * Flujo del protocolo:
 *   1. El cliente crea una cola de respuesta exclusiva (nombre único por PID+timestamp).
 *   2. El cliente rellena un request_msg_t con la operación, los datos y el nombre
 *      de su cola de respuesta, y lo envía a la cola del servidor (SERVER_QUEUE_NAME).
 *   3. El servidor lee la petición, la procesa en un hilo dedicado, y escribe
 *      un response_msg_t en la cola de respuesta del cliente.
 *   4. El cliente lee la respuesta con timeout y elimina su cola temporal.
 */

#ifndef PROTOCOL_MQ_H
#define PROTOCOL_MQ_H

#include "claves.h"

/* Tamaños máximos de los campos (incluyen el '\0' terminador). */
#define KEY_MAX_LEN 256
#define VALUE1_MAX_LEN 256
#define VALUE2_MAX_ELEMS 32          /* Máximo número de floats en value2.     */
#define REPLY_QUEUE_MAX_LEN 64       /* Longitud máxima del nombre de cola.     */

/* Nombre de la cola del servidor (conocido por ambos lados). */
#define SERVER_QUEUE_NAME "/claves_srv"

/* Códigos de operación incluidos en el campo 'op' de la petición. */
typedef enum {
    OP_DESTROY = 1,
    OP_SET     = 2,
    OP_GET     = 3,
    OP_MODIFY  = 4,
    OP_DELETE  = 5,
    OP_EXIST   = 6
} op_code_t;

/*
 * Mensaje de petición (cliente -> servidor).
 * Contiene la operación solicitada, el nombre de la cola de respuesta
 * y todos los datos necesarios para cualquier operación de la API.
 */
typedef struct {
    int  op;                                /* Código de operación (op_code_t). */
    char reply_queue[REPLY_QUEUE_MAX_LEN];  /* Cola donde el cliente espera.    */

    char key[KEY_MAX_LEN];                  /* Clave de la tupla.               */
    char value1[VALUE1_MAX_LEN];            /* Valor cadena de caracteres.      */
    int  n_value2;                          /* Dimensión del vector value2.     */
    float v_value2[VALUE2_MAX_ELEMS];       /* Vector de floats.                */
    struct Paquete value3;                  /* Estructura Paquete (x, y, z).    */
} request_msg_t;

/*
 * Mensaje de respuesta (servidor -> cliente).
 * Contiene el código de retorno y, en caso de OP_GET, los datos leídos.
 */
typedef struct {
    int  status;                            /* Código de retorno (0, -1).       */

    char value1[VALUE1_MAX_LEN];            /* Datos devueltos por get_value.   */
    int  n_value2;
    float v_value2[VALUE2_MAX_ELEMS];
    struct Paquete value3;
} response_msg_t;

#endif
