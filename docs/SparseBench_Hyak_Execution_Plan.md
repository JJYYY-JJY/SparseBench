# SparseBench Hyak Execution Plan

**Document purpose.** This document records the operational plan for taking `SparseBench` from the completed `cpu-g2` smoke milestone to real SuiteSparse benchmarks and then to `cpu-g2-mem2x` scaling. It is designed to be committed to the repo as a traceable project/runbook document.

**Suggested repo path.** `docs/hyak_execution_plan.md`

**Current status as of 2026-04-25.** SuiteSparse ingestion, parser v0.1.1, real `cpu-g2` smoke, smoke evidence packaging, and the 32-core `cpu-g2-mem2x` pilot have passed on Hyak. The real smoke job was `34802647` at commit `820cba9fb10dc4f579b872a3cf93b5d7529982ea`; CTest passed `3/3`, `.err` was empty, real SuiteSparse CSVs were produced, and the smoke evidence archive/checksum were written under `/gscratch/scrubbed/junyej/sparsebench/`. The `mem2x` pilot was job `34803037` at commit `5dc9ef099d752f581a947a6bb9f6c1153e90c952`; Slurm completed it with `COMPLETED 0:0`, CTest passed `3/3`, `.err` was empty, 24 real SuiteSparse CSVs covered 4 matrices and thread counts `1,2,4,8,16,32`, and the mem2x evidence archive/checksum were written under `/gscratch/scrubbed/junyej/sparsebench/`. The medium manifest at `/gscratch/scrubbed/junyej/sparsebench/data/medium_matrices.txt` has been generated and validated with 6 parser-supported matrices. The 96-core medium `cpu-g2-mem2x` job `34825519` completed at commit `2fc974c0a4a0eea087f8e86dd5f54fe626992729` with `COMPLETED 0:0`, CTest `3/3`, empty stderr, and 48 CSVs covering 6 matrices across thread counts `1,2,4,8,16,32,64,96`; its package, checksum, and analysis summary are under `/gscratch/scrubbed/junyej/sparsebench/`. The 192-core Phase 8 job `34851174` has been submitted from a scrubbed-side Slurm script and is currently `PENDING` with reason `QOSGrpCpuLimit`; there are no 192-core result CSVs yet.

---

## 1. Scope and non-goals

### 1.1 Scope

This document covers:

1. Synchronizing the successful Hyak smoke-script commit back to GitHub.
2. Fixing or bypassing the SuiteSparse downloader failure.
3. Producing a real SuiteSparse small-matrix manifest.
4. Running a real-matrix `cpu-g2` smoke job.
5. Running a conservative `cpu-g2-mem2x` pilot.
6. Running a full `cpu-g2-mem2x` scaling benchmark.
7. Packaging results, downloading them locally, plotting figures, and updating README/resume material.

### 1.2 Non-goals for the immediate next pass

Do **not** do these before the 192-core Phase 8 job completes and its logs/CSVs are reviewed:

- Do not claim final 192-thread/full `cpu-g2-mem2x` results while job `34851174` is pending.
- Do not add CG, GMRES, Lanczos, or other algorithms.
- Do not run performance conclusions from `diag5.mtx` fallback data.
- Do not treat `hyakalloc-all` global idle resources as guaranteed availability for this account.

---

## 2. Operational facts and evidence so far

### 2.1 Completed milestone: `cpu-g2` tiny fallback smoke

The following milestone has been completed:

- Smoke job: `34800602`.
- Partition: `cpu-g2`.
- Job script: `slurm/spmv_smoke_cpu_g2.slurm`.
- Result directory: `/gscratch/scrubbed/junyej/sparsebench/results/smoke_34800602/`.
- Expected CSV outputs were produced:
  - `spmv_t1.csv`
  - `spmv_t2.csv`
  - `spmv_t4.csv`
  - `spmv_t8.csv`
- `.err` was empty.
- `.out` showed `CTest` passed: `3/3 tests passed`.
- No `GLIBCXX`, `libstdc++`, or `libgomp` runtime-linker errors appeared.
- Each CSV used `diag5.mtx`, `repeat=20`, and `checksum=15`.

### 2.2 Meaning of this milestone

This confirms:

- CMake and GCC module setup works in a Slurm batch job.
- OpenMP binary execution works under the module-loaded runtime environment.
- CTest can run inside the batch job.
- Slurm output/error paths work.
- CSV result emission works.
- The repository can be built and tested on Hyak `cpu-g2`.

This does **not** confirm:

- SuiteSparse downloader correctness.
- Matrix Market parser coverage on real SuiteSparse matrices.
- Meaningful SpMV performance.
- `cpu-g2-mem2x` scheduling or scaling behavior.

### 2.3 Current Git state from Hyak

A Hyak-local commit was created:

```text
704731d Configure Hyak Slurm smoke jobs
```

The commit includes:

- `slurm/spmv_smoke_cpu_g2.slurm`
  - added `#SBATCH --qos=stf-cpu-g2`
  - added `module load cmake/3.25.1`
  - added module/compiler diagnostics
  - added build-dir logging
  - added `ctest --test-dir "${BUILD_DIR}" --output-on-failure`
- `slurm/spmv_mem2x_scaling.slurm`
  - added `#SBATCH --qos=stf-cpu-g2-mem2x`
  - added `module load cmake/3.25.1`
  - kept `#SBATCH --partition=cpu-g2-mem2x`
  - kept `#SBATCH --nodes=1`
- `.gitignore`
  - added `.codex/`

`git push` from Hyak failed because `git-remote-https` hit the same OpenSSL/Kerberos symbol issue as `curl`.

---

## 3. External constraints and source-backed rules

### 3.1 Login-node rule

Use the Hyak login node only for lightweight operations such as file inspection, directory creation, text edits, `bash -n`, `git status`, and Slurm submission. Do not compile or run benchmark binaries on the login node. Hyak provides Slurm so users can access compute nodes rather than running research workloads on the login node.

### 3.2 Module rule

Load `cmake/3.25.1` only inside a compute-node allocation or batch job. Hyak modules are intended for compute-node use, and this project has already shown that `cmake/3.25.1` brings in the GCC runtime needed by the compiled binary.

