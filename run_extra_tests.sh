#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

CLIENTS_LIST=(1 2 4 8)
ITERS_PER_CLIENT=300

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
  exit 1
}

echo "[extra] Compilando..."
make clean >/dev/null
make >/dev/null

echo "[extra] Verificando rpcbind..."
require_rpcbind

echo "[extra] Benchmark de concurrencia (iteraciones por cliente: ${ITERS_PER_CLIENT})"
printf "%10s | %12s | %14s\n" "clientes" "tiempo(s)" "ops/s aprox"
printf "%10s-+-%12s-+-%14s\n" "----------" "------------" "--------------"

for clients in "${CLIENTS_LIST[@]}"; do
  ./clavesRPC_server >/tmp/claves_rpc_extra.log 2>&1 &
  SERVER_PID=$!
  sleep 1

  if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    cat /tmp/claves_rpc_extra.log
    exit 1
  fi

  start_ns="$(date +%s%N)"
  pids=()

  for i in $(seq 1 "$clients"); do
    env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc "extra_${clients}_${i}" "$ITERS_PER_CLIENT" &
    pids+=("$!")
  done

  for pid in "${pids[@]}"; do
    wait "$pid"
  done

  end_ns="$(date +%s%N)"

  cleanup
  SERVER_PID=""

  elapsed_ns=$((end_ns - start_ns))
  elapsed_s=$(awk -v ns="$elapsed_ns" 'BEGIN { printf "%.3f", ns/1000000000 }')

  total_ops=$((clients * ITERS_PER_CLIENT * 3))
  ops_s=$(awk -v ops="$total_ops" -v ns="$elapsed_ns" 'BEGIN { if (ns == 0) print "0.0"; else printf "%.1f", ops/(ns/1000000000) }')

  printf "%10d | %12s | %14s\n" "$clients" "$elapsed_s" "$ops_s"
done

echo "[extra] Prueba prolongada adicional (4 clientes, 1000 iteraciones)"
./clavesRPC_server >/tmp/claves_rpc_long.log 2>&1 &
SERVER_PID=$!
sleep 1

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
  cat /tmp/claves_rpc_long.log
  exit 1
fi

pids=()

for i in 1 2 3 4; do
  env IP_TUPLAS=127.0.0.1 ./app_cliente3_rpc "long_${i}" 1000 &
  pids+=("$!")
done

for pid in "${pids[@]}"; do
  wait "$pid"
done

cleanup
SERVER_PID=""

echo "[extra] OK: pruebas de estrés completadas"
