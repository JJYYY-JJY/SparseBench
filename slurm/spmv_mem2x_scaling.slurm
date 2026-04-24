#!/usr/bin/env bash
#SBATCH --job-name=sbpp-spmv-mem2x
#SBATCH --account=stf
#SBATCH --partition=cpu-g2-mem2x
#SBATCH --qos=stf-cpu-g2-mem2x
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=96
#SBATCH --mem=512G
#SBATCH --time=04:00:00
#SBATCH --output=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.out
#SBATCH --error=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.err

set -euo pipefail

module load cmake/3.25.1

PROJECT_ROOT="${PROJECT_ROOT:-/gscratch/stf/${USER}/projects/SparseBench}"
RUNROOT="${RUNROOT:-/gscratch/scrubbed/${USER}/sparsebench}"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build}"
BIN="${BIN:-${BUILD_DIR}/sparsebench_spmv}"
MATRIX_LIST="${MATRIX_LIST:-${RUNROOT}/data/small_matrices.txt}"
OUT_DIR="${OUT_DIR:-${RUNROOT}/results/mem2x_${SLURM_JOB_ID}}"
THREADS=(1 2 4 8 16 32 64 96)

cd "${PROJECT_ROOT}"
mkdir -p "${OUT_DIR}"

echo "SparseBench++ CPU G2 mem2x scaling"
date
hostname
echo "Project root: ${PROJECT_ROOT}"
echo "Run root: ${RUNROOT}"
echo "Binary: ${BIN}"
echo "Matrix list: ${MATRIX_LIST}"
echo "SLURM_CPUS_PER_TASK=${SLURM_CPUS_PER_TASK}"
git rev-parse HEAD || true

if [[ ! -s "${MATRIX_LIST}" ]]; then
  echo "Matrix list is missing or empty: ${MATRIX_LIST}" >&2
  exit 3
fi

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j "${SLURM_CPUS_PER_TASK}"

export OMP_PROC_BIND=spread
export OMP_PLACES=cores

while IFS= read -r matrix; do
  [[ -z "${matrix}" ]] && continue
  mbase="$(basename "${matrix}" .mtx)"
  echo "=== MATRIX ${mbase} ==="

  for threads in "${THREADS[@]}"; do
    if (( threads > SLURM_CPUS_PER_TASK )); then
      continue
    fi

    export OMP_NUM_THREADS="${threads}"
    "${BIN}" "${matrix}" \
      --threads "${threads}" \
      --repeat 30 \
      --out "${OUT_DIR}/${mbase}_t${threads}.csv"
  done
done < "${MATRIX_LIST}"

find "${OUT_DIR}" -type f -name "*.csv" | sort
date
