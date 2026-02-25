#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    if kill -0 "$SERVER_PID" 2>/dev/null; then
      kill "$SERVER_PID" 2>/dev/null || true
      wait "$SERVER_PID" 2>/dev/null || true
    fi
  fi
}
trap cleanup EXIT

echo "[1/6] Compilando..."
make clean >/dev/null
make >/dev/null

echo "[2/6] Pruebas funcionales locales..."
./app_cliente_local
./app_cliente2_local

echo "[3/6] Pruebas funcionales distribuidas..."
./servidor_mq >/tmp/servidor_mq.log 2>&1 &
SERVER_PID=$!
sleep 1
./app_cliente_mq
./app_cliente2_mq

cleanup
SERVER_PID=""

echo "[4/6] Prueba error comunicaciones (-2) con servidor apagado..."
./app_cliente4_mq

echo "[5/6] Prueba concurrencia distribuida..."
./servidor_mq >/tmp/servidor_mq.log 2>&1 &
SERVER_PID=$!
sleep 1

P1=0 P2=0 P3=0 P4=0
./app_cliente3_mq t1 200 & P1=$!
./app_cliente3_mq t2 200 & P2=$!
./app_cliente3_mq t3 200 & P3=$!
./app_cliente3_mq t4 200 & P4=$!

wait "$P1"
wait "$P2"
wait "$P3"
wait "$P4"

cleanup
SERVER_PID=""

echo "[6/6] Todo OK"
