#!/usr/bin/env python3
"""Generate deterministic SVG figures for the SparseBench report."""

import argparse
import csv
import html
import math
import os
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


MEM2X_RUN = "34825519"
EIGEN_RUN = "34852262"

MEM2X_FIELDS = [
    "matrix",
    "nrows",
    "ncols",
    "nnz",
    "baseline_t1_median_ms",
    "best_thread",
    "best_median_ms",
    "best_speedup",
    "speedup_t96",
    "efficiency_t96",
    "scaling_note",
]
EIGEN_FIELDS = [
    "matrix",
    "threads",
    "sparsebench_median_ms",
    "eigen_median_ms",
    "sparsebench_speedup",
    "eigen_speedup",
    "eigen_over_sparsebench_time_ratio",
    "winner",
]


def default_summary(name: str) -> Path:
    user = os.environ.get("USER", "junyej")
    return Path(f"/gscratch/scrubbed/{user}/sparsebench/analysis/{name}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate small SVG assets from SparseBench summary CSVs."
    )
    parser.add_argument(
        "--mem2x-summary",
        type=Path,
        default=default_summary(f"mem2x_{MEM2X_RUN}_summary.csv"),
    )
    parser.add_argument(
        "--eigen-summary",
        type=Path,
        default=default_summary(f"eigen_baseline_{EIGEN_RUN}_summary.csv"),
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=Path("docs/reports/assets"),
    )
    return parser.parse_args()


def read_rows(path: Path, expected_fields: Sequence[str]) -> List[Dict[str, str]]:
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != list(expected_fields):
            raise ValueError(f"{path}: unexpected fields {reader.fieldnames}")
        rows = list(reader)
    if not rows:
        raise ValueError(f"{path}: no rows")
    return rows


def write_svg(path: Path, width: int, height: int, body: Iterable[str]) -> None:
    content = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        (
            f'<svg xmlns="http://www.w3.org/2000/svg" '
            f'width="{width}" height="{height}" viewBox="0 0 {width} {height}" '
            'role="img">'
        ),
        '<style><![CDATA[',
        "text { font-family: Arial, Helvetica, sans-serif; fill: #111827; }",
        ".title { font-size: 22px; font-weight: 700; }",
        ".subtitle { font-size: 13px; fill: #4b5563; }",
        ".axis { font-size: 12px; fill: #374151; }",
        ".label { font-size: 13px; fill: #1f2937; }",
        ".small { font-size: 11px; fill: #4b5563; }",
        ".value { font-size: 12px; font-weight: 700; }",
        "]]></style>",
        '<rect width="100%" height="100%" fill="#ffffff"/>',
    ]
    content.extend(body)
    content.append("</svg>")
    path.write_text("\n".join(content) + "\n", encoding="utf-8")


def esc(value: object) -> str:
    return html.escape(str(value), quote=True)


def fmt(value: float, digits: int = 3) -> str:
    text = f"{value:.{digits}f}"
    return text.rstrip("0").rstrip(".")


def nice_max(value: float, step: float) -> float:
    return math.ceil(value / step) * step


def horizontal_grouped_bars(
    path: Path,
    title: str,
    subtitle: str,
    rows: Sequence[Dict[str, object]],
    series: Sequence[Tuple[str, str, str]],
    x_label: str,
    value_suffix: str = "",
    max_value: float = 0.0,
) -> None:
    width = 960
    left = 190
    right = 74
    top = 112
    row_gap = 58
    bar_h = 14
    plot_w = width - left - right
    max_seen = max(float(row[key]) for row in rows for _, key, _ in series)
    if max_value <= 0.0:
        max_value = nice_max(max_seen * 1.08, 0.25)
    height = top + row_gap * len(rows) + 72

    body: List[str] = [
        f"<title>{esc(title)}</title>",
        f'<text class="title" x="28" y="34">{esc(title)}</text>',
        f'<text class="subtitle" x="28" y="58">{esc(subtitle)}</text>',
        f'<text class="axis" x="{left}" y="{height - 28}">{esc(x_label)}</text>',
    ]

    legend_x = left
    for name, _, color in series:
        body.append(f'<rect x="{legend_x}" y="78" width="14" height="14" fill="{color}"/>')
        body.append(f'<text class="small" x="{legend_x + 20}" y="90">{esc(name)}</text>')
        legend_x += 178

    for tick in (0.0, max_value / 2.0, max_value):
        x = left + tick / max_value * plot_w
        body.append(f'<line x1="{x:.2f}" y1="{top - 12}" x2="{x:.2f}" y2="{height - 54}" stroke="#e5e7eb"/>')
        body.append(f'<text class="axis" x="{x:.2f}" y="{height - 38}" text-anchor="middle">{fmt(tick, 2)}</text>')

    body.append(f'<line x1="{left}" y1="{height - 54}" x2="{left + plot_w}" y2="{height - 54}" stroke="#9ca3af"/>')

    for index, row in enumerate(rows):
        y = top + index * row_gap
        body.append(f'<text class="label" x="{left - 14}" y="{y + 18}" text-anchor="end">{esc(row["matrix"])}</text>')
        if "note" in row:
            body.append(f'<text class="small" x="{left - 14}" y="{y + 34}" text-anchor="end">{esc(row["note"])}</text>')

        for series_index, (_, key, color) in enumerate(series):
            value = float(row[key])
            bar_y = y + 7 + series_index * (bar_h + 5)
            bar_w = value / max_value * plot_w
            body.append(f'<rect x="{left}" y="{bar_y}" width="{bar_w:.2f}" height="{bar_h}" rx="2" fill="{color}"/>')
            body.append(
                f'<text class="value" x="{left + bar_w + 8:.2f}" y="{bar_y + bar_h - 2}">'
                f'{fmt(value, 3)}{esc(value_suffix)}</text>'
            )

    write_svg(path, width, height, body)


