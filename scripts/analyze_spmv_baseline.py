#!/usr/bin/env python3
"""Summarize SparseBench vs Eigen SpMV baseline CSVs without third-party deps."""

import argparse
import csv
import math
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple


EXPECTED_HEADER = [
    "matrix",
    "nrows",
    "ncols",
    "nnz",
    "threads",
    "repeat",
    "median_ms",
    "min_ms",
    "nnz_per_sec",
    "checksum",
]
FILENAME_RE = re.compile(r"^(sparsebench|eigen)_(.+)_t([1-9][0-9]*)\.csv$")
BACKENDS = ("sparsebench", "eigen")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Analyze SparseBench and Eigen SpMV baseline CSVs."
    )
    parser.add_argument("result_dir", type=Path)
    parser.add_argument("--out-dir", type=Path)
    parser.add_argument("--run-id")
    parser.add_argument("--expected-repeat", type=int)
    parser.add_argument("--expected-file-count", type=int)
    parser.add_argument("--checksum-tolerance", type=float, default=1e-8)
    return parser.parse_args()


def infer_run_id(result_dir: Path) -> str:
    match = re.search(r"eigen_baseline_([^/]+)$", result_dir.name)
    if match:
        return match.group(1)
    return result_dir.name


def default_out_dir(result_dir: Path) -> Path:
    if result_dir.parent.name == "results":
        return result_dir.parent.parent / "analysis"
    return result_dir / "analysis"


def read_one_csv(path: Path) -> Dict[str, object]:
    match = FILENAME_RE.match(path.name)
    if not match:
        raise ValueError(f"Unexpected CSV filename: {path.name}")
    filename_backend, filename_matrix, filename_threads = match.groups()

    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != EXPECTED_HEADER:
            raise ValueError(f"{path}: unexpected header {reader.fieldnames}")
        rows = list(reader)

    if len(rows) != 1:
        raise ValueError(f"{path}: expected exactly one data row, found {len(rows)}")

    row = rows[0]
    matrix = row["matrix"]
    threads = int(row["threads"])
    repeat = int(row["repeat"])
    if matrix != filename_matrix:
        raise ValueError(f"{path}: filename matrix {filename_matrix} != row {matrix}")
    if threads != int(filename_threads):
        raise ValueError(f"{path}: filename threads {filename_threads} != row {threads}")

    parsed = {
        "backend": filename_backend,
        "matrix": matrix,
        "nrows": int(row["nrows"]),
        "ncols": int(row["ncols"]),
        "nnz": int(row["nnz"]),
        "threads": threads,
        "repeat": repeat,
        "median_ms": float(row["median_ms"]),
        "min_ms": float(row["min_ms"]),
        "nnz_per_sec": float(row["nnz_per_sec"]),
        "checksum": float(row["checksum"]),
        "path": path,
    }

    for key in ("median_ms", "min_ms", "nnz_per_sec", "checksum"):
        value = parsed[key]
        if not isinstance(value, float) or not math.isfinite(value):
            raise ValueError(f"{path}: {key} is not finite")
    for key in ("median_ms", "min_ms", "nnz_per_sec"):
        value = parsed[key]
        if not isinstance(value, float) or value <= 0.0:
            raise ValueError(f"{path}: {key} must be positive")

    return parsed


def format_float(value: float) -> str:
    if math.isnan(value):
        return "nan"
    return f"{value:.12g}"


def validate_rows(
    rows: Dict[Tuple[str, str, int], Dict[str, object]],
    expected_repeat: Optional[int],
    checksum_tolerance: float,
) -> Tuple[List[str], List[str]]:
    errors = []  # type: List[str]
    warnings = []  # type: List[str]

    matrices = sorted({key[1] for key in rows})
    threads = sorted({key[2] for key in rows})
    for matrix in matrices:
        for thread in threads:
            pair = {}
            for backend in BACKENDS:
                row = rows.get((backend, matrix, thread))
                if row is None:
                    errors.append(f"missing {backend} row for {matrix} t{thread}")
                else:
                    pair[backend] = row
                    if expected_repeat is not None and row["repeat"] != expected_repeat:
                        errors.append(
                            f"{row['path']}: repeat {row['repeat']} != {expected_repeat}"
                        )
            if len(pair) == 2:
                sparse_checksum = float(pair["sparsebench"]["checksum"])
                eigen_checksum = float(pair["eigen"]["checksum"])
                limit = checksum_tolerance * max(1.0, abs(sparse_checksum))
                if abs(sparse_checksum - eigen_checksum) > limit:
                    errors.append(
                        f"checksum mismatch for {matrix} t{thread}: "
                        f"sparsebench={sparse_checksum}, eigen={eigen_checksum}"
                    )

                for field in ("nrows", "ncols", "nnz", "repeat"):
                    if pair["sparsebench"][field] != pair["eigen"][field]:
                        errors.append(
                            f"{field} mismatch for {matrix} t{thread}: "
                            f"sparsebench={pair['sparsebench'][field]}, "
                            f"eigen={pair['eigen'][field]}"
                        )

    if not matrices:
        errors.append("no result CSVs found")
    if warnings:
        warnings.sort()
    return errors, warnings


