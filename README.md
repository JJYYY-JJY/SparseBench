# SparseBench++ v0.1.1

SparseBench++ is being initialized as a small, reproducible sparse-matrix benchmark harness. The current goal is correctness first: build the benchmark, run tiny deterministic SpMV cases, and only then scale the same workflow onto Hyak.

## Repository Layout

- `include/`, `src/`: C++ benchmark implementation.
- `tests/`: local correctness and smoke tests.
- `data/tiny/`: checked-in Matrix Market fixtures.
- `scripts/`: local helper templates.
- `slurm/`: Hyak batch templates.
- `results/`, `logs/`, `plots/`: generated benchmark outputs.

## Local Correctness Workflow

Use an out-of-tree build and keep generated files out of the source tree.

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64
cmake -S . -B build-nmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-nmake
ctest --test-dir build-nmake --output-on-failure
```

For a lightweight end-to-end smoke template, inspect:

```powershell
.\scripts\run_local_smoke.ps1
```

The benchmark CLI is:

```powershell
.\build-nmake\sparsebench_spmv.exe data\tiny\diag5.mtx --threads 1 --repeat 10 --out results\local_diag5_t1.csv
```

## SuiteSparse Data

Large SuiteSparse matrices should not be committed. Use scratch storage for downloaded archives and extracted matrices. The template script documents the expected locations:

```bash
bash scripts/download_suitesparse.sh
```

The parser supports these sparse Matrix Market formats:

- `coordinate real general`
- `coordinate integer general`
- `coordinate pattern general`
- `coordinate real symmetric`

The downloader filters the generated matrix list to parser-supported formats.

On Hyak, use:

- Project workspace: `/gscratch/stf/$USER/projects/SparseBench`
- Scratch matrix cache: `/gscratch/scrubbed/$USER/sparsebench`

## Hyak Workflow

The Slurm templates target the `stf` account/partition family and keep generated output under `results/`.

CPU G2 smoke:

```bash
sbatch slurm/spmv_smoke_cpu_g2.slurm
```

CPU G2 memory scaling:

```bash
sbatch slurm/spmv_mem2x_scaling.slurm
```

Before submitting, confirm the binary name, matrix manifest, module stack, time limit, and memory request match the implemented benchmark and the current Hyak policy.

## Implemented

- Matrix Market parser for real, integer, pattern, and real symmetric sparse inputs.
- COO-to-CSR conversion with 1-based to 0-based index conversion.
- Serial CSR SpMV.
- OpenMP CSR SpMV when the compiler toolchain exposes OpenMP.
- CSV benchmark output with median runtime, minimum runtime, nnz/sec, and checksum.

## Benchmarks

The current benchmark report is [docs/reports/sparsebench_spmv_baseline_report.md](docs/reports/sparsebench_spmv_baseline_report.md). It records two completed Hyak evidence sets and keeps the pending 192-core probe separate from completed results.

| Evidence | Scope | Completed output | Run-specific result |
|---|---|---|---|
| `34825519` | 96-core `cpu-g2-mem2x`, 6 SuiteSparse medium matrices, SparseBench CSR SpMV, threads `1,2,4,8,16,32,64,96` | `COMPLETED 0:0`, CTest `3/3`, empty stderr, 48 CSVs | Best observed speedups were `1.693x` to `2.123x`; all matrices peaked before 96 threads, consistent with memory-bandwidth-limited SpMV. |
| `34852262` | 32-core `cpu-g2`, same 6 matrices, paired SparseBench vs Eigen, threads `1,2,4,8,16,32` | `COMPLETED 0:0`, CTest `5/5`, empty stderr, 72 paired CSVs | Eigen won `36/36` paired matrix/thread comparisons in this run. |
| `34851174` | 192-core `cpu-g2-mem2x` probe, same medium manifest | `PENDING` with `QOSGrpCpuLimit` | No 192-core CSVs, logs, package, or benchmark conclusion yet. |

Default build check, with Eigen disabled:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
test ! -e build/sparsebench_spmv_eigen
```

Reproduce the Eigen baseline workflow on Hyak:

```bash
cd /gscratch/stf/$USER/projects/SparseBench

bash -n slurm/spmv_eigen_baseline_cpu_g2.slurm
sbatch --test-only slurm/spmv_eigen_baseline_cpu_g2.slurm
sbatch slurm/spmv_eigen_baseline_cpu_g2.slurm
```

Regenerate the curated report figures from existing summaries:

```bash
python3 scripts/generate_report_assets.py \
  --mem2x-summary /gscratch/scrubbed/$USER/sparsebench/analysis/mem2x_34825519_summary.csv \
  --eigen-summary /gscratch/scrubbed/$USER/sparsebench/analysis/eigen_baseline_34852262_summary.csv \
  --out-dir docs/reports/assets
```

Raw benchmark CSVs, Slurm logs, and scratch packages remain generated artifacts and should stay out of Git. The committed SVGs under `docs/reports/assets/` are lightweight derived report assets.

## Output Policy

Generated results, logs, and plots are ignored by Git. Keep only lightweight metadata, scripts, and reproducibility notes in the repository unless a future release explicitly adds curated benchmark artifacts.
