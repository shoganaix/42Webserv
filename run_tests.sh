#!/bin/bash
set -e
set -o pipefail

LOG_DIR="logs"
LOG_FILE="$LOG_DIR/server.log"
RUN_LOG_FILE="$LOG_DIR/run_tests.log"
TEST_STATE_FILE="$LOG_DIR/current-test.state"
MAX_TEST_SECONDS="${MAX_TEST_SECONDS:-0}"
TEST_START_EPOCH=$(date +%s)
RUN_ID=$(date '+%Y-%m-%d %H:%M:%S')
TEST_STATUS="failed"
CURRENT_TEST="(waiting for tester)"
CURRENT_TEST_START_EPOCH=$TEST_START_EPOCH
CURRENT_TEST_ACTIVE=0
TESTS_STARTED=0
TESTS_COMPLETED=0
LONGEST_TEST=""
LONGEST_TEST_ELAPSED=0
HEAVY_TEST="Test multiple workers(20) doing multiple times(5): Post on /directory/youpi.bla with size 100000000"
HEAVY_TEST_ELAPSED=-1
HEARTBEAT_PID=""
SERVER_PID=""
TESTER_PID=""
MAIN_PID=$$

mkdir -p "$LOG_DIR"
: > "$RUN_LOG_FILE"

log_run() {
  printf '%s\n' "$1" >> "$RUN_LOG_FILE"
  printf '%s\n' "$1" >> "$LOG_FILE"
}

close_current_test() {
  if [ "$CURRENT_TEST_ACTIVE" -ne 1 ]; then
    return
  fi

  local now elapsed
  now=$(date +%s)
  elapsed=$((now - CURRENT_TEST_START_EPOCH))

  log_run "[RUN-TEST-END] run=\"$RUN_ID\" test=\"$CURRENT_TEST\" elapsed=\"${elapsed}s\""

  TESTS_COMPLETED=$((TESTS_COMPLETED + 1))
  if [ "$elapsed" -gt "$LONGEST_TEST_ELAPSED" ]; then
    LONGEST_TEST_ELAPSED=$elapsed
    LONGEST_TEST="$CURRENT_TEST"
  fi

  if [ "$CURRENT_TEST" = "$HEAVY_TEST" ]; then
    HEAVY_TEST_ELAPSED=$elapsed
  fi

  CURRENT_TEST_ACTIVE=0
}

echo "[INFO] Iniciando servidor..."
./webserv > "$LOG_FILE" 2>&1 &
SERVER_PID=$!

cleanup() {
  echo "[INFO] Cerrando servidor..."
  if [ -n "$HEARTBEAT_PID" ]; then
    kill "$HEARTBEAT_PID" 2>/dev/null || true
    wait "$HEARTBEAT_PID" 2>/dev/null || true
  fi
  kill -TERM "$SERVER_PID" 2>/dev/null || true
  wait "$SERVER_PID" 2>/dev/null || true
}

on_interrupt() {
  TEST_STATUS="interrupted"
  exit 130
}

on_exit() {
  if [ -f "$TEST_STATE_FILE" ]; then
    IFS='|' read -r CURRENT_TEST CURRENT_TEST_START_EPOCH < "$TEST_STATE_FILE" || true
  fi

  close_current_test

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
  log_run "[RUN-SUMMARY] run='$RUN_ID' tests_started='$TESTS_STARTED' tests_completed='$TESTS_COMPLETED' longest_test='$LONGEST_TEST' longest_test_elapsed='${LONGEST_TEST_ELAPSED}s' heavy_test_elapsed='${HEAVY_TEST_ELAPSED}s'"
  log_run "[RUN-END] run='$RUN_ID' end='$end_time' status='$TEST_STATUS' elapsed='${elapsed}s' last_test='$CURRENT_TEST' last_test_elapsed='$(( end_epoch - CURRENT_TEST_START_EPOCH ))s'"
  rm -f "$TEST_STATE_FILE" 2>/dev/null || true
}

trap on_interrupt INT TERM
trap on_exit EXIT

log_run "[RUN-START] run='$RUN_ID'"

echo "[INFO] Esperando a que el servidor esté disponible..."
for i in {1..10}; do
  if nc -z localhost 8080; then
    echo "[INFO] Servidor listo"
    break
  fi
  sleep 1
done

echo "[INFO] Iniciando tester..."
log_run "[RUN-TESTER-START] run='$RUN_ID' tester='./tester http://localhost:8080'"
printf '%s|%s\n' "$CURRENT_TEST" "$CURRENT_TEST_START_EPOCH" > "$TEST_STATE_FILE"

