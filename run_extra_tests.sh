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

echo "[extra] Compilando..."
make clean >/dev/null
make >/dev/null

echo "[extra] Benchmark de concurrencia (iteraciones por cliente: ${ITERS_PER_CLIENT})"
printf "%10s | %12s | %14s\n" "clientes" "tiempo(s)" "ops/s aprox"
printf "%10s-+-%12s-+-%14s\n" "----------" "------------" "--------------"

for clients in "${CLIENTS_LIST[@]}"; do
  ./servidor_mq >/tmp/servidor_mq_extra.log 2>&1 &
  SERVER_PID=$!
  sleep 1

  start_ns="$(date +%s%N)"
  pids=()

  for i in $(seq 1 "$clients"); do
    ./app_cliente3_mq "extra_${clients}_${i}" "$ITERS_PER_CLIENT" &
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

echo "[extra] OK: pruebas de estrés completadas"