### 3.3 Slurm directive rule

All `#SBATCH` directives must appear before the first non-comment, non-whitespace shell command. Do not put `#SBATCH` lines after `set -euo pipefail`, variable assignments, `module load`, or any other executable line.

### 3.4 Matrix Market parser rule

The parser supports a limited sparse Matrix Market subset. Real-matrix gates may accept only:

```text
%%MatrixMarket matrix coordinate real general
%%MatrixMarket matrix coordinate integer general
%%MatrixMarket matrix coordinate pattern general
%%MatrixMarket matrix coordinate real symmetric
```

Other Matrix Market variants must either be skipped or supported explicitly in a later parser milestone. `complex`, Hermitian, skew-symmetric, pattern-symmetric, integer-symmetric, and array/dense files remain unsupported.

### 3.5 SuiteSparse rule

SuiteSparse Matrix Collection is a valid benchmark source for this project, but the manifest must contain real `.mtx` files that the current parser can ingest. A fallback manifest pointing to `diag5.mtx` is acceptable only for Slurm mechanics testing.

---

## 4. Directory and path conventions

### 4.1 Repo path on Hyak

```bash
PROJECT_ROOT="/gscratch/stf/${USER}/projects/SparseBench"
```

### 4.2 Runtime scratch path

```bash
RUNROOT="/gscratch/scrubbed/${USER}/sparsebench"
```

Expected subdirectories:

```bash
mkdir -p "${RUNROOT}/data" "${RUNROOT}/results" "${RUNROOT}/logs" "${RUNROOT}/patches"
```

### 4.3 Important generated files

```text
${RUNROOT}/data/small_matrices.txt
${RUNROOT}/data/medium_matrices.txt
${RUNROOT}/results/pre_sbatch_*.csv
${RUNROOT}/results/smoke_<jobid>/*.csv
${RUNROOT}/results/mem2x_<jobid>/*.csv
${RUNROOT}/logs/*<jobid>*.out
${RUNROOT}/logs/*<jobid>*.err
```

---

## 5. Phase 1 — Sync Hyak commit back to GitHub

### 5.1 Goal

Move Hyak commit `704731d` into the canonical GitHub repo without relying on broken Hyak HTTPS Git.

### 5.2 Preferred route: export patch from Hyak, apply on Windows

On Hyak:

```bash
cd /gscratch/stf/$USER/projects/SparseBench

git status -sb
git log --oneline -3

mkdir -p /gscratch/scrubbed/$USER/sparsebench/patches

git format-patch origin/main..HEAD \
  -o /gscratch/scrubbed/$USER/sparsebench/patches

ls -lh /gscratch/scrubbed/$USER/sparsebench/patches
```

On Windows PowerShell:

```powershell
mkdir E:\SparseBench\patches -Force

scp junyej@klone.hyak.uw.edu:/gscratch/scrubbed/junyej/sparsebench/patches/*.patch E:\SparseBench\patches\

cd E:\SparseBench

git status
git pull --ff-only
git am E:\SparseBench\patches\*.patch

git log --oneline -3
git push
```

### 5.3 Optional cleanup: amend author before patch export

Only do this if you want the commit author to match your local/GitHub identity:

```bash
git config user.name "Junye Ji"
git config user.email "<your-github-verified-email>"
git commit --amend --reset-author --no-edit
```

Then run `git format-patch` again.

### 5.4 Acceptance criteria

- `git log --oneline -3` on Windows shows `Configure Hyak Slurm smoke jobs`.
- GitHub contains the Slurm script modifications.
- Hyak and Windows repos agree after pull/push.

### 5.5 Stop conditions

- `git am` conflicts.
- Windows local repo has uncommitted changes that would be overwritten.
- GitHub push fails due to remote divergence.

If any stop condition occurs, inspect:

```powershell
git status
git diff
git log --oneline --graph --decorate --all -10
```

---

## 6. Phase 2 — Fix or bypass SuiteSparse downloader

### 6.1 Goal

Generate a real SuiteSparse manifest:

```text
/gscratch/scrubbed/$USER/sparsebench/data/small_matrices.txt
```

with at least one, preferably 3–10, real `.mtx` files that the parser can read.

### 6.2 Current failure mode

Both `curl` and `git-remote-https` have shown:

```text
/lib64/libk5crypto.so.3: undefined symbol: EVP_KDF_ctrl, version OPENSSL_1_1_1b
```

Treat this as an environment/runtime-library issue, not a SparseBench C++ issue.

### 6.3 Test clean downloader environment

Run on Hyak, preferably outside the module-loaded build environment:

```bash
cd /gscratch/stf/$USER/projects/SparseBench

module purge || true

unset LD_LIBRARY_PATH
unset LIBRARY_PATH
unset CPATH
unset C_INCLUDE_PATH
unset CPLUS_INCLUDE_PATH
unset PKG_CONFIG_PATH

hash -r

which curl || true
curl --version || true
ldd "$(which curl)" | egrep 'ssl|crypto|krb5|k5crypto' || true
```

Then test:

```bash
bash scripts/download_suitesparse.sh
```

### 6.4 Fallback downloader order

Modify `scripts/download_suitesparse.sh` to prefer `wget`, then clean `curl`, then Python `urllib`:

```bash
download_file() {
  local url="$1"
  local out="$2"

  if command -v wget >/dev/null 2>&1; then
    wget -O "$out" "$url"
  elif command -v curl >/dev/null 2>&1; then
    env -u LD_LIBRARY_PATH curl -L -o "$out" "$url"
  elif command -v python3 >/dev/null 2>&1; then
    python3 - "$url" "$out" <<'PY'
import sys
import urllib.request

url, out = sys.argv[1], sys.argv[2]
urllib.request.urlretrieve(url, out)
PY
  else
    echo "No downloader available: wget/curl/python3" >&2
    return 1
  fi
}
```

Then replace direct `curl` calls with:

```bash
download_file "$url" "${name}.tar.gz"
```

### 6.4.1 Implementation record

Local implementation status:

