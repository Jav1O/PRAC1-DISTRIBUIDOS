# Plan de pruebas (EE2 Distribuidos)

## Objetivo
Validar que la API de tuplas cumple el enunciado en modo no distribuido (`libclaves.so`) y distribuido con sockets TCP (`libproxyclaves.so` + `servidor`).

Además, medir comportamiento bajo carga para documentar el comportamiento concurrente del servidor TCP.

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
1. Servidor parado o puerto sin servicio escuchando
2. Operación API (por ejemplo `exist`) debe devolver `-2`

Cliente: `app-cliente-4.c` enlazado con `libproxyclaves.so`

### Bloque D: concurrencia
1. Servidor activo
2. Ejecutar varias instancias concurrentes de `app-cliente-3.c`
3. Cada instancia realiza muchos ciclos `set/get/delete`
4. Todas deben finalizar con código 0

### Bloque E: estrés y rendimiento (Trabajo extra)
1. Ejecutar batería de carga con distinto número de clientes concurrentes (por ejemplo: 1, 2, 4, 8)
2. Cada cliente realiza un número fijo de iteraciones (por ejemplo: 300)
3. Medir tiempo total del escenario y calcular throughput aproximado de operaciones/segundo
4. Verificar que no aparecen fallos (`set/get/delete`) ni bloqueos en escenarios de mayor carga

Script sugerido: `run_extra_tests.sh`

### Bloque F: resolución y configuración TCP
1. Ejecutar el cliente con `IP_TUPLAS=127.0.0.1`
2. Ejecutar el cliente con `IP_TUPLAS=localhost`
3. Ejecutar el cliente sin `IP_TUPLAS`
4. Ejecutar el cliente sin `PORT_TUPLAS`
5. Ejecutar el cliente con `PORT_TUPLAS` inválido (`abc`, `0`, `70000`)
6. En los casos inválidos la API debe devolver `-2`

Cliente: `app-cliente-4.c` y `app-cliente.c`

### Bloque G: límites máximos válidos
1. Insertar una clave de longitud 255
2. Insertar un `value1` de longitud 255
3. Insertar un `value2` con `N_value2 = 32`
4. Recuperar la tupla y comprobar que todos los datos se conservan

Cliente: `app-cliente-5.c`

### Bloque H: puerto de servidor y reconexión
1. Arrancar el servidor en un puerto distinto al usado por el cliente y verificar retorno `-2`
2. Arrancar el servidor con un puerto inválido y verificar que el proceso falla
3. Parar y reiniciar el servidor entre operaciones y comprobar recuperación del servicio

Script: `run_tests.sh`

### Bloque I: colisión concurrente sobre la misma clave
1. Lanzar dos clientes en paralelo intentando insertar exactamente la misma clave
2. Verificar que una inserción devuelve `0` y la otra `-1`

Cliente: `app-cliente-6.c`

### Bloque J: estabilidad prolongada
1. Ejecutar varios clientes concurrentes durante un número alto de iteraciones
2. Verificar ausencia de bloqueos y errores funcionales

Cliente: `app-cliente-3.c`

## Ejecución manual sugerida
1. `make clean && make`
2. Modo local: `./app_cliente_local` y `./app_cliente2_local`
3. Modo distribuido:
   - terminal 1: `./servidor 4500`
   - terminal 2: `env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente_sock`
   - terminal 2: `env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente2_sock`
   - terminal 2: `env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente5_sock`
4. Comunicación: con servidor parado ejecutar `env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente4_sock`
5. Configuración: probar también `env IP_TUPLAS=localhost PORT_TUPLAS=4500 ./app_cliente_sock`
6. Concurrencia: lanzar varias instancias de `env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente3_sock`
7. Estrés y rendimiento: `bash run_extra_tests.sh`

## Criterio de aceptación
- Todos los casos deben producir los códigos esperados.
- En distribuido, errores de transporte TCP deben observarse como `-2`.
- No debe haber bloqueos ni corrupción bajo concurrencia.

## Trazabilidad con el enunciado
- API no distribuida y distribuida verificadas con los mismos clientes de prueba.
- Distinción de errores `-1` (servicio) y `-2` (comunicación TCP) validada explícitamente.
- Concurrencia del servidor validada con múltiples clientes simultáneos.
- Operaciones atómicas validadas indirectamente (sin inconsistencias) en pruebas de concurrencia.
- Configuración mediante variables de entorno y resolución por IP/nombre validadas explícitamente.
- Límites máximos válidos del protocolo verificados con una prueba específica.

## Trabajo extra (texto sugerido para memoria)
Se ha realizado una batería adicional de pruebas de estrés y rendimiento variando el nivel de concurrencia del sistema. Para cada escenario se midió el tiempo total y se estimó el throughput de operaciones, observando el comportamiento del servicio TCP y la ausencia de fallos funcionales bajo carga. Esta batería se ejecuta con `run_extra_tests.sh` y sus resultados se incorporan en la memoria.
