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

start_server() {
  local port="$1"
  ./servidor "$port" >"/tmp/servidor_sock_${port}.log" 2>&1 &
  SERVER_PID=$!
  sleep 1
}

stop_server() {
  cleanup
  SERVER_PID=""
}

check_collision_same_key() {
  local out1 out2 r1 r2

  env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente_sock >/dev/null
  out1="$(mktemp)"
  out2="$(mktemp)"

  env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente6_sock clave_compartida >"$out1" &
  P1=$!
  env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente6_sock clave_compartida >"$out2" &
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

echo "[2/12] Pruebas funcionales locales..."
./app_cliente_local
./app_cliente2_local
./app_cliente5_local

echo "[3/12] Pruebas funcionales distribuidas por IP..."
start_server 4500
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente_sock
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente2_sock
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente5_sock

echo "[4/12] Prueba de resolucion por nombre de host..."
env IP_TUPLAS=localhost PORT_TUPLAS=4500 ./app_cliente_sock >/dev/null
stop_server

echo "[5/12] Pruebas de configuracion invalida en variables de entorno..."
env -u IP_TUPLAS PORT_TUPLAS=4500 ./app_cliente4_sock
env -u PORT_TUPLAS IP_TUPLAS=127.0.0.1 ./app_cliente4_sock
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=abc ./app_cliente4_sock
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=0 ./app_cliente4_sock
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=70000 ./app_cliente4_sock

echo "[6/12] Prueba error comunicaciones (-2) con servidor apagado..."
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente4_sock

echo "[7/12] Prueba de puerto incorrecto con servidor activo en otro puerto..."
start_server 4501
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente4_sock
stop_server

echo "[8/12] Prueba de reinicio del servidor..."
start_server 4500
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente_sock >/dev/null
stop_server
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente4_sock
start_server 4500
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente5_sock >/dev/null
stop_server

echo "[9/12] Validacion de arranque con puertos invalidos..."
if ./servidor abc >/tmp/servidor_sock_invalid.log 2>&1; then
  echo "FAIL: ./servidor abc deberia fallar"
  exit 1
fi
if ./servidor 70000 >/tmp/servidor_sock_invalid.log 2>&1; then
  echo "FAIL: ./servidor 70000 deberia fallar"
  exit 1
fi

echo "[10/12] Prueba concurrencia distribuida con claves unicas..."
start_server 4500

P1=0 P2=0 P3=0 P4=0
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente3_sock t1 200 & P1=$!
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente3_sock t2 200 & P2=$!
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente3_sock t3 200 & P3=$!
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente3_sock t4 200 & P4=$!

wait "$P1"
wait "$P2"
wait "$P3"
wait "$P4"

echo "[11/12] Prueba de colision concurrente sobre la misma clave..."
check_collision_same_key

echo "[12/12] Prueba prolongada de estabilidad..."
P1=0 P2=0
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente3_sock larga1 1000 & P1=$!
env IP_TUPLAS=127.0.0.1 PORT_TUPLAS=4500 ./app_cliente3_sock larga2 1000 & P2=$!
wait "$P1"
wait "$P2"

stop_server

echo "[final] Todo OK"
