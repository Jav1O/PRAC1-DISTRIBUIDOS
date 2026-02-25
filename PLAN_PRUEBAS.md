# Plan de pruebas (EE1 Distribuidos)

## Objetivo
Validar que la API de tuplas cumple el enunciado en modo no distribuido (`libclaves.so`) y distribuido con colas POSIX (`libproxyclaves.so` + `servidor_mq`).

## Entorno
- Linux
- Compilación con `make`

## Casos de prueba

### Bloque A: funcional básico
1. `destroy` inicial -> `0`
2. `set_value` válido -> `0`
3. `exist` clave insertada -> `1`
4. `get_value` clave insertada -> `0` y datos correctos
5. `modify_value` -> `0`
6. `get_value` tras modificar -> `0` y datos actualizados
7. `delete_key` -> `0`
8. `exist` tras borrar -> `0`

Cliente: `app-cliente.c`

### Bloque B: errores de servicio (`-1`)
1. `set_value` duplicada -> `-1`
2. `get_value` inexistente -> `-1`
3. `modify_value` inexistente -> `-1`
4. `delete_key` inexistente -> `-1`
5. `set_value` con `N_value2 > 32` -> `-1`
6. `set_value` con `value1` de más de 255 -> `-1`

Cliente: `app-cliente-2.c`

### Bloque C: error de comunicaciones (`-2`) en distribuido
1. Servidor parado
2. Operación API (por ejemplo `exist`) debe devolver `-2`

Cliente: `app-cliente-4.c` enlazado con `libproxyclaves.so`

### Bloque D: concurrencia
1. Servidor activo
2. Ejecutar varias instancias concurrentes de `app-cliente-3.c`
3. Cada instancia realiza muchos ciclos `set/get/delete`
4. Todas deben finalizar con código 0

## Ejecución manual sugerida
1. `make clean && make`
2. Modo local: `./app_cliente_local` y `./app_cliente2_local`
3. Modo distribuido:
   - terminal 1: `./servidor_mq`
   - terminal 2: `./app_cliente_mq` y `./app_cliente2_mq`
4. Comunicación: con servidor parado ejecutar `./app_cliente4_mq`
5. Concurrencia: lanzar varias instancias de `./app_cliente3_mq`

## Criterio de aceptación
- Todos los casos deben producir los códigos esperados.
- En distribuido, errores de transporte deben observarse como `-2`.
- No debe haber bloqueos ni corrupción bajo concurrencia.
