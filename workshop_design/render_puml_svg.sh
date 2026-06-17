#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_FORMAT="${1:-svg}"
PLANTUML_SERVER="${PLANTUML_SERVER:-https://www.plantuml.com/plantuml}"

# Allow comma-separated formats (e.g. "svg,png")
IFS=',' read -r -a OUT_FORMATS <<< "${OUT_FORMAT}"
for _fmt in "${OUT_FORMATS[@]}"; do
  if [[ "${_fmt}" != "svg" && "${_fmt}" != "png" ]]; then
    echo "[ERR] Unsupported format: ${_fmt} (use svg or png or comma-separated list)" >&2
    exit 1
  fi
done

mapfile -t PUML_FILES < <(find "${ROOT_DIR}" -maxdepth 1 -type f -name '*.puml' | sort)
if [ ${#PUML_FILES[@]} -eq 0 ]; then
  echo "[INFO] No .puml files found in ${ROOT_DIR}"
  exit 0
fi

if command -v plantuml >/dev/null 2>&1; then
  echo "[INFO] Using local plantuml binary"
  for puml in "${PUML_FILES[@]}"; do
    for fmt in "${OUT_FORMATS[@]}"; do
      echo "[INFO] Rendering $(basename "${puml}") -> ${fmt}"
      plantuml -t"${fmt}" "${puml}"
    done
  done
  echo "[OK] Render complete"
  exit 0
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "[ERR] Neither plantuml nor python3 is available" >&2
  exit 1
fi

echo "[INFO] Local plantuml not found; using PlantUML server: ${PLANTUML_SERVER}"
python3 - "${ROOT_DIR}" "${OUT_FORMAT}" "${PLANTUML_SERVER}" <<'PY'
from pathlib import Path
from urllib.request import Request, urlopen
import sys

root = Path(sys.argv[1])
fmt_list = sys.argv[2].split(',')
server = sys.argv[3].rstrip('/')

pumls = sorted(root.glob('*.puml'))

for puml in pumls:
  text = puml.read_text(encoding='utf-8').strip()
  if not text:
    print(f"[WARN] Skipping empty file: {puml.name}")
    continue

  encoded = text.encode('utf-8').hex().upper()
  for fmt in fmt_list:
    headers = {
      'User-Agent': 'Mozilla/5.0',
      'Accept': 'image/svg+xml' if fmt == 'svg' else 'image/png',
    }
    url = f"{server}/{fmt}/~h{encoded}"
    req = Request(url, headers=headers)

    try:
      data = urlopen(req, timeout=120).read()
    except Exception as exc:
      print(f"[ERR] Failed to render {puml.name} ({fmt}): {exc}")
      raise

    out = puml.with_suffix(f'.{fmt}')
    out.write_bytes(data)
    print(f"[OK] Generated {out.name}")

print('[OK] Render complete')
PY
