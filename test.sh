#!/usr/bin/env bash
# Pruebas basicas de humo para el servidor. Requiere que "make" ya haya
# generado el binario ./server.
set -uo pipefail

cd "$(dirname "$0")"

PORT=8099
FAILED=0

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "$SERVER_PID" 2>/dev/null
    wait "$SERVER_PID" 2>/dev/null
  fi
}
trap cleanup EXIT

PORT=$PORT ./server >/tmp/server_test.log 2>&1 &
SERVER_PID=$!

for _ in $(seq 1 20); do
  if curl -s -o /dev/null "http://localhost:$PORT/"; then
    break
  fi
  sleep 0.1
done

check_status() {
  local desc="$1"
  local expected="$2"
  local actual="$3"
  if [[ "$actual" == "$expected" ]]; then
    echo "PASS: $desc (status $actual)"
  else
    echo "FAIL: $desc (esperado $expected, obtuve $actual)"
    FAILED=1
  fi
}

status=$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:$PORT/")
check_status "GET / sirve index.html" 200 "$status"

status=$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:$PORT/index.html")
check_status "GET /index.html" 200 "$status"

status=$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:$PORT/noexiste")
check_status "GET a un archivo inexistente" 404 "$status"

content_type=$(curl -s -D - -o /dev/null "http://localhost:$PORT/index.html" | grep -i '^Content-Type:')
if echo "$content_type" | grep -qi 'text/html'; then
  echo "PASS: Content-Type de index.html es text/html"
else
  echo "FAIL: Content-Type inesperado ($content_type)"
  FAILED=1
fi

# curl normaliza "/../" en la URL antes de mandarla, asi que probamos el
# traversal armando el request HTTP a mano. Apuntamos a /etc/passwd, que
# existe fuera de la raiz del servidor, para confirmar que de verdad se
# bloquea el escape (subir un solo nivel cae en Desktop/, donde no hay
# nada que probar).
response=$(exec 3<>"/dev/tcp/localhost/$PORT"
  printf 'GET /../../../../../../etc/passwd HTTP/1.1\r\nHost: localhost\r\n\r\n' >&3
  timeout 1 cat <&3
  exec 3<&-)
if echo "$response" | grep -qE "HTTP/1\.1 40[03]"; then
  echo "PASS: path traversal bloqueado"
else
  echo "FAIL: path traversal no bloqueado"
  echo "$response"
  FAILED=1
fi

if [[ "$FAILED" -eq 0 ]]; then
  echo "Todos los tests pasaron."
else
  echo "Algunos tests fallaron."
fi

exit "$FAILED"
