/*
 * app-cliente-5.c — Cliente de pruebas de límites máximos válidos.
 *
 * Verifica que la implementación acepta correctamente los tamaños máximos
 * permitidos por el enunciado: key y value1 de 255 caracteres, y value2 con
 * 32 elementos.
 */

#include "claves.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *label, int got, int expected) {
    printf("[FAIL] %s -> got=%d expected=%d\n", label, got, expected);
    return 1;
}

int main(void) {
    char key[256];
    char value1[256];
    char out_value1[256];
    float value2[32];
    float out_value2[32] = {0.0f};
    int out_n = 0;
    int result;
    struct Paquete p = {.x = 101, .y = 202, .z = 303};
    struct Paquete out_p;

    memset(key, 'k', 255);
    key[255] = '\0';
    memset(value1, 'v', 255);
    value1[255] = '\0';

    for (int i = 0; i < 32; ++i) {
        value2[i] = (float)i + 0.5f;
    }

    result = destroy();
    if (result != 0) {
        return fail("destroy inicial", result, 0);
    }

    result = set_value(key, value1, 32, value2, p);
    if (result != 0) {
        return fail("set_value maximos", result, 0);
    }

    result = exist(key);
    if (result != 1) {
        return fail("exist maxima", result, 1);
    }

    result = get_value(key, out_value1, &out_n, out_value2, &out_p);
    if (result != 0) {
        return fail("get_value maximos", result, 0);
    }

    if (strcmp(value1, out_value1) != 0 || out_n != 32 || out_p.x != p.x || out_p.y != p.y || out_p.z != p.z) {
        printf("[FAIL] datos maximos inconsistentes\n");
        return 1;
    }

    for (int i = 0; i < 32; ++i) {
        if (out_value2[i] != value2[i]) {
            printf("[FAIL] value2 maximo inconsistente en posicion %d\n", i);
            return 1;
        }
    }

    result = delete_key(key);
    if (result != 0) {
        return fail("delete_key maxima", result, 0);
    }

    result = destroy();
    if (result != 0) {
        return fail("destroy final", result, 0);
    }

    printf("OK: limites maximos validos\n");
    return 0;
}