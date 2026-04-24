# SparseBench++ v0.1

SparseBench++ v0.1 is being initialized as a small, reproducible sparse-matrix benchmark harness. The v0.1 goal is local correctness first: build the benchmark, run tiny deterministic SpMV cases, and only then scale the same workflow onto Hyak.

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

v0.1 only supports Matrix Market `coordinate real general`; the downloader filters the generated matrix list to that format.

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

## Implemented v0.1

- Matrix Market `coordinate real general` parser.
- COO-to-CSR conversion with 1-based to 0-based index conversion.
- Serial CSR SpMV.
- OpenMP CSR SpMV when the compiler toolchain exposes OpenMP.
- CSV benchmark output with median runtime, minimum runtime, nnz/sec, and checksum.

## Output Policy

Generated results, logs, and plots are ignored by Git. Keep only lightweight metadata, scripts, and reproducibility notes in the repository unless a future release explicitly adds curated benchmark artifacts.
