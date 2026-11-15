#!/usr/bin/env bash
# Compile le firmware, et l'envoie sur la carte si on le demande.
#   ./build.sh            compile seulement
#   ./build.sh auto       compile puis envoie sur la carte branchée en USB (port trouvé tout seul)
#   ./build.sh COM5       compile puis envoie sur le port indiqué
#   ./build.sh monitor    affiche les messages de la carte (Ctrl+C pour quitter)
set -e
cd "$(dirname "$0")"
CLI=tools/arduino-cli/arduino-cli.exe
CFG=tools/arduino-cli.yaml
FQBN="esp32:esp32:esp32c6:FlashSize=16M,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=cdc"

findPort() {
  # Les puces Espressif se présentent en USB avec l'identifiant VID_303A
  powershell.exe -NoProfile -Command "(Get-CimInstance Win32_PnPEntity | Where-Object { \$_.DeviceID -match 'VID_303A' -and \$_.Name -match 'COM\d+' } | Select-Object -First 1).Name" \
    | tr -d '\r' | grep -o 'COM[0-9]*' || true
}

PORT="$1"
if [ "$PORT" = "monitor" ]; then
  P=$(findPort); [ -z "$P" ] && { echo "Carte introuvable : vérifie le câble USB-C (un câble de données, pas seulement de charge)."; exit 1; }
  exec "$CLI" --config-file "$CFG" monitor -p "$P" -c baudrate=115200
fi
if [ "$PORT" = "auto" ]; then
  PORT=$(findPort)
  [ -z "$PORT" ] && { echo "Carte introuvable : vérifie le câble USB-C (un câble de données, pas seulement de charge)."; exit 1; }
  echo "Carte trouvée sur $PORT"
fi

"$CLI" --config-file "$CFG" compile --fqbn "$FQBN" --output-dir build firmware/thermo_horloge
if [ -n "$PORT" ]; then
  "$CLI" --config-file "$CFG" upload --fqbn "$FQBN" --port "$PORT" --input-dir build firmware/thermo_horloge
  echo "Envoyé. Pour voir les messages de la carte : ./build.sh monitor"
fi
