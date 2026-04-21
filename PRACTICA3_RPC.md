# Sistemas Distribuidos (Curso 2025-2026)
## Ejercicio Evaluable 3: RPC

## 1. Objetivo del ejercicio
Se debe diseñar e implementar **el mismo servicio** realizado en los ejercicios evaluables 1 y 2, pero en este caso utilizando **ONC RPC**.

La funcionalidad del servicio debe mantenerse. Lo que cambia en este ejercicio es el mecanismo de comunicación entre cliente y servidor, que ahora debe implementarse mediante RPC.

---

## 2. Relación con ejercicios anteriores
Este ejercicio **reutiliza** el trabajo de ejercicios anteriores.

El enunciado indica que los ficheros:

- `claves.h`
- `claves.c`
- `app-cliente.c`

son los mismos que los implementados en el primer ejercicio.

Además, se indica que **todo el código desarrollado en el ejercicio 1 sigue siendo válido en este ejercicio, excepto el código relacionado con las RPC**.

---

## 3. Archivo nuevo obligatorio
Debe implementarse inicialmente el archivo:

- `clavesRPC.x`

Este archivo debe definir una interfaz similar a la definida en `claves.h` y se utilizará para obtener los archivos que genera automáticamente `rpcgen`, que darán soporte a la funcionalidad del ejercicio.

---

## 4. Generación de código RPC
A partir de `clavesRPC.x`, se deben generar los archivos necesarios para RPC con el comando:

```bash
rpcgen -aNM clavesRPC.x
```

Los archivos obtenidos deberán integrarse de forma adecuada en la solución final.

---

## 5. Estructura general de la aplicación
Según el diagrama del enunciado, la aplicación tiene una estructura cliente-servidor con componentes RPC generados e integrados entre ambos lados.

### Lado servidor
En el lado servidor aparecen, al menos, estos elementos:

- `claves.c`
- `claves.h`
- `clavesRPC.x`
- biblioteca `libclaves.so`
- ficheros generados por `rpcgen` del lado servidor (por ejemplo `_svc` y `_xdr`)
- ejecutable del servidor

### Lado cliente
En el lado cliente aparecen, al menos, estos elementos:

- `app-cliente.c`
- `claves.h`
- un módulo proxy RPC (en el diagrama aparece `proxy-rpc.c`)
- ficheros generados por `rpcgen` del lado cliente (por ejemplo `_clnt` y `_xdr`)
- una biblioteca del lado cliente (en el diagrama aparece `libproxyclaves.so`)
- ejecutable del cliente

### Implicación práctica
Parte del ejercicio consiste en integrar correctamente los archivos generados por `rpcgen` en el diseño final de cliente, biblioteca y servidor.

---

## 6. Aclaraciones adicionales del enunciado

### 6.1 Tratamiento de errores
En las llamadas de la interfaz de tuplas, también se considera error que se produzca un error en el sistema de RPC.

Esto implica que un fallo de RPC debe reflejarse como error de la operación correspondiente.

### 6.2 Dirección IP del servidor
El cliente y la biblioteca a desarrollar deben conocer la dirección IP del servidor que ofrece el servicio de tuplas.

Para evitar fijar la dirección IP en el código y para no pasarla en la línea de mandatos del cliente, se utilizará la variable de entorno:

```bash
IP_TUPLAS
```

Esta variable de entorno define la IP donde ejecuta el servidor RPC.

Debe definirse en cada terminal donde se ejecute el cliente.

Ejemplo:

```bash
export IP_TUPLAS=127.0.0.1
```

### 6.3 Puerto del servidor
Para las RPC no es necesario que el cliente conozca el puerto del servidor.

### 6.4 Ejecución del servidor
El servidor debe ejecutarse de la siguiente forma:

```bash
./clavesRPC_server
```

No debe requerir un puerto pasado por línea de argumentos.

---

## 7. Aspectos importantes del desarrollo
El enunciado destaca como parte importante del ejercicio:

1. Diseñar y especificar correctamente la interfaz de servicio en el archivo `.x`.
2. Descubrir cómo integrar los archivos generados por `rpcgen` para dar soporte a la funcionalidad pedida.
3. Incorporar correctamente esos archivos a la solución final.

---

## 8. Material a entregar
Se debe entregar un fichero:

- `ejercicio_evaluable3.zip`

Ese fichero debe incluir, además de todos los archivos necesarios para compilar cliente y servidor, los siguientes elementos:

### 8.1 Makefile
Debe incluirse un `Makefile` que permita:

- compilar todos los archivos necesarios
- generar la biblioteca `libclaves.so`
- generar dos ejecutables:
  - el ejecutable del servidor, que implementa el servicio
  - el ejecutable del cliente o clientes utilizados para probar el sistema
