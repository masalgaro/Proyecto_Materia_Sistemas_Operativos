#! /usr/bin/env bash
# Este script inicia todos los servicios creados, ejecuta comandos generales desde el ciente y limpia al finaliar.
# Su propósito es asegurar que la comunicación funciona de forma correcta. A la vez que dar un ejemplo de ejecución

set -euo pipefail

# Configuración Inicial

BUILD="${1:-./build}"
ARALMAC="/tmp/aralmac-prueba"

TUB_CLIENTE="/tmp/t-cliente"
TUB_CLIENTE_RET="/tmp/t-cliente-retorno"
TUB_GESFICH="/tmp/t-gesfich"
TUB_GESFICH_RET="/tmp/t-gesfich-retorno"
TUB_GESPROG="/tmp/t-gesprog"
TUB_GESPROG_RET="/tmp/t-gesprog-retorno"
TUB_EJECUTOR="/tmp/t-ejecutor"
TUB_EJECUTOR_RET="/tmp/t-ejecutor-retorno"

TODAS_TUBERIAS=(
  "$TUB_CLIENTE" "$TUB_CLIENTE_RET"
  "$TUB_GESFICH" "$TUB_GESFICH_RET"
  "$TUB_GESPROG" "$TUB_GESPROG_RET"
  "$TUB_EJECUTOR" "$TUB_EJECUTOR_RET"
)

# PIDs de cada servicio
PIDS=()

# Utilidades
log()  { echo -e "\033[1;34m[PRUEBA]\033[0m $*"; }
ok()   { echo -e "\033[1;32m[  OK  ]\033[0m $*"; }
err()  { echo -e "\033[1;31m[ERROR ]\033[0m $*" >&2; }
 
limpiar() {
  log "Limpiando procesos y tuberías..."
  for pid in "${PIDS[@]}"; do
    kill "$pid" 2>/dev/null || true
  done
  for t in "${TODAS_TUBERIAS[@]}"; do
    rm -f "$t"
  done
  rm -rf "$ARALMAC"
  log "Listo."
}

trap limpiar EXIT # Limpiar si se sale por cualquier motivo

verificar_ejecutable() {
  local bin="$BUILD/$1"
  if [[ ! -x "$bin" ]]; then
    err "No se encontró el ejecutable: $bin"
    err "Asegúrate de haber compilado con: cmake --build build"
    exit 1
  fi
}

# VERIFICAR EJECUTABLES
log "Verificando ejecutables en '$BUILD'..."
for prog in ctrllt gesfich gesprog ejecutor cliente; do
  verificar_ejecutable "$prog"
done
ok "Todos los ejecutables encontrados."

# CREACION TUBERIAS
log "Creando tuberías nombradas..."
for t in "${TODAS_TUBERIAS[@]}"; do
  rm -f "$t"
  mkfifo "$t"
done
mkdir -p "$ARALMAC"
ok "Tuberías creadas."

# INICIO SERVICIOS
log "Iniciando gesfich..."
"$BUILD/gesfich" -f "$TUB_GESFICH" -b "$TUB_GESFICH_RET" -x "$ARALMAC" &
PIDS+=($!)
 
log "Iniciando gesprog..."
"$BUILD/gesprog" -p "$TUB_GESPROG" -c "$TUB_GESPROG_RET" -x "$ARALMAC" &
PIDS+=($!)
 
log "Iniciando ejecutor..."
"$BUILD/ejecutor" -e "$TUB_EJECUTOR" -d "$TUB_EJECUTOR_RET" -x "$ARALMAC" &
PIDS+=($!)
 
log "Iniciando ctrllt..."
"$BUILD/ctrllt" \
  -c "$TUB_CLIENTE"     -a "$TUB_CLIENTE_RET" \
  -f "$TUB_GESFICH"     -b "$TUB_GESFICH_RET" \
  -p "$TUB_GESPROG"     -q "$TUB_GESPROG_RET" \
  -e "$TUB_EJECUTOR"    -d "$TUB_EJECUTOR_RET" &
PIDS+=($!)

sleep 2 # sleep 1 deberia ser suficiente pero sleep 2 da garantía de que ctrllt siempre esté listo antes de que el cliente inicie
ok "Servicios iniciados (PIDs: ${PIDS[*]})."

# COMANDOS DE PRUEBA
log "Iniciando cliente con secuencia de prueba..."
echo ""
 
"$BUILD/cliente" -c "$TUB_CLIENTE" -a "$TUB_CLIENTE_RET" <<'COMANDOS'
crear fichero
crear fichero
crear programa
leer
leer f-0001
leer p-0001
suspender
resumir
terminar
COMANDOS
 
echo ""
ok "Secuencia de prueba completada."

# NOTAS FINALES
# Como se puede observar, la forma de llamada de los procesos es con la ruta de las tuberias explicitas.
# Es decir, la ejecución es de la forma "ctrllt -c /tmp/nombre-tuberia-cliente"
# Además, la tuberia debe existir con anterioridad.
