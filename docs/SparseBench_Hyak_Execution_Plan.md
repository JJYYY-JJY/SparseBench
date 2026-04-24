# SparseBench Hyak Execution Plan

**Document purpose.** This document records the operational plan for taking `SparseBench` from the completed `cpu-g2` smoke milestone to real SuiteSparse benchmarks and then to `cpu-g2-mem2x` scaling. It is designed to be committed to the repo as a traceable project/runbook document.

**Suggested repo path.** `docs/hyak_execution_plan.md`

**Current status as of 2026-04-24.** The local implementation and Hyak `cpu-g2` smoke path are operational. The next blocker is data ingestion: SuiteSparse download currently fails because of a `curl` / `git-remote-https` OpenSSL/Kerberos runtime-linker issue, so the last successful Slurm smoke used the planned `diag5.mtx` fallback. That validates the Slurm/environment/log/result path, not sparse benchmark performance. The execution plan was committed locally as `d107441`, but `git push origin main` is currently blocked in this shell by missing non-interactive GitHub credentials. A repo hygiene update now ignores local `.codex` files/directories so session noise stays out of subsequent commits.

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

Do **not** do these until the real SuiteSparse `cpu-g2` smoke passes:

- Do not submit `slurm/spmv_mem2x_scaling.slurm`.
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

The current parser supports only a limited Matrix Market subset. Until parser support is expanded, real-matrix gates must accept only:

```text
%%MatrixMarket matrix coordinate real general
```

Other Matrix Market variants must either be skipped or supported explicitly in a later parser milestone.

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

---

## 7. Phase 3 — Parser coverage decision

### 7.1 Goal

Decide whether to run only `coordinate real general` matrices or expand the parser before real benchmarks.

### 7.2 Minimum current gate

Use this filter if parser remains unchanged:

```bash
manifest=/gscratch/scrubbed/$USER/sparsebench/data/small_matrices.txt
supported=/gscratch/scrubbed/$USER/sparsebench/data/supported_small_matrices.txt

: > "$supported"

while IFS= read -r m; do
  [ -f "$m" ] || continue
  header="$(head -n 1 "$m" | tr -d '\r')"
  if [ "$header" = "%%MatrixMarket matrix coordinate real general" ]; then
    echo "$m" >> "$supported"
  fi
done < "$manifest"

mv "$supported" "$manifest"
cat "$manifest"
```

### 7.3 Recommended parser v0.1.1

Before `cpu-g2-mem2x`, expand Matrix Market support to:

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

### 7.4 Tests to add

Add tiny fixtures:

```text
data/tiny/integer5.mtx
data/tiny/pattern5.mtx
data/tiny/symmetric5.mtx
```

Add CTest cases verifying:

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

### 10.2 Preconditions

Do not start this phase unless all are true:

- GitHub contains the Slurm smoke-script commit.
- SuiteSparse manifest contains at least one real supported matrix.
- Real SuiteSparse `cpu-g2` smoke has completed successfully.
- `slurm/spmv_mem2x_scaling.slurm` passes `bash -n`.
- `sbatch --test-only slurm/spmv_mem2x_scaling.slurm` passes.
- Manifest is not the `diag5.mtx` fallback.

### 10.3 Recommended pilot resource request

Modify the mem2x script for the first pilot:

```bash
#SBATCH --partition=cpu-g2-mem2x
#SBATCH --qos=stf-cpu-g2-mem2x
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --mem=128G
#SBATCH --time=01:00:00
```

Thread list:

```bash
THREADS=(1 2 4 8 16 32)
```

Matrix count:

```text
1–3 real SuiteSparse small matrices
```

### 10.4 Submission

```bash
cd /gscratch/stf/$USER/projects/SparseBench

bash -n slurm/spmv_mem2x_scaling.slurm
sbatch --test-only slurm/spmv_mem2x_scaling.slurm

jid=$(sbatch slurm/spmv_mem2x_scaling.slurm | awk '{print $4}')
echo "$jid"
squeue -j "$jid"
```

### 10.5 Acceptance criteria

- Job completes.
- CTest passes.
- CSVs are produced for `1,2,4,8,16,32` threads.
- No `GLIBCXX`, `libstdc++`, `libgomp`, OpenSSL, or Kerberos runtime-linker errors appear.
- Results use real SuiteSparse matrices.

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
5–10 small/medium SuiteSparse matrices
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
1. Sync Hyak commit to GitHub.
2. Fix downloader or transfer SuiteSparse matrices manually.
3. Build real SuiteSparse manifest.
4. Run real SuiteSparse cpu-g2 smoke.
5. Run mem2x 32-core pilot.
6. Run mem2x 96/192-core scaling.
7. Plot and document results.
8. Add CG/Lanczos only after SpMV benchmark is stable.
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
| 7. mem2x medium | 5–10 matrices | 96-core run | preliminary scaling | plausible speedup/efficiency |
| 8. mem2x full | final matrix set | 192-core run | final CSVs | full thread coverage |
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