- `scripts/download_suitesparse.sh` now uses `download_file URL OUT`.
- Downloader fallback order is `wget`, then `env -u LD_LIBRARY_PATH curl`, then `python3 urllib`.
- Failed downloader attempts remove the partial output before trying the next downloader.
- `SPARSEBENCH_MATRIX_SET=small` remains the default and writes `${SPARSEBENCH_SCRATCH}/data/small_matrices.txt`, defaulting to `/gscratch/scrubbed/$USER/sparsebench/data/small_matrices.txt`.
- `SPARSEBENCH_MATRIX_SET=medium` writes `${SPARSEBENCH_SCRATCH}/data/medium_matrices.txt`, defaulting to `/gscratch/scrubbed/$USER/sparsebench/data/medium_matrices.txt`.
- Manifest filtering accepts the parser v0.1.1 header set: `coordinate real general`, `coordinate integer general`, `coordinate pattern general`, and `coordinate real symmetric`, with carriage returns stripped before header comparison.
- Medium mode fails if fewer than 5 parser-supported matrices remain after filtering.

Validation to record after execution:

```text
bash -n scripts/download_suitesparse.sh: passed
clean downloader command: env -u LD_LIBRARY_PATH -u LIBRARY_PATH -u CPATH -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH -u PKG_CONFIG_PATH SPARSEBENCH_SCRATCH=/gscratch/scrubbed/$USER/sparsebench bash scripts/download_suitesparse.sh
download result: wget succeeded for 5 SuiteSparse tarballs
manifest path: /gscratch/scrubbed/junyej/sparsebench/data/small_matrices.txt
matrix count: 0 under the v0.1 coordinate-real-general filter
sample headers:
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/1138_bus/1138_bus.mtx: %%MatrixMarket matrix coordinate real symmetric
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/494_bus/494_bus.mtx: %%MatrixMarket matrix coordinate real symmetric
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/662_bus/662_bus.mtx: %%MatrixMarket matrix coordinate real symmetric
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/ash958/ash958.mtx: %%MatrixMarket matrix coordinate pattern general
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/bcspwr06/bcspwr06.mtx: %%MatrixMarket matrix coordinate pattern symmetric
download errors, if any: none after escaping the sandbox write restriction; script exited 3 because no v0.1-supported headers remained
next blocker: parser v0.1.1
```

### 6.5 Last resort: download locally, transfer to Hyak

If Hyak downloader remains broken, download small Matrix Market tarballs on Windows and transfer them:

```powershell
scp E:\SparseBench\data\suitesparse_small\*.tar.gz junyej@klone.hyak.uw.edu:/gscratch/scrubbed/junyej/sparsebench/data/
```

On Hyak:

```bash
mkdir -p /gscratch/scrubbed/$USER/sparsebench/data/suitesparse_small
cd /gscratch/scrubbed/$USER/sparsebench/data/suitesparse_small

for f in /gscratch/scrubbed/$USER/sparsebench/data/*.tar.gz; do
  name="$(basename "$f" .tar.gz)"
  mkdir -p "$name"
  tar -xzf "$f" -C "$name" --strip-components=1 || tar -xzf "$f" -C "$name"
done

find /gscratch/scrubbed/$USER/sparsebench/data/suitesparse_small \
  -name "*.mtx" \
  | sort \
  > /gscratch/scrubbed/$USER/sparsebench/data/small_matrices.txt
```

### 6.6 Acceptance criteria

- `small_matrices.txt` exists and is nonempty.
- Each path in the manifest points to an existing `.mtx` file.
- At least one matrix header is currently supported.

Validation:

```bash
manifest=/gscratch/scrubbed/$USER/sparsebench/data/small_matrices.txt

test -s "$manifest"
wc -l "$manifest"

while IFS= read -r m; do
  [ -f "$m" ] || { echo "Missing file: $m"; exit 1; }
  echo "== $m =="
  head -n 1 "$m"
done < "$manifest"
```

### 6.7 Medium matrix-set workflow

The 96-core `cpu-g2-mem2x` medium run must use a separately generated medium manifest instead of reusing the 4-entry small manifest from the 32-core pilot. Generate it with:

```bash
cd /gscratch/stf/$USER/projects/SparseBench

env -u LD_LIBRARY_PATH -u LIBRARY_PATH -u CPATH \
  -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH -u PKG_CONFIG_PATH \
  SPARSEBENCH_SCRATCH=/gscratch/scrubbed/$USER/sparsebench \
  SPARSEBENCH_MATRIX_SET=medium \
  bash scripts/download_suitesparse.sh
```

The fixed candidate list is:

```text
Williams/mac_econ_fwd500
Williams/mc2depi
Williams/cop20k_A
Williams/cant
Williams/consph
Williams/pdb1HYS
Pothen/pwt
Pothen/skirt
HB/bcsstk31
HB/bcsstk32
```

Medium manifest path:

```text
/gscratch/scrubbed/junyej/sparsebench/data/medium_matrices.txt
```

Medium manifest acceptance:

- `medium_matrices.txt` exists and has 5-10 entries.
- Every entry points to an existing `.mtx` file.
- Every header is in the parser-supported header set.
- No `data/tiny/diag5.mtx` path appears.

---

## 7. Phase 3 — Parser coverage decision

### 7.1 Goal

Decide whether to run only `coordinate real general` matrices or expand the parser before real benchmarks.

### 7.2 Supported-manifest gate

The manifest filter must include only parser-supported headers:

```bash
manifest=/gscratch/scrubbed/$USER/sparsebench/data/small_matrices.txt
supported=/gscratch/scrubbed/$USER/sparsebench/data/supported_small_matrices.txt

: > "$supported"

while IFS= read -r m; do
  [ -f "$m" ] || continue
  header="$(head -n 1 "$m" | tr -d '\r')"
  case "$header" in
    "%%MatrixMarket matrix coordinate real general" | \
      "%%MatrixMarket matrix coordinate integer general" | \
      "%%MatrixMarket matrix coordinate pattern general" | \
      "%%MatrixMarket matrix coordinate real symmetric")
      echo "$m" >> "$supported"
      ;;
  esac
done < "$manifest"

mv "$supported" "$manifest"
cat "$manifest"
```

### 7.3 Parser v0.1.1 implementation

The downloader validation triggered parser v0.1.1 because the initial SuiteSparse set contained zero `coordinate real general` matrices. Parser v0.1.1 expands Matrix Market support to:

```text
coordinate integer general    -> parse values as double
coordinate pattern general    -> implicit value = 1.0
coordinate real symmetric     -> insert mirrored entry (j, i, value) when i != j
```

Continue rejecting:

```text
complex
Hermitian
skew-symmetric
array / dense Matrix Market format
```

Implementation record:

- `src/matrix_market.cpp` accepts `coordinate integer general` values as doubles.
- `src/matrix_market.cpp` accepts `coordinate pattern general` with implicit value `1.0`.
- `src/matrix_market.cpp` accepts `coordinate real symmetric` and inserts mirrored off-diagonal entries.
- The parser validates declared file-entry count before symmetric expansion changes CSR `nnz`.
- The downloader manifest filter now accepts the same parser-supported header set and still skips `pattern symmetric`.
- Tiny fixtures were added for `integer5.mtx`, `pattern5.mtx`, and `symmetric5.mtx`.
- Tests cover the new accepted formats and continued rejection of `complex`, Hermitian, skew-symmetric, and array/dense headers.

Manifest regeneration record:

```text
bash -n scripts/download_suitesparse.sh: passed
manifest path: /gscratch/scrubbed/junyej/sparsebench/data/small_matrices.txt
matrix count after parser v0.1.1 filter: 4
accepted:
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/1138_bus/1138_bus.mtx
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/494_bus/494_bus.mtx
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/662_bus/662_bus.mtx
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/ash958/ash958.mtx
skipped:
  /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/bcspwr06/bcspwr06.mtx: %%MatrixMarket matrix coordinate pattern symmetric
CTest/build status: pending compute-node smoke job
```

### 7.4 Tests added

Tiny fixtures:

```text
data/tiny/integer5.mtx
data/tiny/pattern5.mtx
data/tiny/symmetric5.mtx
```

CTest cases verify:

- expected `nrows`, `ncols`, `nnz`
- expected checksum for SpMV with deterministic `x`
- duplicate-coordinate behavior remains additive
- unsupported headers fail with a clear error

### 7.5 Acceptance criteria

- `ctest --test-dir build --output-on-failure` passes.
- Manifest contains at least one real SuiteSparse matrix supported by parser.
- Unsupported formats fail clearly rather than silently producing wrong output.

---

## 8. Phase 4 — Real SuiteSparse `cpu-g2` smoke

### 8.1 Goal

Run the existing `cpu-g2` smoke path against a real SuiteSparse matrix, not `diag5.mtx`.

### 8.2 Interactive precheck

Request a small allocation:

```bash
salloc \
  --account=stf \
  --partition=cpu-g2 \
  --qos=stf-cpu-g2 \
  --nodes=1 \
  --ntasks=1 \
  --cpus-per-task=8 \
  --mem=64G \
  --time=01:00:00
```

Inside the allocation:

```bash
hostname
echo "$SLURM_JOB_ID"
echo "$SLURM_CPUS_PER_TASK"
```

Stop if `hostname` is still `klone-login*`.

Build and test:

```bash
cd /gscratch/stf/$USER/projects/SparseBench

module load cmake/3.25.1
module list
which cmake
cmake --version
which g++
g++ --version

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$SLURM_CPUS_PER_TASK"
ctest --test-dir build --output-on-failure
```

Run one real matrix manually:

```bash
manifest=/gscratch/scrubbed/$USER/sparsebench/data/small_matrices.txt
first_matrix="$(head -n 1 "$manifest")"

echo "$first_matrix"
head -n 5 "$first_matrix"

./build/sparsebench_spmv "$first_matrix" \
  --threads 1 \
  --repeat 10 \
  --out /gscratch/scrubbed/$USER/sparsebench/results/pre_sbatch_real_matrix_t1.csv

./build/sparsebench_spmv "$first_matrix" \
  --threads 8 \
  --repeat 10 \
  --out /gscratch/scrubbed/$USER/sparsebench/results/pre_sbatch_real_matrix_t8.csv
```

### 8.3 Batch smoke

From a shell where `sbatch` is available:

```bash
cd /gscratch/stf/$USER/projects/SparseBench

bash -n slurm/spmv_smoke_cpu_g2.slurm
sbatch --test-only slurm/spmv_smoke_cpu_g2.slurm

jid=$(sbatch slurm/spmv_smoke_cpu_g2.slurm | awk '{print $4}')
echo "$jid"
squeue -j "$jid"
```

Smoke-script validation record:

- `slurm/spmv_smoke_cpu_g2.slurm` validates that the first manifest entry exists before building.
- The smoke script refuses the `data/tiny/diag5.mtx` fallback for real SuiteSparse smoke.
- The smoke script validates the first matrix header against the parser-supported header set before running benchmarks.

After completion:

```bash
ls -lh /gscratch/scrubbed/$USER/sparsebench/logs/*${jid}*

tail -n 120 /gscratch/scrubbed/$USER/sparsebench/logs/*${jid}*.out
tail -n 120 /gscratch/scrubbed/$USER/sparsebench/logs/*${jid}*.err

find /gscratch/scrubbed/$USER/sparsebench/results/smoke_${jid} \
  -maxdepth 1 \
  -type f \
  -name "*.csv" \
  -print
```

### 8.4 Acceptance criteria

- Manifest is not `diag5.mtx` fallback.
- CTest passes.
- `threads=1,2,4,8` CSV files are produced.
- `.err` has no runtime-linker errors.
- CSV checksum is finite and non-NaN.
- CSV includes matrix dimensions and `nnz` for a real matrix.

Execution record:

```text
job id: 34802647
commit: 820cba9fb10dc4f579b872a3cf93b5d7529982ea
prechecks:
  bash -n slurm/spmv_smoke_cpu_g2.slurm: passed
  sbatch --test-only slurm/spmv_smoke_cpu_g2.slurm: accepted as test job 34802646
first matrix: /gscratch/scrubbed/junyej/sparsebench/data/suitesparse_small/1138_bus/1138_bus.mtx
first matrix header: %%MatrixMarket matrix coordinate real symmetric
logs:
  /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-smoke-34802647.out
  /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-smoke-34802647.err
result directory: /gscratch/scrubbed/junyej/sparsebench/results/smoke_34802647/
CSV outputs:
  spmv_t1.csv
  spmv_t2.csv
  spmv_t4.csv
  spmv_t8.csv
CSV fact: 1138_bus, nrows=1138, ncols=1138, nnz=4054, repeat=20, checksum=1460.0402679
CTest: 3/3 tests passed
.err status: empty
runtime-linker errors: none observed
Phase 4 acceptance: passed
```

