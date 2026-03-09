#!/usr/bin/env python3
"""
Genera graficas de rendimiento a partir de results.csv.

Graficas producidas:
  1. speedup_curves.png   — curvas de speedup por version y tamaño de grafo
  2. efficiency_bars.png  — eficiencia por configuracion
  3. time_comparison.png  — tiempo de ejecucion absoluto por tamaño
  4. scaling_analysis.png — strong scaling: speedup vs workers para cada N

Uso: python3 plot_results.py [results.csv]
"""

import sys
import csv
from pathlib import Path
import matplotlib
matplotlib.use("Agg")          # sin display (apto para servidores)
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np

# ── Configuracion de estilo ──────────────────────────────────────────────────
plt.rcParams.update({
    "figure.dpi": 150,
    "font.size": 11,
    "axes.titlesize": 13,
    "axes.labelsize": 11,
    "legend.fontsize": 9,
    "lines.linewidth": 2,
    "lines.markersize": 7,
})

VERSION_COLORS = {
    "Secuencial": "#555555",
    "MPI":        "#1f77b4",
    "OpenMP":     "#d62728",
    "Hibrido":    "#2ca02c",
}

def color_for(label: str) -> str:
    for k, v in VERSION_COLORS.items():
        if k.lower() in label.lower():
            return v
    return "#888888"

# ── Carga de datos ───────────────────────────────────────────────────────────

def load_csv(path: str) -> list[dict]:
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            rows.append({
                "version":    r["version"],
                "n":          int(r["n"]),
                "procs":      int(r["procs"]),
                "threads":    int(r["threads"]),
                "workers":    int(r["workers"]),
                "time_min":   float(r["time_min"]),
                "time_avg":   float(r["time_avg"]),
                "time_seq":   float(r["time_seq"]),
                "speedup":    float(r["speedup"]),
                "efficiency": float(r["efficiency"]),
            })
    return rows

# ── Graficas ─────────────────────────────────────────────────────────────────

def plot_speedup_curves(rows: list[dict], out: str = "speedup_curves.png"):
    """Speedup vs numero de workers para cada N, una linea por version."""
    sizes = sorted(set(r["n"] for r in rows))
    versions = [v for v in sorted(set(r["version"] for r in rows))
                if v.lower() != "secuencial"]

    ncols = min(2, len(sizes))
    nrows = (len(sizes) + ncols - 1) // ncols
    fig, axes = plt.subplots(nrows, ncols, figsize=(7 * ncols, 5 * nrows))
    axes = np.array(axes).flatten()

    for ax, n in zip(axes, sizes):
        # Linea ideal
        max_w = max(r["workers"] for r in rows)
        ax.plot([1, max_w], [1, max_w], "k--", lw=1, alpha=0.4, label="Ideal")

        for ver in versions:
            pts = [(r["workers"], r["speedup"])
                   for r in rows if r["n"] == n and r["version"] == ver]
            if not pts:
                continue
            pts.sort()
            ws, sps = zip(*pts)
            ax.plot(ws, sps, "o-", color=color_for(ver), label=ver)

        ax.set_title(f"N = {n}")
        ax.set_xlabel("Numero de workers")
        ax.set_ylabel("Speedup")
        ax.legend()
        ax.grid(True, alpha=0.3)

    for ax in axes[len(sizes):]:
        ax.set_visible(False)

    fig.suptitle("Curvas de Speedup — Floyd-Warshall", fontsize=14, y=1.01)
    fig.tight_layout()
    fig.savefig(out, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out}")


def plot_efficiency_bars(rows: list[dict], out: str = "efficiency_bars.png"):
    """Eficiencia por version para un tamano representativo (el mayor N)."""
    max_n = max(r["n"] for r in rows)
    data  = [r for r in rows if r["n"] == max_n and r["version"].lower() != "secuencial"]
    data.sort(key=lambda r: (r["version"], r["workers"]))

    labels = [r["version"] for r in data]
    efics  = [r["efficiency"] for r in data]
    colors = [color_for(r["version"]) for r in data]

    fig, ax = plt.subplots(figsize=(max(8, len(data) * 0.9), 5))
    bars = ax.bar(range(len(data)), efics, color=colors, edgecolor="white", linewidth=0.8)
    ax.axhline(1.0, color="black", lw=1, ls="--", alpha=0.5, label="Eficiencia ideal")
    ax.set_xticks(range(len(data)))
    ax.set_xticklabels(labels, rotation=30, ha="right")
    ax.set_ylabel("Eficiencia (Speedup / Workers)")
    ax.set_ylim(0, 1.15)
    ax.set_title(f"Eficiencia — N={max_n}")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.3)

    for bar, e in zip(bars, efics):
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + 0.02, f"{e:.2f}",
                ha="center", va="bottom", fontsize=9)

    fig.tight_layout()
    fig.savefig(out, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out}")


