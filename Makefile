# ============================================================================
# Makefile — Ejercicio Evaluable 3: Servicio de tuplas con ONC RPC.
#
# Genera:
#   libclaves.so       Biblioteca cliente que implementa la API sobre RPC.
#   clavesRPC_server   Servidor ONC RPC.
#   app_cliente*_rpc   Clientes de prueba enlazados con la biblioteca RPC.
#
# Uso:
#   make            Compila todo y regenera los ficheros RPC si es necesario.
#   make clean      Elimina binarios, objetos y ficheros generados por rpcgen.
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -fPIC -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS_SHARED = -shared
RPC_CFLAGS = -I/usr/include/tirpc
RPC_GEN_WARNING_CFLAGS = -Wno-unused-variable -Wno-unused-parameter -Wno-cast-function-type
RPC_LIBS = -ltirpc -pthread
RPATH_ORIGIN = -Wl,-rpath,'$$ORIGIN'
RPC_GEN_FILES = clavesRPC.h clavesRPC_clnt.c clavesRPC_svc.c clavesRPC_xdr.c

.PHONY: all clean rpcgen

all: libclaves.so clavesRPC_server \
	app_cliente_rpc \
	app_cliente2_rpc \
	app_cliente3_rpc \
	app_cliente4_rpc \
	app_cliente5_rpc \
	app_cliente6_rpc

libclaves.so: proxy-rpc.o clavesRPC_clnt.o clavesRPC_xdr.o $(RPC_GEN_FILES)
	$(CC) $(LDFLAGS_SHARED) -o $@ proxy-rpc.o clavesRPC_clnt.o clavesRPC_xdr.o $(RPC_LIBS)

clavesRPC_server: claves.o rpc-service.o clavesRPC_svc.o clavesRPC_xdr.o $(RPC_GEN_FILES)
	$(CC) -o $@ claves.o rpc-service.o clavesRPC_svc.o clavesRPC_xdr.o $(RPC_LIBS)

app_cliente_rpc: app-cliente.o libclaves.so
	$(CC) -o $@ app-cliente.o -L. -lclaves $(RPATH_ORIGIN) $(RPC_LIBS)

app_cliente2_rpc: app-cliente-2.o libclaves.so
	$(CC) -o $@ app-cliente-2.o -L. -lclaves $(RPATH_ORIGIN) $(RPC_LIBS)

app_cliente3_rpc: app-cliente-3.o libclaves.so
	$(CC) -o $@ app-cliente-3.o -L. -lclaves $(RPATH_ORIGIN) $(RPC_LIBS)

app_cliente4_rpc: app-cliente-4.o libclaves.so
	$(CC) -o $@ app-cliente-4.o -L. -lclaves $(RPATH_ORIGIN) $(RPC_LIBS)

app_cliente5_rpc: app-cliente-5.o libclaves.so
	$(CC) -o $@ app-cliente-5.o -L. -lclaves $(RPATH_ORIGIN) $(RPC_LIBS)

app_cliente6_rpc: app-cliente-6.o libclaves.so
	$(CC) -o $@ app-cliente-6.o -L. -lclaves $(RPATH_ORIGIN) $(RPC_LIBS)

rpcgen: $(RPC_GEN_FILES)

$(RPC_GEN_FILES): clavesRPC.x
	rm -f Makefile.clavesRPC
	rpcgen -aNM clavesRPC.x
	rm -f clavesRPC_client.c clavesRPC_server.c Makefile.clavesRPC

claves.o: claves.c claves.h
	$(CC) $(CFLAGS) -c claves.c -o $@

proxy-rpc.o: proxy-rpc.c claves.h clavesRPC.h
	$(CC) $(CFLAGS) $(RPC_CFLAGS) -c proxy-rpc.c -o $@

rpc-service.o: rpc-service.c claves.h clavesRPC.h
	$(CC) $(CFLAGS) $(RPC_CFLAGS) -c rpc-service.c -o $@

clavesRPC_clnt.o: clavesRPC_clnt.c clavesRPC.h
	$(CC) $(CFLAGS) $(RPC_CFLAGS) $(RPC_GEN_WARNING_CFLAGS) -c clavesRPC_clnt.c -o $@

clavesRPC_svc.o: clavesRPC_svc.c clavesRPC.h
	$(CC) $(CFLAGS) $(RPC_CFLAGS) $(RPC_GEN_WARNING_CFLAGS) -c clavesRPC_svc.c -o $@

clavesRPC_xdr.o: clavesRPC_xdr.c clavesRPC.h
	$(CC) $(CFLAGS) $(RPC_CFLAGS) $(RPC_GEN_WARNING_CFLAGS) -c clavesRPC_xdr.c -o $@

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
	rm -f *.o *.so clavesRPC_server \
		app_cliente_rpc \
		app_cliente2_rpc \
		app_cliente3_rpc \
		app_cliente4_rpc \
		app_cliente5_rpc \
		app_cliente6_rpc \
		$(RPC_GEN_FILES) clavesRPC_client.c clavesRPC_server.c Makefile.clavesRPC
