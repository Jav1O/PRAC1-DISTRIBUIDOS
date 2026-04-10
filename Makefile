# ============================================================================
# Makefile — Ejercicio Evaluable 2: Servicio de tuplas con sockets TCP.
#
# Genera:
#   libclaves.so         Biblioteca local (API directa sobre la tabla hash).
#   libproxyclaves.so    Biblioteca proxy (envía peticiones al servidor TCP).
#   servidor             Servidor concurrente con sockets TCP.
#   app_cliente_*_local  Clientes enlazados con libclaves.so (uso local).
#   app_cliente_*_sock   Clientes enlazados con libproxyclaves.so (uso TCP).
#
# Uso:
#   make            Compila todo.
#   make clean      Elimina binarios y objetos.
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -fPIC -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS_SHARED = -shared
LDFLAGS_TCP = -pthread
RPATH_ORIGIN = -Wl,-rpath,'$$ORIGIN'

.PHONY: all clean

all: libclaves.so libproxyclaves.so servidor servidor_sock \
	app_cliente_local app_cliente_sock \
	app_cliente2_local app_cliente2_sock \
	app_cliente3_local app_cliente3_sock \
	app_cliente4_sock \
	app_cliente5_local app_cliente5_sock \
	app_cliente6_sock

libclaves.so: claves.o
	$(CC) $(LDFLAGS_SHARED) -o $@ $^ -pthread

libproxyclaves.so: proxy-sock.o protocol_sock.o
	$(CC) $(LDFLAGS_SHARED) -o $@ $^ $(LDFLAGS_TCP)

servidor: servidor-sock.o protocol_sock.o libclaves.so
	$(CC) -o $@ servidor-sock.o protocol_sock.o -L. -lclaves $(RPATH_ORIGIN) $(LDFLAGS_TCP)

servidor_sock: servidor
	ln -sf servidor servidor_sock

app_cliente_local: app-cliente.o libclaves.so
	$(CC) -o $@ app-cliente.o -L. -lclaves $(RPATH_ORIGIN) -pthread

app_cliente_sock: app-cliente.o libproxyclaves.so
	$(CC) -o $@ app-cliente.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_TCP)

app_cliente2_local: app-cliente-2.o libclaves.so
	$(CC) -o $@ app-cliente-2.o -L. -lclaves $(RPATH_ORIGIN) -pthread

app_cliente2_sock: app-cliente-2.o libproxyclaves.so
	$(CC) -o $@ app-cliente-2.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_TCP)

app_cliente3_local: app-cliente-3.o libclaves.so
	$(CC) -o $@ app-cliente-3.o -L. -lclaves $(RPATH_ORIGIN) -pthread

app_cliente3_sock: app-cliente-3.o libproxyclaves.so
	$(CC) -o $@ app-cliente-3.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_TCP)

app_cliente4_sock: app-cliente-4.o libproxyclaves.so
	$(CC) -o $@ app-cliente-4.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_TCP)

app_cliente5_local: app-cliente-5.o libclaves.so
	$(CC) -o $@ app-cliente-5.o -L. -lclaves $(RPATH_ORIGIN) -pthread

app_cliente5_sock: app-cliente-5.o libproxyclaves.so
	$(CC) -o $@ app-cliente-5.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_TCP)

app_cliente6_sock: app-cliente-6.o libproxyclaves.so
	$(CC) -o $@ app-cliente-6.o -L. -lproxyclaves $(RPATH_ORIGIN) $(LDFLAGS_TCP)

claves.o: claves.c claves.h
	$(CC) $(CFLAGS) -c claves.c -o $@

protocol_sock.o: protocol_sock.c protocol_sock.h claves.h
	$(CC) $(CFLAGS) -c protocol_sock.c -o $@

proxy-sock.o: proxy-sock.c claves.h protocol_sock.h
	$(CC) $(CFLAGS) -c proxy-sock.c -o $@

servidor-sock.o: servidor-sock.c claves.h protocol_sock.h
	$(CC) $(CFLAGS) -c servidor-sock.c -o $@

app-cliente.o: app-cliente.c claves.h
	$(CC) $(CFLAGS) -c app-cliente.c -o $@

app-cliente-2.o: app-cliente-2.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-2.c -o $@

app-cliente-3.o: app-cliente-3.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-3.c -o $@

app-cliente-4.o: app-cliente-4.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-4.c -o $@

app-cliente-5.o: app-cliente-5.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-5.c -o $@

app-cliente-6.o: app-cliente-6.c claves.h
	$(CC) $(CFLAGS) -c app-cliente-6.c -o $@

clean:
	rm -f *.o *.so servidor servidor_sock \
		app_cliente_local app_cliente_sock \
		app_cliente2_local app_cliente2_sock \
		app_cliente3_local app_cliente3_sock \
		app_cliente4_sock \
		app_cliente5_local app_cliente5_sock \
		app_cliente6_sock
