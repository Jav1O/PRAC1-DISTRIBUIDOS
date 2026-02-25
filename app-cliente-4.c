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
