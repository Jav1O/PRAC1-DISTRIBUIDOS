#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

SERVER_PID=""

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    if kill -0 "$SERVER_PID" 2>/dev/null; then
      kill "$SERVER_PID" 2>/dev/null || true
      wait "$SERVER_PID" 2>/dev/null || true
    fi
  fi
}
trap cleanup EXIT

require_rpcbind() {
  if command -v rpcinfo >/dev/null 2>&1 && rpcinfo -p localhost >/dev/null 2>&1; then
    return 0
  fi

  echo "FAIL: rpcbind no esta activo. Arranca el servicio con 'sudo systemctl start rpcbind' o 'sudo service rpcbind start'."
  return 1
}

start_server() {
  ./clavesRPC_server >"/tmp/claves_rpc_server.log" 2>&1 &
  SERVER_PID=$!
  sleep 1

  if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    cat "/tmp/claves_rpc_server.log"
    return 1
  fi
}

stop_server() {
  cleanup
  SERVER_PID=""
}

check_collision_same_key() {
  local out1 out2 r1 r2

  env IP_TUPLAS=127.0.0.1 ./app_cliente_rpc >/dev/null
  out1="$(mktemp)"
  out2="$(mktemp)"

  env IP_TUPLAS=127.0.0.1 ./app_cliente6_rpc clave_compartida >"$out1" &
  P1=$!
  env IP_TUPLAS=127.0.0.1 ./app_cliente6_rpc clave_compartida >"$out2" &
  P2=$!

  wait "$P1"
  wait "$P2"

  r1="$(tr -d '[:space:]' <"$out1")"
  r2="$(tr -d '[:space:]' <"$out2")"
  rm -f "$out1" "$out2"

  if [[ "$r1" == "0" && "$r2" == "-1" ]] || [[ "$r1" == "-1" && "$r2" == "0" ]]; then
    echo "OK: colision concurrente sobre la misma clave validada"
    return 0
  fi

  echo "FAIL: se esperaba una insercion 0 y otra -1, pero se obtuvo r1=$r1 r2=$r2"
  return 1
}

echo "[1/12] Compilando..."
make clean >/dev/null
make >/dev/null

echo "[rpcbind] Verificando servicio de portmapper..."
require_rpcbind

echo "[2/12] Pruebas funcionales RPC por IP..."
start_server
env IP_TUPLAS=127.0.0.1 ./app_cliente_rpc
env IP_TUPLAS=127.0.0.1 ./app_cliente2_rpc
env IP_TUPLAS=127.0.0.1 ./app_cliente5_rpc

echo "[4/12] Prueba de resolucion por nombre de host..."
env IP_TUPLAS=localhost ./app_cliente_rpc >/dev/null
stop_server

echo "[5/12] Pruebas de configuracion invalida en variables de entorno..."
env -u IP_TUPLAS ./app_cliente4_rpc
env IP_TUPLAS= ./app_cliente4_rpc
env IP_TUPLAS=no-such-host.invalid ./app_cliente4_rpc

echo "[6/12] Prueba error RPC (-1) con servidor apagado..."
env IP_TUPLAS=127.0.0.1 ./app_cliente4_rpc

echo "[7/12] Prueba de host incorrecto con servidor activo..."
start_server
env IP_TUPLAS=host-inexistente.invalid ./app_cliente4_rpc
stop_server

echo "[8/12] Prueba de reinicio del servidor..."
start_server
env IP_TUPLAS=127.0.0.1 ./app_cliente_rpc >/dev/null
stop_server
env IP_TUPLAS=127.0.0.1 ./app_cliente4_rpc
start_server
env IP_TUPLAS=127.0.0.1 ./app_cliente5_rpc >/dev/null
stop_server

echo "[9/12] Validacion de arranque del servidor RPC..."
if ! start_server; then
  echo "FAIL: ./clavesRPC_server deberia iniciar correctamente"
  exit 1
fi
stop_server

echo "[10/12] Prueba concurrencia distribuida con claves unicas..."
start_server

P1=0 P2=0 P3=0 P4=0
env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc t1 200 & P1=$!
env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc t2 200 & P2=$!
env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc t3 200 & P3=$!
env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc t4 200 & P4=$!

wait "$P1"
wait "$P2"
wait "$P3"
wait "$P4"

echo "[11/12] Prueba de colision concurrente sobre la misma clave..."
check_collision_same_key

echo "[12/12] Prueba prolongada de estabilidad..."
P1=0 P2=0
env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc larga1 1000 & P1=$!
env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc larga2 1000 & P2=$!
wait "$P1"
wait "$P2"

stop_server

echo "[final] Todo OK"