- compilar el cliente a partir de `cliente.c` (u otros clientes de prueba) y la biblioteca `libclaves.so`
- incluir los ficheros obtenidos con `rpcgen` que sean necesarios
- incluir una regla para invocar `rpcgen` y crear los archivos necesarios para la aplicación final

> Nota: en el PDF aparece `rpcpgen` en una línea del apartado del Makefile, pero el propio enunciado antes fija claramente el comando `rpcgen -aNM clavesRPC.x`, así que debe entenderse como una errata.

### 8.2 Memoria en PDF
Debe incluirse una pequeña memoria en PDF de **no más de cinco páginas, incluida la portada**, indicando:

- el diseño realizado
- la estructura de archivos utilizada en la compilación del servidor
- la estructura de archivos utilizada en la compilación de la biblioteca `libclaves.so`
- la estructura de archivos utilizada en la compilación del cliente
- la forma de compilar y generar el ejecutable del cliente
- la forma de compilar y generar el ejecutable del servidor

### 8.3 Archivos adicionales
Deben incluirse todos los archivos adicionales necesarios para el desarrollo del servicio, por ejemplo:

- ficheros auxiliares
- ficheros de estructuras de datos
- cualquier otro módulo necesario para la implementación

---

## 9. Requisitos sobre almacenamiento
Para el almacenamiento de los elementos `key-value1-value2-value3` puede utilizarse la estructura de datos o el mecanismo de almacenamiento que se considere más adecuado, por ejemplo:

- listas
- ficheros
- otras estructuras

La estructura elegida debe describirse en la memoria.

La estructura elegida **no debe fijar un límite** en el número de elementos que se pueden almacenar.

---

## 10. Concurrencia
Se recomienda probar el servidor con varios clientes de forma concurrente.

---

## 11. Restricciones de interfaz
Se debe respetar lo siguiente:

- **No se puede modificar el prototipo** de las funciones definidas en `claves.h`.
- **No se pueden añadir más funciones**.

---

## 12. Makefile
El enunciado recuerda expresamente que hay que **modificar el `Makefile`** para adaptarlo a este ejercicio.

---

## 13. Entrega
La entrega se realizará mediante Aula Global en el entregador habilitado.

### Fecha límite
**26/04/2026 (23:55 horas)**

---

# Guía operativa para usar este enunciado con Copilot
Esta sección no añade requisitos nuevos. Solo traduce el enunciado a tareas concretas para que Copilot no se desvíe.

## Objetivo técnico
Implementar el servicio de claves ya existente, sustituyendo la comunicación previa por una solución basada en ONC RPC.

## Reglas que deben respetarse
- Mantener la misma funcionalidad que en ejercicios anteriores.
- Reutilizar `claves.h`, `claves.c` y `app-cliente.c`.
- No modificar prototipos de `claves.h`.
- No añadir nuevas funciones públicas.
- Definir `clavesRPC.x` como interfaz RPC equivalente a la interfaz pública.
- Generar código con `rpcgen -aNM clavesRPC.x`.
- Integrar correctamente los archivos generados por `rpcgen`.
- Hacer que los fallos de RPC cuenten como error de la operación.
- Obtener la IP del servidor desde la variable de entorno `IP_TUPLAS`.
- Ejecutar el servidor como `./clavesRPC_server`.
- Adaptar el `Makefile` para automatizar compilación y generación de archivos RPC.
- Preparar una solución que pueda probarse con varios clientes concurrentes.

## Estructura mínima que Copilot debe asumir
### Reutilizados
- `claves.h`
- `claves.c`
- `app-cliente.c`

### Nuevos o adaptados
- `clavesRPC.x`
- proxy RPC del lado cliente
- ficheros generados por `rpcgen`
- `Makefile`
- módulos auxiliares si hacen falta para almacenamiento o sincronización

## Qué NO debe hacer Copilot
- No reescribir toda la lógica desde cero.
- No inventar una interfaz pública distinta.
- No añadir parámetros nuevos a las funciones de `claves.h`.
- No obligar al cliente a pasar la IP o el puerto por línea de comandos si no es necesario.
- No fijar un límite artificial al número de elementos almacenados.

## Qué SÍ debe hacer Copilot
1. Leer la interfaz actual de `claves.h`.
2. Traducirla a `clavesRPC.x`.
3. Generar e integrar el código RPC.
4. Implementar el servidor RPC apoyándose en la lógica de `claves.c`.
5. Implementar el proxy del lado cliente.
6. Adaptar el `Makefile`.
7. Verificar compilación, ejecución y pruebas básicas con concurrencia.

## Orden recomendado de implementación
1. Revisar `claves.h`.
2. Crear `clavesRPC.x`.
3. Ejecutar `rpcgen`.
4. Integrar stubs cliente/servidor y serialización XDR.
5. Implementar servidor RPC.
6. Implementar proxy cliente.
7. Adaptar `Makefile`.
8. Probar con `IP_TUPLAS` configurada.
9. Probar con varios clientes concurrentes.