heartbeat_loop() {
  local last_cgi_done=0
  local last_progress_epoch=$TEST_START_EPOCH

  while true; do
    sleep 60
    local heartbeat_now test_elapsed total_elapsed cgi_done no_progress
    heartbeat_now=$(date +%s)
    if [ -f "$TEST_STATE_FILE" ]; then
      IFS='|' read -r CURRENT_TEST CURRENT_TEST_START_EPOCH < "$TEST_STATE_FILE" || true
    fi

    cgi_done=$(grep -a -c "\[CGI-DONE\]" "$LOG_FILE" 2>/dev/null || true)
    case "$cgi_done" in
      ''|*[!0-9]*) cgi_done=0 ;;
    esac
    if [ "$cgi_done" -gt "$last_cgi_done" ]; then
      last_cgi_done=$cgi_done
      last_progress_epoch=$heartbeat_now
    fi

    test_elapsed=$((heartbeat_now - CURRENT_TEST_START_EPOCH))
    total_elapsed=$((heartbeat_now - TEST_START_EPOCH))
    no_progress=$((heartbeat_now - last_progress_epoch))
    log_run "[RUN-HEARTBEAT] run=\"$RUN_ID\" test=\"$CURRENT_TEST\" test_elapsed=\"${test_elapsed}s\" total_elapsed=\"${total_elapsed}s\" cgi_done=\"$cgi_done\" no_progress=\"${no_progress}s\""

    if [ "$no_progress" -ge 900 ]; then
      log_run "[RUN-STALL-SUSPECT] run=\"$RUN_ID\" test=\"$CURRENT_TEST\" no_progress=\"${no_progress}s\""
      ps -o pid,stat,etime,pcpu,pmem,cmd -p "$SERVER_PID" >> "$LOG_FILE" 2>/dev/null || true
      if [ -n "$TESTER_PID" ]; then
        ps -o pid,stat,etime,pcpu,pmem,cmd -p "$TESTER_PID" >> "$LOG_FILE" 2>/dev/null || true
      fi
    fi

    if [ "$MAX_TEST_SECONDS" -gt 0 ] && [ "$total_elapsed" -ge "$MAX_TEST_SECONDS" ]; then
      log_run "[RUN-TIMEOUT] run=\"$RUN_ID\" max_total=\"${MAX_TEST_SECONDS}s\" last_test=\"$CURRENT_TEST\""
      kill -TERM "$MAIN_PID" 2>/dev/null || true
      return
    fi
  done
}

heartbeat_loop &
HEARTBEAT_PID=$!

TESTER_CMD=(./tester http://localhost:8080)
if command -v stdbuf >/dev/null 2>&1; then
  TESTER_CMD=(stdbuf -oL -eL ./tester http://localhost:8080)
fi

exec 3< <("${TESTER_CMD[@]}" 2>&1)
TESTER_PID=$!

while IFS= read -r raw_line <&3 || [ -n "$raw_line" ]; do
  line=$(printf '%s' "$raw_line" | tr -d '\r')
  printf '%s\n' "$line"
  printf '%s\n' "$line" >> "$LOG_FILE"

  case "$line" in
    *Test\ *)
      close_current_test
      CURRENT_TEST="Test ${line#*Test }"
      CURRENT_TEST_START_EPOCH=$(date +%s)
      CURRENT_TEST_ACTIVE=1
      TESTS_STARTED=$((TESTS_STARTED + 1))
      printf '%s|%s\n' "$CURRENT_TEST" "$CURRENT_TEST_START_EPOCH" > "$TEST_STATE_FILE"
      log_run "[RUN-TEST-START] run=\"$RUN_ID\" test=\"$CURRENT_TEST\" start=\"$(date '+%Y-%m-%d %H:%M:%S')\""
      ;;
  esac
done

close_current_test

if wait "$TESTER_PID"; then
  TESTER_STATUS=0
else
  TESTER_STATUS=$?
fi

if [ -n "$HEARTBEAT_PID" ]; then
  kill "$HEARTBEAT_PID" 2>/dev/null || true
  wait "$HEARTBEAT_PID" 2>/dev/null || true
  HEARTBEAT_PID=""
fi

if [ "$TESTER_STATUS" -eq 0 ]; then
  TEST_STATUS="completed"
else
  TEST_STATUS="failed"
fi

log_run "[RUN-TESTER-END] run='$RUN_ID' status='$TEST_STATUS' last_test='$CURRENT_TEST' last_test_elapsed='$(( $(date +%s) - CURRENT_TEST_START_EPOCH ))s'"

if [ "$TESTER_STATUS" -ne 0 ]; then
  exit "$TESTER_STATUS"
fi
