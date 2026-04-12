#!/bin/bash
set -e

LOG_DIR="logs"
LOG_FILE="$LOG_DIR/server.log"
TEST_STATUS="failed"
TEST_START_EPOCH=$(date +%s)
RUN_ID=$(date '+%Y-%m-%d %H:%M:%S')

mkdir -p "$LOG_DIR"

echo "[INFO] Iniciando servidor..."
./webserv > "$LOG_FILE" 2>&1 &
SERVER_PID=$!

# 🔥 Cleanup automático pase lo que pase
cleanup() {
  echo "[INFO] Cerrando servidor..."
  kill -TERM $SERVER_PID 2>/dev/null || true
  wait $SERVER_PID 2>/dev/null || true
}

on_interrupt() {
  TEST_STATUS="interrupted"
  exit 130
}

on_exit() {
  cleanup

  local end_epoch end_time elapsed summary
  end_epoch=$(date +%s)
  end_time=$(date '+%Y-%m-%d %H:%M:%S')
  elapsed=$((end_epoch - TEST_START_EPOCH))

  if [ "$TEST_STATUS" = "completed" ]; then
    summary="[INFO] Testing completado correctamente a las ${end_time} en ${elapsed}s. Logs en: $LOG_FILE"
  elif [ "$TEST_STATUS" = "interrupted" ]; then
    summary="[WARN] Testing interrumpido por el usuario a las ${end_time} tras ${elapsed}s. Logs parciales en: $LOG_FILE"
  else
    summary="[ERROR] Testing finalizado con error a las ${end_time} tras ${elapsed}s. Logs parciales en: $LOG_FILE"
  fi

  echo "$summary"
  echo "[RUN-END] run='$RUN_ID' end='$end_time' status='$TEST_STATUS' elapsed='${elapsed}s'" >> "$LOG_FILE"
}

trap on_interrupt INT TERM
trap on_exit EXIT

echo "[RUN-START] run='$RUN_ID'" >> "$LOG_FILE"

# 🔍 Esperar a que el server esté listo (mejor que sleep)
echo "[INFO] Esperando a que el servidor esté disponible..."

for i in {1..10}; do
  if nc -z localhost 8080; then
    echo "[INFO] Servidor listo"
    break
  fi
  sleep 1
done

echo "[INFO] Iniciando tester..."
./tester http://localhost:8080

TEST_STATUS="completed"