---

## 9. Phase 5 — Package and download smoke evidence

### 9.1 Goal

Preserve smoke evidence outside `/gscratch/scrubbed`.

### 9.2 Package on Hyak

```bash
RUNID=<cpu_g2_real_smoke_job_id>
PACK=/gscratch/scrubbed/$USER/sparsebench/package_${RUNID}

mkdir -p "$PACK"
mkdir -p "$PACK/logs"

cp -r /gscratch/scrubbed/$USER/sparsebench/results/smoke_${RUNID} "$PACK/results" 2>/dev/null || true
cp /gscratch/scrubbed/$USER/sparsebench/logs/*${RUNID}* "$PACK/logs/" 2>/dev/null || true
cp -r slurm "$PACK/slurm"
cp -r scripts "$PACK/scripts"

git rev-parse HEAD > "$PACK/git_commit.txt"

{
  date
  hostname
  uname -a
  module list 2>&1 || true
  cmake --version 2>/dev/null || true
  g++ --version 2>/dev/null || true
} > "$PACK/environment.txt"

cd /gscratch/scrubbed/$USER/sparsebench

tar -czf sparsebench_smoke_${RUNID}.tar.gz package_${RUNID}
sha256sum sparsebench_smoke_${RUNID}.tar.gz > sparsebench_smoke_${RUNID}.sha256
```

Package record:

```text
RUNID: 34802647
package directory: /gscratch/scrubbed/junyej/sparsebench/package_34802647
archive: /gscratch/scrubbed/junyej/sparsebench/sparsebench_smoke_34802647.tar.gz
archive size: 4.2K
sha256: 78e348f785bf4982ede4d591f4d1f55be9dd34d7be09db0fbbeb68c65d4633bf
checksum file: /gscratch/scrubbed/junyej/sparsebench/sparsebench_smoke_34802647.sha256
packaged commit: 820cba9fb10dc4f579b872a3cf93b5d7529982ea
packaged contents: logs, results, slurm, scripts, git_commit.txt, environment.txt
```

### 9.3 Download on Windows

```powershell
mkdir E:\SparseBench\hyak_runs -Force
cd E:\SparseBench\hyak_runs

scp junyej@klone.hyak.uw.edu:/gscratch/scrubbed/junyej/sparsebench/sparsebench_smoke_<RUNID>.tar.gz .
scp junyej@klone.hyak.uw.edu:/gscratch/scrubbed/junyej/sparsebench/sparsebench_smoke_<RUNID>.sha256 .
```

---

## 10. Phase 6 — `cpu-g2-mem2x` pilot

### 10.1 Goal

Verify that `cpu-g2-mem2x` scheduling, QOS, module stack, CTest, and OpenMP thread scaling work on a small real workload.

### 10.2 Pre-run checklist

The 32-core pilot started only after these were true:

- Hyak `main`, local `origin/main`, and GitHub `main` were synchronized at `5dc9ef099d752f581a947a6bb9f6c1153e90c952`.
- SuiteSparse manifest contains at least one real supported matrix.
- Real SuiteSparse `cpu-g2` smoke has completed successfully.
- `slurm/spmv_mem2x_scaling.slurm` passes `bash -n`.
- `sbatch --test-only slurm/spmv_mem2x_scaling.slurm` passes.
- Manifest is not the `diag5.mtx` fallback.
- `slurm/spmv_mem2x_scaling.slurm` validates every manifest entry before build.
- `slurm/spmv_mem2x_scaling.slurm` runs `ctest --test-dir "${BUILD_DIR}" --output-on-failure` after build.

### 10.3 Recommended pilot resource request

Use command-line `sbatch` overrides for the first pilot instead of changing the tracked medium-run defaults in `slurm/spmv_mem2x_scaling.slurm`:

```bash
sbatch --cpus-per-task=32 --mem=128G --time=01:00:00 slurm/spmv_mem2x_scaling.slurm
```

The checked-in script keeps the medium-run defaults:

```bash
#SBATCH --cpus-per-task=96
#SBATCH --mem=512G
#SBATCH --time=04:00:00
THREADS=(1 2 4 8 16 32 64 96)
```

For the 32-core pilot, the script skips `64` and `96` because they exceed `SLURM_CPUS_PER_TASK=32`, so the effective thread list is:

```bash
1 2 4 8 16 32
```

The pilot used all 4 current manifest entries.

### 10.4 Submission

```bash
cd /gscratch/stf/$USER/projects/SparseBench

bash -n slurm/spmv_mem2x_scaling.slurm
sbatch --test-only --cpus-per-task=32 --mem=128G --time=01:00:00 slurm/spmv_mem2x_scaling.slurm

jid=$(sbatch --cpus-per-task=32 --mem=128G --time=01:00:00 slurm/spmv_mem2x_scaling.slurm | awk '{print $4}')
echo "$jid"
squeue -j "$jid"
```

Precheck record:

```text
bash -n slurm/spmv_mem2x_scaling.slurm: passed
sbatch --test-only --cpus-per-task=32 --mem=128G --time=01:00:00 slurm/spmv_mem2x_scaling.slurm: accepted as test job 34803035
test-only placement: node n3443, partition cpu-g2-mem2x, 32 processors
real pilot submission: submitted as job 34803037
```

### 10.5 Acceptance criteria

- Job completes.
- CTest passes.
- CSVs are produced for `1,2,4,8,16,32` threads.
- No `GLIBCXX`, `libstdc++`, `libgomp`, OpenSSL, or Kerberos runtime-linker errors appear.
- Results use real SuiteSparse matrices.

Execution record:

