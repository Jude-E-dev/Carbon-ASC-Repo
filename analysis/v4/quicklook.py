"""One-line-per-run summary of a reduced v4 campaign (numpy only).

    python3 analysis/v4/quicklook.py <ROOT>/reduced > quicklook.csv

Columns: run, family, change, CRY total counts/day, CRY and NORM counts/day in a
few line windows (ideal resolution, 1 keV bins, +-3 keV), the oxygen/carbon split of
the 4.44 MeV window, and the MC relative error of each CRY value (32 shower batches).
Meant for sanity checks and RUN_NOTES, not for results.
"""

import csv
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from spectra import Run, N_BATCH  # noqa: E402

LINES = {"0.511": 0.511, "1.461": 1.4608, "1.779": 1.7788, "2.223": 2.2232,
         "2.614": 2.6145, "3.539": 3.5394, "4.438": 4.4389, "4.934": 4.9335,
         "6.129": 6.1299, "7.631": 7.6311}
HALF = 0.003
DAY = 86400.0


def main(reduced_dir):
    reduced_dir = Path(reduced_dir)
    rows = []
    for jpath in sorted(reduced_dir.glob("*.json")):
        rid = jpath.stem
        run = Run.load(reduced_dir, rid)
        info = run.info
        row = {"run": rid, "family": info["run"].get("family"),
               "change": info["run"].get("change")}
        if "cry" in info:
            cry = run.cry()
            v, s = cry.window(0.001, 12.0)
            row["cry_total_per_day"] = f"{v * DAY:.4g}"
            row["cry_total_relerr"] = f"{s / v:.3f}" if v else ""
            for name, e in LINES.items():
                v, s = cry.window(e - HALF, e + HALF)
                row[f"cry_{name}"] = f"{v * DAY:.4g}"
                row[f"cry_{name}_relerr"] = f"{s / v:.3f}" if v else ""
            cr = run.class_rates(4.40, 4.48)
            row["w4.44_inelastic_C_per_day"] = f"{cr['inelastic_C'][0] * DAY:.4g}"
            row["w4.44_inelastic_O_per_day"] = f"{cr['inelastic_O'][0] * DAY:.4g}"
        if "norm" in info:
            try:
                norm = run.norm()
                for name in ("1.461", "2.223", "2.614"):
                    e = LINES[name]
                    v, _ = norm.window(e - HALF, e + HALF)
                    row[f"norm_{name}"] = f"{v * DAY:.4g}"
            except KeyError:
                pass
        if "analog" in info:
            p = run.data["analog_pulses"]
            row["analog_pulses_per_s"] = f"{len(p) / info['analog']['exposure_s']:.4g}"
        rows.append(row)
    keys = []
    for r in rows:
        for k in r:
            if k not in keys:
                keys.append(k)
    w = csv.DictWriter(sys.stdout, fieldnames=keys)
    w.writeheader()
    w.writerows(rows)


if __name__ == "__main__":
    main(sys.argv[1])
