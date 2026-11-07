# Compares two dumps of the pmove determinism test (netcodec_pmove_game --dump=<file>), command by command: per
# scenario and pmove_float mode, the commands whose movement fields differ, the first one, and the largest difference
# of origin (units) and velocity (units/s). --modes=<a>:<b> compares the rounding mode a of the first dump with the
# mode b of the second (for example 3:0, 1.01's rounding against the engine's), else every mode with itself.
# Usage: python -I pmove_compare.py [--modes=a:b] <dump a> <dump b>
import struct
import sys

NAMES = ["origin x", "origin y", "origin z", "velocity x", "velocity y", "velocity z", "pitch", "yaw", "roll"]


def f32(h):
    return struct.unpack("<f", struct.pack("<I", int(h, 16)))[0]


def read(path):
    rows = {}
    with open(path) as f:
        for line in f:
            t = line.split()
            rows[(int(t[0]), int(t[1]), int(t[2]))] = (t[3:12], t[12:15])
    return rows


def main():
    args = sys.argv[1:]
    modes = None
    if args[0].startswith("--modes="):
        ma, mb = args.pop(0)[8:].split(":")
        modes = (int(ma), int(mb))
    a, b = read(args[0]), read(args[1])
    if modes:   # relabel b's mode as a's
        b = {(s, modes[0], i): v for (s, m, i), v in b.items() if m == modes[1]}
        a = {k: v for k, v in a.items() if k[1] == modes[0]}
    keys = sorted(set(a) & set(b))
    runs = sorted({(s, m) for s, m, _ in keys})
    for s, m in runs:
        steps = [k for k in keys if k[0] == s and k[1] == m]
        diff = 0
        first = None
        max_org = max_vel = 0.0
        for k in steps:
            (fa, ia), (fb, ib) = a[k], b[k]
            if fa == fb and ia == ib:
                continue
            diff += 1
            if first is None:
                fields = [NAMES[i] for i in range(9) if fa[i] != fb[i]] + (["ground/flags/time"] if ia != ib else [])
                first = (k[2], fields)
            max_org = max(max_org, max(abs(f32(fa[i]) - f32(fb[i])) for i in range(3)))
            max_vel = max(max_vel, max(abs(f32(fa[i]) - f32(fb[i])) for i in range(3, 6)))
        where = f"first at command {first[0]} ({', '.join(first[1])})" if first else "identical"
        print(f"scenario {s} mode {m if not modes else f'{modes[0]}:{modes[1]}'}: {diff} of {len(steps)} commands differ; {where}; "
              f"max |d origin| {max_org:.6g}, max |d velocity| {max_vel:.6g}")


main()
