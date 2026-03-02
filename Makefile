# ============================================================================
# Makefile — Ejercicio Evaluable 1: Servicio de tuplas con colas POSIX.
#
# Genera:
#   libclaves.so        Biblioteca local (API directa sobre la tabla hash).
#   libproxyclaves.so   Biblioteca proxy (envía peticiones al servidor MQ).
#   servidor_mq         Servidor concurrente con colas POSIX.
#   app_cliente_*_local  Clientes enlazados con libclaves.so (uso local).
#   app_cliente_*_mq     Clientes enlazados con libproxyclaves.so (uso MQ).
#
# Uso:
#   make            Compila todo.
#   make clean      Elimina binarios y objetos.
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -fPIC -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS_SHARED = -shared
LDFLAGS_MQ = -lrt -pthread
RPATH_ORIGIN = -Wl,-rpath,'$$ORIGIN'

.PHONY: all clean

all: libclaves.so libproxyclaves.so servidor_mq \
	app_cliente_local app_cliente_mq \
	app_cliente2_local app_cliente2_mq \
	app_cliente3_local app_cliente3_mq \
	app_cliente4_mq

libclaves.so: claves.o
	$(CC) $(LDFLAGS_SHARED) -o $@ $^ -pthread

libproxyclaves.so: proxy-mq.o
	$(CC) $(LDFLAGS_SHARED) -o $@ $^ $(LDFLAGS_MQ)

servidor_mq: servidor-mq.o libclaves.so
	$(CC) -o $@ servidor-mq.o -L. -lclaves $(RPATH_ORIGIN) $(LDFLAGS_MQ)

app_cliente_local: app-cliente.o libclaves.so
	$(CC) -o $@ app-cliente.o -L. -lclaves $(RPATH_ORIGIN) -pthread

app_cliente_mq: app-cliente.o libproxyclaves.so
	$(CC) -o $@ app-cliente.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_MQ)

app_cliente2_local: app-cliente-2.o libclaves.so
	$(CC) -o $@ app-cliente-2.o -L. -lclaves $(RPATH_ORIGIN) -pthread

app_cliente2_mq: app-cliente-2.o libproxyclaves.so
	$(CC) -o $@ app-cliente-2.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_MQ)

app_cliente3_local: app-cliente-3.o libclaves.so
	$(CC) -o $@ app-cliente-3.o -L. -lclaves $(RPATH_ORIGIN) -pthread

app_cliente3_mq: app-cliente-3.o libproxyclaves.so
	$(CC) -o $@ app-cliente-3.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_MQ)

app_cliente4_mq: app-cliente-4.o libproxyclaves.so
	$(CC) -o $@ app-cliente-4.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_MQ)

claves.o: claves.c claves.h
	$(CC) $(CFLAGS) -c claves.c -o $@

proxy-mq.o: proxy-mq.c claves.h protocol_mq.h
	$(CC) $(CFLAGS) -c proxy-mq.c -o $@

servidor-mq.o: servidor-mq.c claves.h protocol_mq.h
	$(CC) $(CFLAGS) -c servidor-mq.c -o $@

app-cliente.o: app-cliente.c claves.h
	$(CC) $(CFLAGS) -c app-cliente.c -o $@

app-cliente-2.o: app-cliente-2.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-2.c -o $@

app-cliente-3.o: app-cliente-3.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-3.c -o $@

app-cliente-4.o: app-cliente-4.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-4.c -o $@

clean:
	rm -f *.o *.so servidor_mq \
		app_cliente_local app_cliente_mq \
		app_cliente2_local app_cliente2_mq \
		app_cliente3_local app_cliente3_mq \
		app_cliente4_mq