def hex_to_rgb(color: str) -> Tuple[int, int, int]:
    color = color.lstrip("#")
    return tuple(int(color[i : i + 2], 16) for i in (0, 2, 4))  # type: ignore[return-value]


def rgb_to_hex(rgb: Tuple[int, int, int]) -> str:
    return "#" + "".join(f"{max(0, min(255, c)):02x}" for c in rgb)


def interpolate_color(low: str, high: str, fraction: float) -> str:
    lo = hex_to_rgb(low)
    hi = hex_to_rgb(high)
    rgb = tuple(round(lo[i] + (hi[i] - lo[i]) * fraction) for i in range(3))
    return rgb_to_hex(rgb)  # type: ignore[arg-type]


def luminance(color: str) -> float:
    r, g, b = hex_to_rgb(color)
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def draw_ratio_heatmap(path: Path, rows: Sequence[Dict[str, str]]) -> None:
    matrices = sorted({row["matrix"] for row in rows})
    threads = sorted({int(row["threads"]) for row in rows})
    ratio_by_key = {
        (row["matrix"], int(row["threads"])): float(row["eigen_over_sparsebench_time_ratio"])
        for row in rows
    }
    ratios = list(ratio_by_key.values())
    min_ratio = min(ratios)
    max_ratio = max(ratios)

    width = 920
    left = 184
    top = 104
    cell_w = 102
    cell_h = 42
    height = top + cell_h * len(matrices) + 78

    body: List[str] = [
        "<title>Eigen/SparseBench median time ratio by matrix and thread count</title>",
        '<text class="title" x="28" y="34">Eigen/SparseBench median time ratio</text>',
        '<text class="subtitle" x="28" y="58">Job 34852262 on cpu-g2; lower values mean Eigen was faster in that paired run.</text>',
    ]

    for col, thread in enumerate(threads):
        x = left + col * cell_w + cell_w / 2
        body.append(f'<text class="axis" x="{x:.2f}" y="{top - 16}" text-anchor="middle">{thread}t</text>')

    for row_index, matrix in enumerate(matrices):
        y = top + row_index * cell_h
        body.append(f'<text class="label" x="{left - 14}" y="{y + 26}" text-anchor="end">{esc(matrix)}</text>')
        for col, thread in enumerate(threads):
            value = ratio_by_key[(matrix, thread)]
            if max_ratio == min_ratio:
                frac = 0.0
            else:
                frac = (value - min_ratio) / (max_ratio - min_ratio)
            color = interpolate_color("#0f766e", "#f59e0b", frac)
            text_color = "#ffffff" if luminance(color) < 125 else "#111827"
            x = left + col * cell_w
            body.append(f'<rect x="{x}" y="{y}" width="{cell_w - 4}" height="{cell_h - 4}" rx="3" fill="{color}"/>')
            body.append(
                f'<text class="value" x="{x + (cell_w - 4) / 2:.2f}" y="{y + 24}" '
                f'text-anchor="middle" fill="{text_color}">{fmt(value, 2)}</text>'
            )

    body.append(f'<text class="small" x="{left}" y="{height - 30}">ratio range: {fmt(min_ratio, 3)} to {fmt(max_ratio, 3)}</text>')
    write_svg(path, width, height, body)


