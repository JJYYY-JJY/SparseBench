#!/usr/bin/env bash
#SBATCH --job-name=sbpp-spmv-smoke
#SBATCH --account=stf
#SBATCH --partition=cpu-g2
#SBATCH --qos=stf-cpu-g2
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem=64G
#SBATCH --time=00:30:00
#SBATCH --output=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.out
#SBATCH --error=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.err

set -euo pipefail

module load cmake/3.25.1

PROJECT_ROOT="${PROJECT_ROOT:-/gscratch/stf/${USER}/projects/SparseBench}"
RUNROOT="${RUNROOT:-/gscratch/scrubbed/${USER}/sparsebench}"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build}"
BIN="${BIN:-${BUILD_DIR}/sparsebench_spmv}"
MATRIX_LIST="${MATRIX_LIST:-${RUNROOT}/data/small_matrices.txt}"
OUT_DIR="${OUT_DIR:-${RUNROOT}/results/smoke_${SLURM_JOB_ID}}"

cd "${PROJECT_ROOT}"
mkdir -p "${OUT_DIR}"

module list
which cmake
cmake --version
which g++
g++ --version

echo "SparseBench++ CPU G2 smoke"
date
hostname
echo "Project root: ${PROJECT_ROOT}"
echo "Run root: ${RUNROOT}"
echo "Build dir: ${BUILD_DIR}"
echo "Binary: ${BIN}"
echo "Matrix list: ${MATRIX_LIST}"
git rev-parse HEAD || true

if [[ ! -s "${MATRIX_LIST}" ]]; then
  echo "Matrix list is missing or empty: ${MATRIX_LIST}" >&2
  exit 3
fi

first_matrix="$(head -n 1 "${MATRIX_LIST}")"
if [[ -z "${first_matrix}" || ! -f "${first_matrix}" ]]; then
  echo "First matrix is missing: ${first_matrix}" >&2
  exit 4
fi

if [[ "${first_matrix}" == *"/data/tiny/diag5.mtx" ]]; then
  echo "Refusing fallback diag5 matrix for real SuiteSparse smoke: ${first_matrix}" >&2
  exit 5
fi

first_header="$(head -n 1 "${first_matrix}" | tr -d '\r' | tr '[:upper:]' '[:lower:]')"
case "${first_header}" in
  "%%matrixmarket matrix coordinate real general" | \
    "%%matrixmarket matrix coordinate integer general" | \
    "%%matrixmarket matrix coordinate pattern general" | \
    "%%matrixmarket matrix coordinate real symmetric")
    ;;
  *)
    echo "Unsupported Matrix Market header in ${first_matrix}: ${first_header}" >&2
    exit 6
    ;;
esac

echo "First matrix: ${first_matrix}"
echo "First matrix header: ${first_header}"

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j "${SLURM_CPUS_PER_TASK}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

export OMP_PROC_BIND=spread
export OMP_PLACES=cores

for threads in 1 2 4 8; do
  export OMP_NUM_THREADS="${threads}"
  "${BIN}" "${first_matrix}" \
    --threads "${threads}" \
    --repeat 20 \
    --out "${OUT_DIR}/spmv_t${threads}.csv"
done

find "${OUT_DIR}" -maxdepth 1 -type f -print
date
