#!/usr/bin/env python3
"""Report observed RSS bounds without extrapolating a leak claim."""
import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("samples")
    args = parser.parse_args()
    rows = Path(args.samples).read_text().splitlines()[1:]
    values = [int(row.split("\t")[1]) for row in rows if row.strip()]
    if len(values) < 2:
        raise SystemExit("RSSSummary: FAIL fewer than two samples")
    steady = values[len(values) // 4:(3 * len(values)) // 4]
    print(
        "RSSSummary: "
        f"samples={len(values)} min_kb={min(values)} max_kb={max(values)} "
        f"first_kb={values[0]} last_kb={values[-1]} delta_kb={values[-1] - values[0]}"
    )
    print(
        "RSSSummaryMiddleHalf: "
        f"samples={len(steady)} min_kb={min(steady)} max_kb={max(steady)} "
        f"first_kb={steady[0]} last_kb={steady[-1]} delta_kb={steady[-1] - steady[0]}"
    )


if __name__ == "__main__":
    main()
