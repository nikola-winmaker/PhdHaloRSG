#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_FORMAT="${1:-svg}"
PLANTUML_SERVER="${PLANTUML_SERVER:-https://www.plantuml.com/plantuml}"

if [[ "${OUT_FORMAT}" != "svg" && "${OUT_FORMAT}" != "png" ]]; then
  echo "[ERR] Unsupported format: ${OUT_FORMAT} (use svg or png)" >&2
  exit 1
fi

mapfile -t PUML_FILES < <(find "${ROOT_DIR}" -maxdepth 1 -type f -name '*.puml' | sort)
if [ ${#PUML_FILES[@]} -eq 0 ]; then
  echo "[INFO] No .puml files found in ${ROOT_DIR}"
  exit 0
fi

if command -v plantuml >/dev/null 2>&1; then
  echo "[INFO] Using local plantuml binary"
  for puml in "${PUML_FILES[@]}"; do
    echo "[INFO] Rendering $(basename "${puml}")"
    plantuml -t"${OUT_FORMAT}" "${puml}"
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
fmt = sys.argv[2]
server = sys.argv[3].rstrip('/')

pumls = sorted(root.glob('*.puml'))
headers = {
    'User-Agent': 'Mozilla/5.0',
    'Accept': 'image/svg+xml,image/png,text/html,*/*;q=0.8',
}

for puml in pumls:
    text = puml.read_text(encoding='utf-8').strip()
    if not text:
        print(f"[WARN] Skipping empty file: {puml.name}")
        continue

    encoded = text.encode('utf-8').hex().upper()
    url = f"{server}/{fmt}/~h{encoded}"
    req = Request(url, headers=headers)

    try:
        data = urlopen(req, timeout=120).read()
    except Exception as exc:
        print(f"[ERR] Failed to render {puml.name}: {exc}")
        raise

    out = puml.with_suffix(f'.{fmt}')
    out.write_bytes(data)
    print(f"[OK] Generated {out.name}")

print('[OK] Render complete')
PY
