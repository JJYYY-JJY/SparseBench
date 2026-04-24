#!/usr/bin/env bash
#SBATCH --job-name=sbpp-spmv-smoke
#SBATCH --account=stf
#SBATCH --partition=cpu-g2
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=64G
#SBATCH --time=00:30:00
#SBATCH --output=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.out
#SBATCH --error=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.err

set -euo pipefail

PROJECT_ROOT="${PROJECT_ROOT:-/gscratch/stf/${USER}/projects/SparseBench}"
RUNROOT="${RUNROOT:-/gscratch/scrubbed/${USER}/sparsebench}"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build}"
BIN="${BIN:-${BUILD_DIR}/sparsebench_spmv}"
MATRIX_LIST="${MATRIX_LIST:-${RUNROOT}/data/small_matrices.txt}"
OUT_DIR="${OUT_DIR:-${RUNROOT}/results/smoke_${SLURM_JOB_ID}}"

cd "${PROJECT_ROOT}"
mkdir -p "${OUT_DIR}"

echo "SparseBench++ CPU G2 smoke"
date
hostname
echo "Project root: ${PROJECT_ROOT}"
echo "Run root: ${RUNROOT}"
echo "Binary: ${BIN}"
echo "Matrix list: ${MATRIX_LIST}"
git rev-parse HEAD || true

if [[ ! -s "${MATRIX_LIST}" ]]; then
  echo "Matrix list is missing or empty: ${MATRIX_LIST}" >&2
  exit 3
fi

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j "${SLURM_CPUS_PER_TASK}"

export OMP_PROC_BIND=spread
export OMP_PLACES=cores

first_matrix="$(head -n 1 "${MATRIX_LIST}")"
for threads in 1 2 4 8; do
  export OMP_NUM_THREADS="${threads}"
  "${BIN}" "${first_matrix}" \
    --threads "${threads}" \
    --repeat 20 \
    --out "${OUT_DIR}/spmv_t${threads}.csv"
done

find "${OUT_DIR}" -maxdepth 1 -type f -print
date
