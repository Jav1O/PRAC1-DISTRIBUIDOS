#include "claves.h"

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STR_LEN 255
#define MAX_V2 32
#define BUCKET_COUNT 1024

typedef struct Entry {
    char key[MAX_STR_LEN + 1];
    char value1[MAX_STR_LEN + 1];
    int n_value2;
    float v_value2[MAX_V2];
    struct Paquete value3;
    struct Entry *next;
} Entry;

static Entry *g_table[BUCKET_COUNT];
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    return bounded_len(s, MAX_STR_LEN + 1) <= MAX_STR_LEN;
}

static unsigned long hash_key(const char *s) {
    unsigned long h = 5381UL;
    unsigned char c;

    while ((c = (unsigned char)*s++) != '\0') {
        h = ((h << 5) + h) + c;
    }

    return h;
}

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

int set_value(char *key, char *value1, int N_value2, float *V_value2, struct Paquete value3) {
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
