#!/usr/bin/env bash
#SBATCH --job-name=sbpp-spmv-eigen
#SBATCH --account=stf
#SBATCH --partition=cpu-g2
#SBATCH --qos=stf-cpu-g2
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --mem=128G
#SBATCH --time=02:00:00
#SBATCH --output=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.out
#SBATCH --error=/gscratch/scrubbed/%u/sparsebench/logs/%x-%j.err

set -euo pipefail

module load cmake/3.25.1
module load cesg/eigen/3.3.9

PROJECT_ROOT="${PROJECT_ROOT:-/gscratch/stf/${USER}/projects/SparseBench}"
RUNROOT="${RUNROOT:-/gscratch/scrubbed/${USER}/sparsebench}"
BUILD_DIR="${BUILD_DIR:-${RUNROOT}/build_eigen_${SLURM_JOB_ID}}"
SPARSEBENCH_BIN="${SPARSEBENCH_BIN:-${BUILD_DIR}/sparsebench_spmv}"
EIGEN_BIN="${EIGEN_BIN:-${BUILD_DIR}/sparsebench_spmv_eigen}"
MATRIX_LIST="${MATRIX_LIST:-${RUNROOT}/data/medium_matrices.txt}"
OUT_DIR="${OUT_DIR:-${RUNROOT}/results/eigen_baseline_${SLURM_JOB_ID}}"
ANALYSIS_DIR="${ANALYSIS_DIR:-${RUNROOT}/analysis}"
THREADS=(1 2 4 8 16 32)
REPEAT="${REPEAT:-30}"
EXPECTED_CSV_COUNT=72
EXPECTED_MATRIX_COUNT=6

cd "${PROJECT_ROOT}"
mkdir -p "${OUT_DIR}" "${ANALYSIS_DIR}" "${RUNROOT}/logs"

module list
which cmake
cmake --version
which g++
g++ --version

echo "SparseBench++ Eigen baseline on CPU G2"
date
hostname
echo "Project root: ${PROJECT_ROOT}"
echo "Run root: ${RUNROOT}"
echo "Build dir: ${BUILD_DIR}"
echo "SparseBench binary: ${SPARSEBENCH_BIN}"
echo "Eigen binary: ${EIGEN_BIN}"
echo "Matrix list: ${MATRIX_LIST}"
echo "Output dir: ${OUT_DIR}"
echo "Analysis dir: ${ANALYSIS_DIR}"
echo "Repeat: ${REPEAT}"
echo "SLURM_CPUS_PER_TASK=${SLURM_CPUS_PER_TASK}"
git rev-parse HEAD || true

if [[ ! -s "${MATRIX_LIST}" ]]; then
  echo "Matrix list is missing or empty: ${MATRIX_LIST}" >&2
  exit 3
fi

matrix_count=0
while IFS= read -r matrix; do
  [[ -z "${matrix}" ]] && continue
  matrix_count=$((matrix_count + 1))

  if [[ ! -f "${matrix}" ]]; then
    echo "Matrix path is missing: ${matrix}" >&2
    exit 4
  fi

  if [[ "${matrix}" == *"/data/tiny/diag5.mtx" ]]; then
    echo "Refusing fallback diag5 matrix for Eigen baseline: ${matrix}" >&2
    exit 5
  fi

  header="$(head -n 1 "${matrix}" | tr -d '\r' | tr '[:upper:]' '[:lower:]')"
  case "${header}" in
    "%%matrixmarket matrix coordinate real general" | \
      "%%matrixmarket matrix coordinate integer general" | \
      "%%matrixmarket matrix coordinate pattern general" | \
      "%%matrixmarket matrix coordinate real symmetric")
      ;;
    *)
      echo "Unsupported Matrix Market header in ${matrix}: ${header}" >&2
      exit 6
      ;;
  esac
done < "${MATRIX_LIST}"

if (( matrix_count != EXPECTED_MATRIX_COUNT )); then
  echo "Expected ${EXPECTED_MATRIX_COUNT} matrices, found ${matrix_count}" >&2
  exit 7
fi

cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DSPARSEBENCH_USE_EIGEN=ON
cmake --build "${BUILD_DIR}" -j "${SLURM_CPUS_PER_TASK}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

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
    "${SPARSEBENCH_BIN}" "${matrix}" \
      --threads "${threads}" \
      --repeat "${REPEAT}" \
      --out "${OUT_DIR}/sparsebench_${mbase}_t${threads}.csv"
    "${EIGEN_BIN}" "${matrix}" \
      --threads "${threads}" \
      --repeat "${REPEAT}" \
      --out "${OUT_DIR}/eigen_${mbase}_t${threads}.csv"
  done
done < "${MATRIX_LIST}"

actual_csv_count="$(find "${OUT_DIR}" -maxdepth 1 -type f -name "*.csv" | wc -l)"
if [[ "${actual_csv_count}" != "${EXPECTED_CSV_COUNT}" ]]; then
  echo "Expected ${EXPECTED_CSV_COUNT} CSV files, found ${actual_csv_count}" >&2
  exit 8
fi

python3 scripts/analyze_spmv_baseline.py "${OUT_DIR}" \
  --out-dir "${ANALYSIS_DIR}" \
  --run-id "${SLURM_JOB_ID}" \
  --expected-repeat "${REPEAT}" \
  --expected-file-count "${EXPECTED_CSV_COUNT}"

find "${OUT_DIR}" -maxdepth 1 -type f -name "*.csv" | sort
ls -lh "${ANALYSIS_DIR}/eigen_baseline_${SLURM_JOB_ID}_summary."*
date