```text
job id: 34803037
commit printed by job: 5dc9ef099d752f581a947a6bb9f6c1153e90c952
partition: cpu-g2-mem2x
qos: stf-cpu-g2-mem2x
requested resources: 32 CPUs, 128G, 01:00:00
Slurm state: COMPLETED
Slurm exit code: 0:0
node: n3445
start: 2026-04-24T02:31:34
end: 2026-04-24T02:33:14
elapsed: 00:01:40
stdout: /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-mem2x-34803037.out
stderr: /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-mem2x-34803037.err
CTest: 3/3 tests passed
.err status: empty
runtime-linker errors: none observed
result directory: /gscratch/scrubbed/junyej/sparsebench/results/mem2x_34803037/
CSV count: 24
matrices: 1138_bus, 494_bus, 662_bus, ash958
thread coverage per matrix: 1,2,4,8,16,32
repeat: 30
CSV numeric status: finite checksum and positive timing/rate fields observed
Phase 6 acceptance: passed
```

---

## 11. Phase 7 — `cpu-g2-mem2x` medium scaling

### 11.1 Goal

Produce preliminary performance results and catch scaling pathologies before the 192-thread run.

### 11.2 Recommended resource request

```bash
#SBATCH --cpus-per-task=96
#SBATCH --mem=512G
#SBATCH --time=04:00:00
```

Thread list:

```bash
THREADS=(1 2 4 8 16 32 64 96)
```

Matrix set:

```text
5–10 medium SuiteSparse matrices from /gscratch/scrubbed/junyej/sparsebench/data/medium_matrices.txt
```

Do not reuse the 4-entry small manifest from the 32-core pilot for 96-core performance conclusions. The small manifest is useful evidence that the Slurm/build/parser/result path works, but its largest matrix is still too small for the next scaling gate.

### 11.2.1 Submission command

Use the checked-in 96-core defaults in `slurm/spmv_mem2x_scaling.slurm` and point `MATRIX_LIST` at the medium manifest:

```bash
cd /gscratch/stf/$USER/projects/SparseBench

MATRIX_LIST=/gscratch/scrubbed/$USER/sparsebench/data/medium_matrices.txt

bash -n slurm/spmv_mem2x_scaling.slurm
sbatch --test-only \
  --export=ALL,MATRIX_LIST="$MATRIX_LIST" \
  slurm/spmv_mem2x_scaling.slurm

jid=$(sbatch \
  --export=ALL,MATRIX_LIST=/gscratch/scrubbed/$USER/sparsebench/data/medium_matrices.txt \
  slurm/spmv_mem2x_scaling.slurm | awk '{print $4}')

echo "$jid"
squeue -j "$jid"
```

Expected coverage:

```text
threads: 1,2,4,8,16,32,64,96
CSV count: matrix_count * 8
result directory: /gscratch/scrubbed/$USER/sparsebench/results/mem2x_${jid}
```

Medium manifest, execution, analysis, and package record:

```text
medium-manifest workflow commit: f06137fc6166194286b9c1fd491bf17a8f5a9c4c
manifest path: /gscratch/scrubbed/junyej/sparsebench/data/medium_matrices.txt
manifest count: 6
manifest validation: passed path/header/count/diag5 checks
accepted matrices: cant, consph, cop20k_A, mac_econ_fwd500, mc2depi, pdb1HYS
bash -n scripts/download_suitesparse.sh: passed
bash -n slurm/spmv_mem2x_scaling.slurm: passed
git diff --check: passed
sbatch --test-only --export=ALL,MATRIX_LIST=/gscratch/scrubbed/junyej/sparsebench/data/medium_matrices.txt slurm/spmv_mem2x_scaling.slurm: accepted as test job 34825338
test-only placement: node n3445, partition cpu-g2-mem2x, 96 processors
real 96-core submission: submitted as job 34825519
requested resources: 96 CPUs, 512G, 04:00:00
submit time: 2026-04-24T16:23:51
start time: 2026-04-25T03:30:12
end time: 2026-04-25T03:31:14
elapsed: 00:01:02
node: n3443
Slurm state: COMPLETED
Slurm exit code: 0:0
commit printed by job: 2fc974c0a4a0eea087f8e86dd5f54fe626992729
stdout: /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-mem2x-34825519.out
stderr: /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-mem2x-34825519.err
CTest: 3/3 tests passed
.err status: empty
result directory: /gscratch/scrubbed/junyej/sparsebench/results/mem2x_34825519/
CSV count: 48
matrices: cant, consph, cop20k_A, mac_econ_fwd500, mc2depi, pdb1HYS
thread coverage per matrix: 1,2,4,8,16,32,64,96
repeat: 30
CSV validation: every file has the expected header, one data row, repeat=30, positive median_ms/min_ms/nnz_per_sec, and finite checksum
analysis CSV: /gscratch/scrubbed/junyej/sparsebench/analysis/mem2x_34825519_summary.csv
analysis Markdown: /gscratch/scrubbed/junyej/sparsebench/analysis/mem2x_34825519_summary.md
best observed speedup range: 1.693x to 2.123x
speedup at 96 threads: 1.514x to 2.121x
scaling interpretation: valid benchmark signal, but CSR SpMV is memory-bandwidth limited; matrices that peak before 96 threads are signal, not failure
package directory: /gscratch/scrubbed/junyej/sparsebench/package_mem2x_34825519
archive: /gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_34825519.tar.gz
archive size: 7.5K
sha256: de90447e85738344e2479ac9b8c7147b1090ab8562331b48465e7911736f9c59
checksum file: /gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_34825519.sha256
packaged contents: logs, results, slurm, scripts, git_commit.txt, environment.txt
Phase 7 acceptance: passed
```

### 11.3 Measurements

Each CSV row should contain at least:

```text
matrix
nrows
ncols
nnz
threads
repeat
median_ms
min_ms
nnz_per_sec
checksum
```

Derived metrics to compute later:

```text
speedup = median_ms_at_1_thread / median_ms_at_t_threads
efficiency = speedup / threads
```

### 11.4 Acceptance criteria

- Results show plausible runtime changes across thread counts.
- No timing rows are dominated by `0 ms` or invalid measurements.
- CSV format is compatible with plotting scripts.
- At least one matrix shows nontrivial thread scaling.
- Slurm completes with `COMPLETED 0:0`.
- CTest reports `3/3 tests passed`.
- `.err` is empty or contains no runtime-linker failures.
- CSV coverage is exactly `matrix_count * 8` files with thread counts `1,2,4,8,16,32,64,96` for every matrix.
- Every CSV row has `repeat=30`, positive `median_ms`, positive `min_ms`, positive `nnz_per_sec`, and finite non-NaN checksum.

