#!/usr/bin/env bash
set -euo pipefail

SPARSEBENCH_SCRATCH="${SPARSEBENCH_SCRATCH:-/gscratch/scrubbed/${USER}/sparsebench}"
SPARSEBENCH_MATRIX_SET="${SPARSEBENCH_MATRIX_SET:-small}"

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

case "${SPARSEBENCH_MATRIX_SET}" in
  small)
    DEFAULT_DEST="${SPARSEBENCH_SCRATCH}/data/suitesparse_small"
    DEFAULT_LIST_OUT="${SPARSEBENCH_SCRATCH}/data/small_matrices.txt"
    MIN_SUPPORTED=1
    MATS=(
      "HB/1138_bus"
      "HB/494_bus"
      "HB/662_bus"
      "HB/ash958"
      "HB/bcspwr06"
    )
    ;;
  medium)
    DEFAULT_DEST="${SPARSEBENCH_SCRATCH}/data/suitesparse_medium"
    DEFAULT_LIST_OUT="${SPARSEBENCH_SCRATCH}/data/medium_matrices.txt"
    MIN_SUPPORTED=5
    MATS=(
      "Williams/mac_econ_fwd500"
      "Williams/mc2depi"
      "Williams/cop20k_A"
      "Williams/cant"
      "Williams/consph"
      "Williams/pdb1HYS"
      "Pothen/pwt"
      "Pothen/skirt"
      "HB/bcsstk31"
      "HB/bcsstk32"
    )
    ;;
  *)
    echo "Unknown SPARSEBENCH_MATRIX_SET=${SPARSEBENCH_MATRIX_SET}; expected small or medium." >&2
    exit 2
    ;;
esac

DEST="${1:-${DEFAULT_DEST}}"
LIST_OUT="${2:-${DEFAULT_LIST_OUT}}"

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
accepted_count=0

while IFS= read -r matrix; do
  header="$(head -n 1 "${matrix}" | tr -d '\r' | tr '[:upper:]' '[:lower:]')"
  case "${header}" in
    "%%matrixmarket matrix coordinate real general" | \
      "%%matrixmarket matrix coordinate integer general" | \
      "%%matrixmarket matrix coordinate pattern general" | \
      "%%matrixmarket matrix coordinate real symmetric")
      echo "${matrix}" >> "${LIST_OUT}"
      accepted_count=$((accepted_count + 1))
      ;;
    *)
      echo "Skipping unsupported Matrix Market header in ${matrix}: ${header}" >&2
      ;;
  esac
done < "${tmp_list}"
rm -f "${tmp_list}"

if (( accepted_count < MIN_SUPPORTED )); then
  echo "Only ${accepted_count} parser-supported Matrix Market matrices were found for ${SPARSEBENCH_MATRIX_SET}; need at least ${MIN_SUPPORTED}." >&2
  exit 3
fi

echo "Matrix set: ${SPARSEBENCH_MATRIX_SET}"
echo "Matrix list written to ${LIST_OUT}"
echo "Parser-supported matrix count: ${accepted_count}"
