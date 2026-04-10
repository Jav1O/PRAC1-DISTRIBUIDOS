/*
 * app-cliente-4.c — Cliente de prueba de error de comunicaciones (Bloque C).
 *
 * Se ejecuta con el servidor apagado o con configuración inválida: la llamada
 * a exist() debe devolver -2 indicando un error de comunicación TCP.
 * Solo se enlaza con libproxyclaves.so.
 */

#include "claves.h"

#include <stdio.h>

int main(void) {
    int r = exist("clave_cualquiera");

    if (r == -2) {
        printf("OK: error de comunicaciones detectado (-2)\n");
        return 0;
    }

    printf("FAIL: se esperaba -2 y se obtuvo %d\n", r);
    return 1;
}