def write_outputs(
    rows: Dict[Tuple[str, str, int], Dict[str, object]],
    out_dir: Path,
    run_id: str,
    warnings: List[str],
) -> Tuple[Path, Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    csv_path = out_dir / f"eigen_baseline_{run_id}_summary.csv"
    md_path = out_dir / f"eigen_baseline_{run_id}_summary.md"

    matrices = sorted({key[1] for key in rows})
    threads = sorted({key[2] for key in rows})
    t1 = {
        (backend, matrix): float(row["median_ms"])
        for (backend, matrix, thread), row in rows.items()
        if thread == 1
    }

    summary_rows = []  # type: List[Dict[str, str]]
    wins = {"sparsebench": 0, "eigen": 0, "tie": 0}
    best_by_matrix = {}  # type: Dict[str, Tuple[str, int, float]]

    for matrix in matrices:
        best_backend = ""
        best_threads = 0
        best_time = math.inf
        for thread in threads:
            sparse = rows.get(("sparsebench", matrix, thread))
            eigen = rows.get(("eigen", matrix, thread))
            if sparse is None or eigen is None:
                continue

            sparse_time = float(sparse["median_ms"])
            eigen_time = float(eigen["median_ms"])
            sparse_speedup = t1[("sparsebench", matrix)] / sparse_time
            eigen_speedup = t1[("eigen", matrix)] / eigen_time
            ratio = eigen_time / sparse_time

            if abs(ratio - 1.0) <= 1e-9:
                winner = "tie"
            elif ratio < 1.0:
                winner = "eigen"
            else:
                winner = "sparsebench"
            wins[winner] += 1

            for backend, backend_threads, backend_time in (
                ("sparsebench", thread, sparse_time),
                ("eigen", thread, eigen_time),
            ):
                if backend_time < best_time:
                    best_backend = backend
                    best_threads = backend_threads
                    best_time = backend_time

            summary_rows.append(
                {
                    "matrix": matrix,
                    "threads": str(thread),
                    "sparsebench_median_ms": format_float(sparse_time),
                    "eigen_median_ms": format_float(eigen_time),
                    "sparsebench_speedup": format_float(sparse_speedup),
                    "eigen_speedup": format_float(eigen_speedup),
                    "eigen_over_sparsebench_time_ratio": format_float(ratio),
                    "winner": winner,
                }
            )

        best_by_matrix[matrix] = (best_backend, best_threads, best_time)

    with csv_path.open("w", newline="") as handle:
        fieldnames = [
            "matrix",
            "threads",
            "sparsebench_median_ms",
            "eigen_median_ms",
            "sparsebench_speedup",
            "eigen_speedup",
            "eigen_over_sparsebench_time_ratio",
            "winner",
        ]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(summary_rows)

    with md_path.open("w") as handle:
        handle.write(f"# Eigen Baseline Summary {run_id}\n\n")
        handle.write("## Coverage\n\n")
        handle.write(f"- Matrices: {len(matrices)}\n")
        handle.write(f"- Thread counts: {','.join(str(t) for t in threads)}\n")
        handle.write(f"- Paired matrix/thread comparisons: {len(summary_rows)}\n")
        handle.write(f"- SparseBench wins: {wins['sparsebench']}\n")
        handle.write(f"- Eigen wins: {wins['eigen']}\n")
        handle.write(f"- Ties: {wins['tie']}\n")
        if warnings:
            handle.write(f"- Warnings: {len(warnings)}\n")
        handle.write("\n## Best Backend By Matrix\n\n")
        handle.write("| matrix | best_backend | best_threads | best_median_ms |\n")
        handle.write("|---|---:|---:|---:|\n")
        for matrix in matrices:
            backend, thread, median = best_by_matrix[matrix]
            handle.write(
                f"| {matrix} | {backend} | {thread} | {format_float(median)} |\n"
            )
        handle.write("\n## Per Thread Summary\n\n")
        handle.write(
            "| matrix | threads | sparsebench_ms | eigen_ms | "
            "sparsebench_speedup | eigen_speedup | eigen/sparsebench | winner |\n"
        )
        handle.write("|---|---:|---:|---:|---:|---:|---:|---|\n")
        for row in summary_rows:
            handle.write(
                f"| {row['matrix']} | {row['threads']} | "
                f"{row['sparsebench_median_ms']} | {row['eigen_median_ms']} | "
                f"{row['sparsebench_speedup']} | {row['eigen_speedup']} | "
                f"{row['eigen_over_sparsebench_time_ratio']} | {row['winner']} |\n"
            )
        if warnings:
            handle.write("\n## Warnings\n\n")
            for warning in warnings:
                handle.write(f"- {warning}\n")

    return csv_path, md_path


def main() -> int:
    args = parse_args()
    result_dir = args.result_dir.resolve()
    if not result_dir.is_dir():
        raise SystemExit(f"Result directory does not exist: {result_dir}")

    paths = sorted(result_dir.glob("*.csv"))
    if args.expected_file_count is not None and len(paths) != args.expected_file_count:
        raise SystemExit(
            f"Expected {args.expected_file_count} CSV files, found {len(paths)}"
        )

    rows = {}  # type: Dict[Tuple[str, str, int], Dict[str, object]]
    for path in paths:
        row = read_one_csv(path)
        key = (
            str(row["backend"]),
            str(row["matrix"]),
            int(row["threads"]),
        )
        if key in rows:
            raise SystemExit(f"Duplicate result for {key}: {path}")
        rows[key] = row

    errors, warnings = validate_rows(
        rows,
        args.expected_repeat,
        args.checksum_tolerance,
    )
    if errors:
        raise SystemExit("\n".join(errors))

    run_id = args.run_id or infer_run_id(result_dir)
    out_dir = args.out_dir.resolve() if args.out_dir else default_out_dir(result_dir)
    csv_path, md_path = write_outputs(rows, out_dir, run_id, warnings)
    print(f"Wrote {csv_path}")
    print(f"Wrote {md_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
