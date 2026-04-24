#!/usr/bin/env bash
set -euo pipefail

SPARSEBENCH_SCRATCH="${SPARSEBENCH_SCRATCH:-/gscratch/scrubbed/${USER}/sparsebench}"
DEST="${1:-${SPARSEBENCH_SCRATCH}/data/suitesparse_small}"
LIST_OUT="${2:-${SPARSEBENCH_SCRATCH}/data/small_matrices.txt}"

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
  curl -L -o "${archive}" "${url}"
  mkdir -p "${name}"
  tar -xzf "${archive}" -C "${name}" --strip-components=1 || tar -xzf "${archive}" -C "${name}"
done

tmp_list="$(mktemp)"
find "${DEST}" -name "*.mtx" | sort > "${tmp_list}"
: > "${LIST_OUT}"

while IFS= read -r matrix; do
  header="$(head -n 1 "${matrix}" | tr '[:upper:]' '[:lower:]')"
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