Phase 7 acceptance record:

```text
job 34825519 passed all Phase 7 acceptance criteria.
The resulting speedups are modest, as expected for memory-bandwidth-limited CSR SpMV, but the run is mechanically valid and sufficient to advance to the 192-core scheduling/scaling probe.
```

---

## 12. Phase 8 — Full `cpu-g2-mem2x` 192-thread scaling

### 12.1 Goal

Produce final benchmark figures suitable for README and resume claims.

### 12.2 Recommended resource request

```bash
#SBATCH --partition=cpu-g2-mem2x
#SBATCH --qos=stf-cpu-g2-mem2x
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=192
#SBATCH --mem=1500G
#SBATCH --time=08:00:00
```

Use `--nodes=1` because this benchmark is single-process OpenMP shared-memory.

### 12.3 Thread list

```bash
THREADS=(1 2 4 8 16 32 64 96 128 192)
```

### 12.4 Matrix set

Recommended categories:

```text
small:   1e5–1e6 nnz, to observe overhead and timing noise
medium:  1e6–1e8 nnz, main benchmark category
large:   >1e8 nnz, optional, add only after medium run is stable
```

### 12.5 Repeat policy

```text
small matrices:   repeat 50 or more
medium matrices:  repeat 20–30
large matrices:   repeat 5–10
```

### 12.6 Acceptance criteria

- Job completes or produces enough partial CSVs to analyze.
- Logs contain environment diagnostics and commit hash.
- Results cover all planned thread counts for at least 5 real matrices.
- No fallback matrix is included in the official benchmark manifest.
- CSVs are packaged and downloaded after completion.

### 12.7 Submission record

The first Phase 8 run reuses the validated 6-entry medium manifest for a controlled 96-to-192 comparison. Generated script and job artifacts stay under `/gscratch/scrubbed/junyej/sparsebench` because `/mmfs1/gscratch/stf` is 99% full.

```text
scrubbed status note: /gscratch/scrubbed/junyej/sparsebench/phase8_status.md
scrubbed Slurm script: /gscratch/scrubbed/junyej/sparsebench/slurm/spmv_mem2x_192_20260425.slurm
script source: copied from slurm/spmv_mem2x_scaling.slurm and changed only Phase 8 parameters
requested resources: 192 CPUs, 1500G, 08:00:00
manifest: /gscratch/scrubbed/junyej/sparsebench/data/medium_matrices.txt
manifest validation: passed path/header/count/diag5 checks
thread list: 1,2,4,8,16,32,64,96,128,192
expected CSV count if completed: 6 matrices x 10 thread counts = 60
bash -n /gscratch/scrubbed/junyej/sparsebench/slurm/spmv_mem2x_192_20260425.slurm: passed
sbatch --test-only /gscratch/scrubbed/junyej/sparsebench/slurm/spmv_mem2x_192_20260425.slurm: accepted as test job 34851157
test-only placement: node n3443, partition cpu-g2-mem2x, 192 processors
real 192-core submission: submitted as job 34851174
duplicate cleanup: accidental duplicate job 34851657 was cancelled; keep only 34851174
Slurm state at latest check: PENDING
Slurm reason at latest check: QOSGrpCpuLimit
submit time: 2026-04-25T18:41:58
start time: Unknown
stdout path once started: /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-mem2x-34851174.out
stderr path once started: /gscratch/scrubbed/junyej/sparsebench/logs/sbpp-spmv-mem2x-34851174.err
expected result directory once started: /gscratch/scrubbed/junyej/sparsebench/results/mem2x_34851174/
current blocker: scheduler QOS CPU limit; no 192-core logs, CSVs, package, or benchmark conclusions yet
```

---

## 13. Phase 9 — Package, download, and plot mem2x results

### 13.1 Package on Hyak

```bash
RUNID=<mem2x_job_id>
PACK=/gscratch/scrubbed/$USER/sparsebench/package_mem2x_${RUNID}

mkdir -p "$PACK"
mkdir -p "$PACK/logs"

cp -r /gscratch/scrubbed/$USER/sparsebench/results/mem2x_${RUNID} "$PACK/results"
cp /gscratch/scrubbed/$USER/sparsebench/logs/*${RUNID}* "$PACK/logs/" 2>/dev/null || true
cp -r slurm "$PACK/slurm"
cp -r scripts "$PACK/scripts"

git rev-parse HEAD > "$PACK/git_commit.txt"

{
  date
  hostname
  uname -a
  module list 2>&1 || true
  cmake --version 2>/dev/null || true
  g++ --version 2>/dev/null || true
  hyakalloc 2>/dev/null || true
} > "$PACK/environment.txt"

cd /gscratch/scrubbed/$USER/sparsebench

tar -czf sparsebench_mem2x_${RUNID}.tar.gz package_mem2x_${RUNID}
sha256sum sparsebench_mem2x_${RUNID}.tar.gz > sparsebench_mem2x_${RUNID}.sha256
```

Pilot package record:

```text
RUNID: 34803037
package directory: /gscratch/scrubbed/junyej/sparsebench/package_mem2x_34803037
archive: /gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_34803037.tar.gz
archive size: 6.0K
sha256: 16bf0aa5594f65172fc2b51909633db387f4bb1de919aed5ae6ce757c3be4e33
checksum file: /gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_34803037.sha256
packaged commit: 5dc9ef099d752f581a947a6bb9f6c1153e90c952
packaged contents: logs, results, slurm, scripts, git_commit.txt, environment.txt
```

Medium package record:

```text
RUNID: 34825519
package directory: /gscratch/scrubbed/junyej/sparsebench/package_mem2x_34825519
archive: /gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_34825519.tar.gz
archive size: 7.5K
sha256: de90447e85738344e2479ac9b8c7147b1090ab8562331b48465e7911736f9c59
checksum file: /gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_34825519.sha256
packaged commit: 2fc974c0a4a0eea087f8e86dd5f54fe626992729
packaged contents: logs, results, slurm, scripts, git_commit.txt, environment.txt
package verification: sha256sum -c passed; archive contains 48 result CSVs plus logs/scripts/slurm/environment metadata
analysis files:
  /gscratch/scrubbed/junyej/sparsebench/analysis/mem2x_34825519_summary.csv
  /gscratch/scrubbed/junyej/sparsebench/analysis/mem2x_34825519_summary.md
```