def plot_time_comparison(rows: list[dict], out: str = "time_comparison.png"):
    """Tiempo minimo de ejecucion vs N para cada version (usando el mejor config)."""
    sizes    = sorted(set(r["n"] for r in rows))
    versions = sorted(set(r["version"] for r in rows))

    fig, ax = plt.subplots(figsize=(8, 5))

    for ver in versions:
        times = []
        ns    = []
        for n in sizes:
            pts = [r["time_min"] for r in rows if r["n"] == n and r["version"] == ver]
            if pts:
                times.append(min(pts))
                ns.append(n)
        if ns:
            marker = "s" if "sec" in ver.lower() else "o"
            ax.plot(ns, times, marker + "-", color=color_for(ver), label=ver)

    ax.set_yscale("log")
    ax.set_xlabel("N (vertices)")
    ax.set_ylabel("Tiempo minimo (s) — escala log")
    ax.set_title("Comparativa de Tiempo de Ejecucion")
    ax.legend()
    ax.grid(True, which="both", alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out}")


def plot_scaling_analysis(rows: list[dict], out: str = "scaling_analysis.png"):
    """Strong scaling: speedup vs workers para el tamano mas grande, con Ley de Amdahl."""
    max_n    = max(r["n"] for r in rows)
    versions = [v for v in sorted(set(r["version"] for r in rows))
                if v.lower() != "secuencial"]

    fig, ax = plt.subplots(figsize=(7, 5))

    # Estimacion de la fraccion paralela (Amdahl) usando el mejor speedup observado
    all_sps = [r["speedup"] for r in rows if r["n"] == max_n and r["version"] != "Secuencial"]
    if all_sps:
        max_sp  = max(all_sps)
        max_w   = max(r["workers"] for r in rows if r["n"] == max_n and r["version"] != "Secuencial")
        # Amdahl: S(p) = 1 / (f + (1-f)/p)  =>  f = (1/S - 1/p) / (1 - 1/p)
        p_vals  = np.linspace(1, max_w + 4, 200)
        if max_sp > 1 and max_w > 1:
            f = (1/max_sp - 1/max_w) / (1 - 1/max_w)
            f = max(0, min(1, f))
            amdahl = 1 / (f + (1 - f) / p_vals)
            ax.plot(p_vals, amdahl, "k:", lw=1.5, alpha=0.6,
                    label=f"Amdahl (f_par≈{1-f:.2f})")

    # Ideal
    max_w_all = max(r["workers"] for r in rows)
    ax.plot([1, max_w_all], [1, max_w_all], "k--", lw=1, alpha=0.4, label="Ideal")

    for ver in versions:
        pts = [(r["workers"], r["speedup"])
               for r in rows if r["n"] == max_n and r["version"] == ver]
        if not pts:
            continue
        pts.sort()
        ws, sps = zip(*pts)
        ax.plot(ws, sps, "o-", color=color_for(ver), label=ver)

    ax.set_xlabel("Numero de workers (procesos × hilos)")
    ax.set_ylabel("Speedup")
    ax.set_title(f"Analisis de Escalado (Strong Scaling) — N={max_n}")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(out, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out}")


# ── Entry point ──────────────────────────────────────────────────────────────

def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "results.csv"
    if not Path(csv_path).exists():
        print(f"Error: no se encontro '{csv_path}'. Ejecuta primero ./benchmark_runner")
        sys.exit(1)

    print(f"Leyendo datos de '{csv_path}' ...")
    rows = load_csv(csv_path)
    print(f"  {len(rows)} registros cargados.\n")
    print("Generando graficas:")

    plot_speedup_curves(rows)
    plot_efficiency_bars(rows)
    plot_time_comparison(rows)
    plot_scaling_analysis(rows)

    print("\nListo. Graficas guardadas en el directorio actual.")


if __name__ == "__main__":
    main()
