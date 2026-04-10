/*
 * app-cliente-6.c — Cliente para colisión concurrente sobre la misma clave.
 *
 * Lanza una única inserción sobre la clave recibida por argumento. Se usa en
 * paralelo desde el script de pruebas para verificar que solo una inserción
 * tiene éxito y la otra falla con -1.
 */

#include "claves.h"

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    const char *key = (argc > 1) ? argv[1] : "clave_compartida";
    char value1[64];
    float value2[1] = {42.0f};
    struct Paquete p = {.x = (int)getpid(), .y = 2, .z = 3};
    int result;

    snprintf(value1, sizeof(value1), "pid_%d", (int)getpid());
    result = set_value((char *)key, value1, 1, value2, p);

    if (result != 0 && result != -1) {
        printf("%d\n", result);
        return 1;
    }

    printf("%d\n", result);
    return 0;
}