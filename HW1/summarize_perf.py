#!/usr/bin/env python3
import csv
import os
import statistics

ROOT = os.path.dirname(os.path.abspath(__file__))
CSV_PATH = os.path.join(ROOT, "perf_results.csv")
OUT_MD = os.path.join(ROOT, "perf_summary.md")
OUT_PNG = os.path.join(ROOT, "perf_plot.png")

rows = []
with open(CSV_PATH, newline="") as f:
    r = csv.DictReader(f)
    for row in r:
        rows.append(row)

by = {}
for row in rows:
    key = (row["impl"], int(row["p"]))
    by.setdefault(key, []).append(float(row["seconds"]))

lines = []
lines.append("# Performance Summary\n")
lines.append("| Impl | P | Avg Time (s) | Std Dev (s) |\n")
lines.append("|------|---|--------------|-------------|\n")
for (impl, p) in sorted(by.keys()):
    vals = by[(impl, p)]
    avg = statistics.mean(vals)
    stdev = statistics.pstdev(vals)
    lines.append(f"| {impl} | {p} | {avg:.6f} | {stdev:.6f} |\n")

with open(OUT_MD, "w") as f:
    f.writelines(lines)

# Optional plot
try:
    import matplotlib.pyplot as plt

    impls = sorted(set(k[0] for k in by.keys()))
    for impl in impls:
        ps = sorted(p for (i, p) in by.keys() if i == impl)
        avgs = [statistics.mean(by[(impl, p)]) for p in ps]
        plt.plot(ps, avgs, marker="o", label=impl)

    plt.xlabel("Processes (P)")
    plt.ylabel("Avg Time (s)")
    plt.title("HW1 Performance")
    plt.legend()
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(OUT_PNG, dpi=150)
except Exception:
    pass

print(f"Wrote {OUT_MD}")
if os.path.exists(OUT_PNG):
    print(f"Wrote {OUT_PNG}")
else:
    print("Plot not generated (matplotlib missing).")
