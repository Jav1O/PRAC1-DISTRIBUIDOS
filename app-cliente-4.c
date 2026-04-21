/*
 * app-cliente-4.c — Cliente de prueba de error de comunicaciones (Bloque C).
 *
 * Se ejecuta con el servidor apagado o con configuración inválida: la llamada
 * a exist() debe devolver -1 indicando un error de la operación por fallo RPC.
 * Solo se enlaza con la biblioteca cliente RPC.
 */

#include "claves.h"

#include <stdio.h>

int main(void) {
    int r = exist("clave_cualquiera");

    if (r == -1) {
        printf("OK: error remoto detectado (-1)\n");
        return 0;
    }

    printf("FAIL: se esperaba -1 y se obtuvo %d\n", r);
    return 1;
}