### 13.2 Download on Windows

```powershell
cd E:\SparseBench\hyak_runs

scp junyej@klone.hyak.uw.edu:/gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_<RUNID>.tar.gz .
scp junyej@klone.hyak.uw.edu:/gscratch/scrubbed/junyej/sparsebench/sparsebench_mem2x_<RUNID>.sha256 .
```

### 13.3 Plot requirements

Generate at least these plots:

```text
runtime vs threads
speedup vs threads
parallel efficiency vs threads
nnz/sec vs threads
nnz/sec by matrix
```

At least four representative matrices should appear in the README:

```text
1 small matrix
1 medium matrix
1 matrix with poor scaling
1 matrix with relatively good scaling
```

---

## 14. README update plan

Add a `Benchmarks` section with:

```text
Hardware / partition / account / module stack
Git commit hash
Matrix list and source
Parser-supported Matrix Market variants
Command lines
Thread list
Result summary table
Figures
Limitations
```

Example limitation text:

```text
SparseBench++ v0.1 measures CSR SpMV only. The current implementation is designed as a reproducible CPU-only benchmark harness rather than a fully optimized sparse linear algebra library. Performance depends strongly on matrix sparsity pattern, row-length imbalance, memory bandwidth, and OpenMP scheduling.
```

---

## 15. Resume bullets

Before full mem2x run:

```text
Built SparseBench++, a CPU-only sparse linear algebra benchmark suite in C++20/OpenMP; implemented Matrix Market parsing, COO-to-CSR conversion, serial/parallel CSR SpMV, CTest coverage, and Slurm workflows on UW Hyak.
```

After real SuiteSparse `cpu-g2` smoke:

```text
Validated SparseBench++ on UW Hyak using Slurm batch jobs and real SuiteSparse Matrix Market inputs, with reproducible module setup, CTest checks, CSV result capture, and OpenMP thread sweeps.
```

After full `cpu-g2-mem2x` scaling:

```text
Benchmarked CSR SpMV on real SuiteSparse matrices using UW Hyak’s cpu-g2-mem2x high-memory node, measuring scaling from 1 to 192 OpenMP threads with reproducible Slurm scripts, CSV result capture, and speedup/efficiency analysis.
```

After parser v0.1.1:

```text
Extended Matrix Market ingestion to support real, integer, pattern, and symmetric sparse matrices, enabling robust benchmarking across heterogeneous SuiteSparse inputs.
```

---

## 16. Version roadmap

```text
v0.1     CSR SpMV + Slurm smoke + mem2x scaling
v0.1.1   Matrix Market parser variants: integer, pattern, symmetric
v0.2     Conjugate Gradient for SPD matrices
v0.3     Lanczos eigenvalue approximation
v0.4     baseline comparison: Eigen / SuiteSparse / SciPy / MKL if available
v0.5     OpenMP scheduling experiments: static / dynamic / guided
v0.6     NUMA and memory-placement experiments
```

Current priority:

```text
1. Track 192-core job 34851174 until it leaves QOSGrpCpuLimit and either completes or fails with actionable logs.
2. If 34851174 completes, verify COMPLETED 0:0, empty stderr, CTest 3/3, and 60 CSVs for 6 matrices x 10 threads.
3. Package 34851174 with the same Phase 9 layout and generate 192-core speedup/efficiency analysis.
4. Plot and document 96-core vs 192-core behavior, including memory-bandwidth limits and matrices that peak early.
5. Add CG/Lanczos only after SpMV benchmark reporting is stable.
```

---

## 17. Traceability matrix

| Phase | Input | Command / artifact | Output | Gate |
|---|---|---|---|---|
| 1. Sync commit | Hyak commit `704731d` | `git format-patch` | patch file | Windows `git am` and `git push` succeed |
| 2. Downloader | SuiteSparse URLs | `scripts/download_suitesparse.sh` | `.mtx` files + manifest | manifest nonempty |
| 3. Parser decision | `.mtx` headers | header filter or parser v0.1.1 | supported manifest | at least one real supported matrix |
| 4. Real cpu-g2 smoke | supported manifest | `spmv_smoke_cpu_g2.slurm` | real matrix CSVs | threads `1,2,4,8` CSVs |
| 5. Smoke package | smoke job ID | `tar`, `sha256sum` | downloadable archive | archive downloaded locally |
| 6. mem2x pilot | real manifest | `spmv_mem2x_scaling.slurm` with 32 cores | pilot CSVs | threads `1..32` CSVs |
| 7. mem2x medium | 6 medium matrices | job `34825519` | 48 CSVs + package + analysis | `COMPLETED 0:0`, CTest `3/3`, empty stderr, exact thread coverage |
| 8. mem2x full | same validated medium manifest for controlled probe | job `34851174` | pending 192-core run | currently blocked by `QOSGrpCpuLimit`; require 60 CSVs before conclusions |
| 9. Reporting | final CSVs | plotting scripts | figures + README | benchmark section committed |

---

## 18. References

- Hyak Research Computing Documentation, “Scheduling Jobs”: https://hyak.uw.edu/docs/compute/scheduling-jobs/
- Hyak Research Computing Documentation, “Modules”: https://hyak.uw.edu/docs/tools/modules/
- Hyak Research Computing Documentation, “Start Here”: https://hyak.uw.edu/docs/compute/start-here/
- Slurm Workload Manager, `sbatch` documentation: https://slurm.schedmd.com/sbatch.html
- NIST Matrix Market file format documentation: https://math.nist.gov/MatrixMarket/formats.html
- NIST Matrix Market C I/O note: https://math.nist.gov/MatrixMarket/mmio-c.html
- SuiteSparse Matrix Collection: https://sparse.tamu.edu/
- SuiteSparse Matrix Collection, “About”: https://sparse.tamu.edu/about
