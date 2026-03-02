/*
 * claves.c — Implementación del servicio de tuplas <key, value1, value2, value3>.
 *
 * Estructura de datos elegida: tabla hash abierta (encadenamiento) con
 * BUCKET_COUNT cubetas.  No impone límite en el número de tuplas (se usa
 * memoria dinámica).  Todas las operaciones están protegidas por un mutex
 * global (g_mutex) para garantizar atomicidad en entornos concurrentes.
 */

#include "claves.h"

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR_LEN 255    /* Longitud máxima de key y value1 (sin '\0'). */
#define MAX_V2      32     /* Dimensión máxima del vector value2.         */
#define BUCKET_COUNT 1024  /* Número de cubetas de la tabla hash.         */

/* Nodo de la lista enlazada que almacena una tupla completa. */
typedef struct Entry {
    char key[MAX_STR_LEN + 1];
    char value1[MAX_STR_LEN + 1];
    int n_value2;
    float v_value2[MAX_V2];
    struct Paquete value3;
    struct Entry *next;   /* Siguiente nodo en la misma cubeta. */
} Entry;

/* Tabla hash global y mutex para acceso atómico. */
static Entry *g_table[BUCKET_COUNT];
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ---------- Funciones auxiliares internas ---------- */

/* Longitud acotada: recorre como máximo 'max' bytes. */
static size_t bounded_len(const char *s, size_t max) {
    size_t i = 0;
    while (i < max && s[i] != '\0') {
        ++i;
    }
    return i;
}

/* Devuelve 1 si 's' es una cadena válida de <= 255 caracteres. */
static int valid_cstr_255(const char *s) {
    if (s == NULL) {
        return 0;
    }
    return bounded_len(s, MAX_STR_LEN + 1) <= MAX_STR_LEN;
}

/* Función hash djb2 (Daniel J. Bernstein) para distribuir claves. */
static unsigned long hash_key(const char *s) {
    unsigned long h = 5381UL;
    unsigned char c;

    while ((c = (unsigned char)*s++) != '\0') {
        h = ((h << 5) + h) + c;   /* h * 33 + c */
    }

    return h;
}

/* Busca la entrada con la clave dada en la tabla.  Si prev_out no es NULL,
 * almacena el nodo anterior (necesario para eliminar de la lista enlazada). */
static Entry *find_entry(const char *key, Entry **prev_out) {
    unsigned long idx = hash_key(key) % BUCKET_COUNT;
    Entry *prev = NULL;
    Entry *cur = g_table[idx];

    while (cur != NULL) {
        if (strcmp(cur->key, key) == 0) {
            if (prev_out != NULL) {
                *prev_out = prev;
            }
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }

    if (prev_out != NULL) {
        *prev_out = NULL;
    }
    return NULL;
}

/* Libera toda la memoria de la tabla (usado por destroy). */
static void free_all_entries(void) {
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        Entry *cur = g_table[i];
        while (cur != NULL) {
            Entry *next = cur->next;
            free(cur);
            cur = next;
        }
        g_table[i] = NULL;
    }
}

/* ---------- Implementación de la API pública ---------- */

/* destroy: elimina todas las tuplas almacenadas. */
int destroy(void) {
    if (pthread_mutex_lock(&g_mutex) != 0) {
        return -1;
    }

    free_all_entries();

    if (pthread_mutex_unlock(&g_mutex) != 0) {
        return -1;
    }

    return 0;
}

/* set_value: inserta una nueva tupla; error si la clave ya existe o parámetros inválidos. */
int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    /* Validación de parámetros antes de adquirir el mutex. */
    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > MAX_V2 || V_value2 == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&g_mutex) != 0) {
        return -1;
    }

    if (find_entry(key, NULL) != NULL) {
        (void)pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    Entry *e = (Entry *)malloc(sizeof(Entry));
    if (e == NULL) {
        (void)pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    strcpy(e->key, key);
    strcpy(e->value1, value1);
    e->n_value2 = N_value2;
    memcpy(e->v_value2, V_value2, (size_t)N_value2 * sizeof(float));
    e->value3 = value3;

    unsigned long idx = hash_key(key) % BUCKET_COUNT;
    e->next = g_table[idx];
    g_table[idx] = e;

    if (pthread_mutex_unlock(&g_mutex) != 0) {
        return -1;
    }

    return 0;
}

/* get_value: obtiene los valores asociados a una clave existente. */
int get_value(char *key, char *value1, int *N_value2, float *V_value2, struct Paquete *value3) {
    if (!valid_cstr_255(key) || value1 == NULL || N_value2 == NULL || V_value2 == NULL || value3 == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&g_mutex) != 0) {
        return -1;
    }

    Entry *e = find_entry(key, NULL);
    if (e == NULL) {
        (void)pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    strcpy(value1, e->value1);
    *N_value2 = e->n_value2;
    memcpy(V_value2, e->v_value2, (size_t)e->n_value2 * sizeof(float));
    *value3 = e->value3;

    if (pthread_mutex_unlock(&g_mutex) != 0) {
        return -1;
    }

    return 0;
}

/* modify_value: modifica los valores de una tupla existente. */
int modify_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
    if (!valid_cstr_255(key) || !valid_cstr_255(value1) || N_value2 < 1 || N_value2 > MAX_V2 || V_value2 == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&g_mutex) != 0) {
        return -1;
    }

    Entry *e = find_entry(key, NULL);
    if (e == NULL) {
        (void)pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    strcpy(e->value1, value1);
    e->n_value2 = N_value2;
    memcpy(e->v_value2, V_value2, (size_t)N_value2 * sizeof(float));
    e->value3 = value3;

    if (pthread_mutex_unlock(&g_mutex) != 0) {
        return -1;
    }

    return 0;
}

/* delete_key: elimina una tupla por clave; error si no existe. */
int delete_key(char *key) {
    if (!valid_cstr_255(key)) {
        return -1;
    }

    if (pthread_mutex_lock(&g_mutex) != 0) {
        return -1;
    }

    Entry *prev = NULL;
    Entry *e = find_entry(key, &prev);
    if (e == NULL) {
        (void)pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    unsigned long idx = hash_key(key) % BUCKET_COUNT;
    if (prev == NULL) {
        g_table[idx] = e->next;
    } else {
        prev->next = e->next;
    }

    free(e);

    if (pthread_mutex_unlock(&g_mutex) != 0) {
        return -1;
    }

    return 0;
}

/* exist: devuelve 1 si la clave existe, 0 si no. */
int exist(char *key) {
    if (!valid_cstr_255(key)) {
        return -1;
    }

    if (pthread_mutex_lock(&g_mutex) != 0) {
        return -1;
    }

    Entry *e = find_entry(key, NULL);
    int ret = (e != NULL) ? 1 : 0;

    if (pthread_mutex_unlock(&g_mutex) != 0) {
        return -1;
    }

    return ret;
}
