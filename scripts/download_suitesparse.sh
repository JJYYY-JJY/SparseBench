#!/usr/bin/env bash
set -euo pipefail

SPARSEBENCH_SCRATCH="${SPARSEBENCH_SCRATCH:-/gscratch/scrubbed/${USER}/sparsebench}"
DEST="${1:-${SPARSEBENCH_SCRATCH}/data/suitesparse_small}"
LIST_OUT="${2:-${SPARSEBENCH_SCRATCH}/data/small_matrices.txt}"

download_file() {
  local url="$1"
  local out="$2"

  rm -f "${out}"

  if command -v wget >/dev/null 2>&1; then
    if wget -O "${out}" "${url}"; then
      return 0
    fi
    echo "wget failed for ${url}; trying next downloader" >&2
    rm -f "${out}"
  fi

  if command -v curl >/dev/null 2>&1; then
    if env -u LD_LIBRARY_PATH curl -L -o "${out}" "${url}"; then
      return 0
    fi
    echo "curl failed for ${url}; trying next downloader" >&2
    rm -f "${out}"
  fi

  if command -v python3 >/dev/null 2>&1; then
    if python3 - "${url}" "${out}" <<'PY'
import sys
import urllib.request

url, out = sys.argv[1], sys.argv[2]
urllib.request.urlretrieve(url, out)
PY
    then
      return 0
    fi
    echo "python3 urllib failed for ${url}" >&2
    rm -f "${out}"
  fi

  echo "Failed to download ${url}; no working downloader found" >&2
  return 1
}

MATS=(
  "HB/1138_bus"
  "HB/494_bus"
  "HB/662_bus"
  "HB/ash958"
  "HB/bcspwr06"
)

mkdir -p "${DEST}"
mkdir -p "$(dirname "${LIST_OUT}")"
cd "${DEST}"

for item in "${MATS[@]}"; do
  group="${item%/*}"
  name="${item#*/}"
  url="https://sparse.tamu.edu/MM/${group}/${name}.tar.gz"
  archive="${name}.tar.gz"

  echo "Downloading ${url}"
  download_file "${url}" "${archive}"
  mkdir -p "${name}"
  tar -xzf "${archive}" -C "${name}" --strip-components=1 || tar -xzf "${archive}" -C "${name}"
done

tmp_list="$(mktemp)"
find "${DEST}" -name "*.mtx" | sort > "${tmp_list}"
: > "${LIST_OUT}"

while IFS= read -r matrix; do
  header="$(head -n 1 "${matrix}" | tr -d '\r' | tr '[:upper:]' '[:lower:]')"
  if [[ "${header}" == "%%matrixmarket matrix coordinate real general" ]]; then
    echo "${matrix}" >> "${LIST_OUT}"
  else
    echo "Skipping unsupported Matrix Market header in ${matrix}: ${header}" >&2
  fi
done < "${tmp_list}"
rm -f "${tmp_list}"

if [[ ! -s "${LIST_OUT}" ]]; then
  echo "No supported coordinate real general matrices were found." >&2
  exit 3
fi

echo "Matrix list written to ${LIST_OUT}"
