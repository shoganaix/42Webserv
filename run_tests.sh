#!/bin/bash
set -e

LOG_DIR="logs"
LOG_FILE="$LOG_DIR/server.log"

mkdir -p $LOG_DIR

echo "[INFO] Iniciando servidor..."
./webserv > "$LOG_FILE" 2>&1 &
SERVER_PID=$!

# 🔥 Cleanup automático pase lo que pase
cleanup() {
  echo "[INFO] Cerrando servidor..."
  kill -TERM $SERVER_PID 2>/dev/null || true
  wait $SERVER_PID 2>/dev/null || true
}
trap cleanup EXIT

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

echo "[INFO] Tester completado. Logs en: $LOG_FILE"