def draw_best_backend(path: Path, rows: Sequence[Dict[str, str]]) -> None:
    by_matrix: Dict[str, Dict[str, object]] = {}
    wins = {"sparsebench": 0, "eigen": 0, "tie": 0}

    for row in rows:
        winner = row["winner"]
        if winner not in wins:
            raise ValueError(f"Unexpected winner value: {winner}")
        wins[winner] += 1

        matrix = row["matrix"]
        thread = int(row["threads"])
        candidates = [
            ("SparseBench", thread, float(row["sparsebench_median_ms"]), "#2563eb"),
            ("Eigen", thread, float(row["eigen_median_ms"]), "#16a34a"),
        ]
        current = by_matrix.get(matrix)
        for backend, backend_thread, median_ms, color in candidates:
            if current is None or median_ms < float(current["median_ms"]):
                current = {
                    "matrix": matrix,
                    "backend": backend,
                    "threads": backend_thread,
                    "median_ms": median_ms,
                    "color": color,
                }
        if current is not None:
            by_matrix[matrix] = current

    chart_rows = [by_matrix[matrix] for matrix in sorted(by_matrix)]
    best_counts: Dict[str, int] = {}
    for row in chart_rows:
        best_counts[str(row["backend"])] = best_counts.get(str(row["backend"]), 0) + 1

    width = 960
    left = 190
    right = 96
    top = 106
    row_gap = 44
    bar_h = 17
    plot_w = width - left - right
    max_value = nice_max(max(float(row["median_ms"]) for row in chart_rows) * 1.15, 0.1)
    height = top + row_gap * len(chart_rows) + 72

    subtitle = (
        f"Job {EIGEN_RUN}; best backend by matrix: "
        f"Eigen {best_counts.get('Eigen', 0)}/{len(chart_rows)}, "
        f"SparseBench {best_counts.get('SparseBench', 0)}/{len(chart_rows)}. "
        f"Pairwise wins: Eigen {wins['eigen']}, SparseBench {wins['sparsebench']}."
    )

    body: List[str] = [
        "<title>Best backend by matrix</title>",
        '<text class="title" x="28" y="34">Best backend by matrix</text>',
        f'<text class="subtitle" x="28" y="58">{esc(subtitle)}</text>',
        f'<text class="axis" x="{left}" y="{height - 28}">best median ms across tested thread counts</text>',
    ]

    for tick in (0.0, max_value / 2.0, max_value):
        x = left + tick / max_value * plot_w
        body.append(f'<line x1="{x:.2f}" y1="{top - 12}" x2="{x:.2f}" y2="{height - 54}" stroke="#e5e7eb"/>')
        body.append(f'<text class="axis" x="{x:.2f}" y="{height - 38}" text-anchor="middle">{fmt(tick, 2)}</text>')

    body.append(f'<line x1="{left}" y1="{height - 54}" x2="{left + plot_w}" y2="{height - 54}" stroke="#9ca3af"/>')

    for index, row in enumerate(chart_rows):
        y = top + index * row_gap
        value = float(row["median_ms"])
        bar_w = value / max_value * plot_w
        body.append(f'<text class="label" x="{left - 14}" y="{y + 16}" text-anchor="end">{esc(row["matrix"])}</text>')
        body.append(f'<rect x="{left}" y="{y}" width="{bar_w:.2f}" height="{bar_h}" rx="3" fill="{row["color"]}"/>')
        body.append(
            f'<text class="value" x="{left + bar_w + 8:.2f}" y="{y + bar_h - 3}">'
            f'{esc(row["backend"])} t{row["threads"]}: {fmt(value, 3)} ms</text>'
        )

    write_svg(path, width, height, body)


def main() -> None:
    args = parse_args()
    mem2x_rows = read_rows(args.mem2x_summary, MEM2X_FIELDS)
    eigen_rows = read_rows(args.eigen_summary, EIGEN_FIELDS)
    args.out_dir.mkdir(parents=True, exist_ok=True)

    speedup_rows = [
        {
            "matrix": row["matrix"],
            "best_speedup": float(row["best_speedup"]),
            "speedup_t96": float(row["speedup_t96"]),
            "note": f"best t{row['best_thread']}",
        }
        for row in mem2x_rows
    ]
    horizontal_grouped_bars(
        args.out_dir / f"mem2x_{MEM2X_RUN}_speedup.svg",
        "SparseBench 96-core mem2x speedup",
        "Job 34825519; speedup is relative to each matrix's 1-thread median time.",
        speedup_rows,
        [
            ("Best observed speedup", "best_speedup", "#2563eb"),
            ("Speedup at 96 threads", "speedup_t96", "#d97706"),
        ],
        "speedup over 1 thread",
        max_value=2.35,
    )

    efficiency_rows = [
        {
            "matrix": row["matrix"],
            "efficiency_pct": float(row["efficiency_t96"]) * 100.0,
            "note": "96 threads",
        }
        for row in mem2x_rows
    ]
    horizontal_grouped_bars(
        args.out_dir / f"mem2x_{MEM2X_RUN}_efficiency.svg",
        "SparseBench 96-thread parallel efficiency",
        "Job 34825519; efficiency = speedup / 96, shown as percent.",
        efficiency_rows,
        [("Efficiency at 96 threads", "efficiency_pct", "#7c3aed")],
        "parallel efficiency at 96 threads (%)",
        value_suffix="%",
        max_value=2.5,
    )

    draw_ratio_heatmap(args.out_dir / f"eigen_{EIGEN_RUN}_time_ratio.svg", eigen_rows)
    draw_best_backend(args.out_dir / f"eigen_{EIGEN_RUN}_best_backend.svg", eigen_rows)

    for name in (
        f"mem2x_{MEM2X_RUN}_speedup.svg",
        f"mem2x_{MEM2X_RUN}_efficiency.svg",
        f"eigen_{EIGEN_RUN}_time_ratio.svg",
        f"eigen_{EIGEN_RUN}_best_backend.svg",
    ):
        print(args.out_dir / name)


if __name__ == "__main__":
    main()
