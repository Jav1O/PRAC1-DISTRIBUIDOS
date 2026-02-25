#include "claves.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    const char *prefix = (argc > 1) ? argv[1] : "cli";
    int iters = (argc > 2) ? atoi(argv[2]) : 100;

    if (iters <= 0) {
        iters = 100;
    }

    int pid = (int)getpid();

    for (int i = 0; i < iters; ++i) {
        char key[256];
        char value1[256];
        float v2[2] = {(float)i, (float)(i + 1)};
        struct Paquete p = {.x = i, .y = i + 10, .z = i + 20};

        snprintf(key, sizeof(key), "%s_%d_%d", prefix, pid, i);
        snprintf(value1, sizeof(value1), "valor_%d", i);

        if (set_value(key, value1, 2, v2, p) != 0) {
            printf("set_value fallo en iter=%d\n", i);
            return 1;
        }

        {
            char out_v1[256];
            int out_n = 0;
            float out_v2[32] = {0};
            struct Paquete out_p;

            if (get_value(key, out_v1, &out_n, out_v2, &out_p) != 0) {
                printf("get_value fallo en iter=%d\n", i);
                return 1;
            }
            if (strcmp(out_v1, value1) != 0 || out_n != 2) {
                printf("get_value inconsistente en iter=%d\n", i);
                return 1;
            }
        }

        if (delete_key(key) != 0) {
            printf("delete_key fallo en iter=%d\n", i);
            return 1;
        }
    }

    printf("OK %s pid=%d iters=%d\n", prefix, pid, iters);
    return 0;
}
