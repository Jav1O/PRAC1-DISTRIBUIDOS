/*
 * app-cliente-2.c — Cliente de pruebas de errores (Bloque B del plan de pruebas).
 *
 * Valida que la API devuelve -1 en todos los casos de error documentados:
 * clave duplicada, clave inexistente, N_value2 fuera de rango, value1 > 255.
 * Se enlaza tanto con libclaves.so como con libproxyclaves.so.
 */

#include "claves.h"

#include <stdio.h>
#include <string.h>

/* Función auxiliar para verificar un caso de prueba. */
static int check_eq(const char *label, int got, int expected) {
    if (got != expected) {
        printf("[FAIL] %s -> got=%d expected=%d\n", label, got, expected);
        return 1;
    }
    printf("[ OK ] %s -> %d\n", label, got);
    return 0;
}

int main(void) {
    int fails = 0;

    struct Paquete p = {.x = 1, .y = 2, .z = 3};
    float v2_ok[] = {1.0f, 2.0f};

    fails += check_eq("destroy inicial", destroy(), 0);
    fails += check_eq("set_value valido", set_value("k1", "v1", 2, v2_ok, p), 0);
    fails += check_eq("set_value duplicada", set_value("k1", "v2", 2, v2_ok, p), -1);

    {
        char v1_out[256];
        int n_out = 0;
        float v2_out[32] = {0};
        struct Paquete p_out;
        fails += check_eq("get_value inexistente", get_value("no_existe", v1_out, &n_out, v2_out, &p_out), -1);
    }

    fails += check_eq("modify_value inexistente", modify_value("no_existe", "v", 2, v2_ok, p), -1);
    fails += check_eq("delete_key inexistente", delete_key("no_existe"), -1);

    fails += check_eq("set_value N fuera de rango", set_value("k2", "v2", 33, v2_ok, p), -1);

    {
        char long_v1[257];
        memset(long_v1, 'a', 256);
        long_v1[256] = '\0';
        fails += check_eq("set_value value1>255", set_value("k3", long_v1, 2, v2_ok, p), -1);
    }

    fails += check_eq("exist k1", exist("k1"), 1);
    fails += check_eq("exist no_existe", exist("no_existe"), 0);

    fails += check_eq("destroy final", destroy(), 0);

    if (fails == 0) {
        printf("\nResultado: OK\n");
        return 0;
    }

    printf("\nResultado: FAIL (%d errores)\n", fails);
    return 1;
}
