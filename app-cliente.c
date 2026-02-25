#include "claves.h"

#include <stdio.h>

int main(void) {
    const char *key = "clave_demo";
    char value1_out[256];
    int n_out = 0;
    float v2_out[32] = {0.0f};
    struct Paquete p_out;

    struct Paquete p = {.x = 10, .y = 20, .z = 30};
    float v2[] = {1.1f, 2.2f, 3.3f};

    int r = destroy();
    printf("destroy -> %d\n", r);

    r = set_value((char *)key, "valor inicial", 3, v2, p);
    printf("set_value -> %d\n", r);

    r = exist((char *)key);
    printf("exist (debe ser 1) -> %d\n", r);

    r = get_value((char *)key, value1_out, &n_out, v2_out, &p_out);
    printf("get_value -> %d | value1=%s | N=%d | p=(%d,%d,%d)\n",
           r,
           value1_out,
           n_out,
           p_out.x,
           p_out.y,
           p_out.z);

    struct Paquete p2 = {.x = 7, .y = 8, .z = 9};
    float v2b[] = {9.9f, 8.8f};
    r = modify_value((char *)key, "valor modificado", 2, v2b, p2);
    printf("modify_value -> %d\n", r);

    r = get_value((char *)key, value1_out, &n_out, v2_out, &p_out);
    printf("get_value (tras modify) -> %d | value1=%s | N=%d | p=(%d,%d,%d)\n",
           r,
           value1_out,
           n_out,
           p_out.x,
           p_out.y,
           p_out.z);

    r = delete_key((char *)key);
    printf("delete_key -> %d\n", r);

    r = exist((char *)key);
    printf("exist (debe ser 0) -> %d\n", r);

    return 0;
}
