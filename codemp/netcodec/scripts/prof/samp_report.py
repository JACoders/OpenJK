# Aggregate sampler output: per phase, share of samples by innermost function, by inlined call chain (two levels),
# and by source line. Usage: samp_report.py samples.txt exe base_hex [phase ...]
import sys, subprocess, collections, re

samples_path, exe, base = sys.argv[1], sys.argv[2], int(sys.argv[3], 16)
phases = [int(p) for p in sys.argv[4:]] or [1, 2, 3, 4]
PREF = 0x140000000
TEXT_END = PREF + 0x400000

rows = []
for line in open(samples_path):
    ph, a = line.split()
    rows.append((int(ph), int(a, 16) if a.startswith('0x') else a))

def rva(a):
    if isinstance(a, str):
        return None
    v = a - base + PREF
    return v if PREF <= v < TEXT_END else None

uniq = sorted({rva(a) for _, a in rows if rva(a) is not None})
out = subprocess.run(["llvm-symbolizer", "--obj=" + exe, "--functions=short", "--demangle", "--relative-address"] if False else
                     ["llvm-symbolizer", "--obj=" + exe, "--functions=short", "--demangle"],
                     input="\n".join(hex(x) for x in uniq) + "\n", capture_output=True, text=True).stdout
blocks = out.strip("\n").split("\n\n")
frames = {}
for addr, blk in zip(uniq, blocks):
    ls = blk.split("\n")
    fr = []
    for i in range(0, len(ls) - 1, 2):
        fn = re.sub(r"\(.*", "", ls[i]).strip()
        loc = ls[i + 1].strip()
        loc = re.sub(r"^.*[\\/]", "", loc)
        fr.append((fn, loc))
    frames[addr] = fr

names = {1: "warm encode", 2: "warm decode", 3: "cold encode", 4: "cold decode"}
for ph in phases:
    sel = [a for p, a in rows if p == ph]
    n = len(sel)
    if n == 0:
        continue
    inner, chain, lines = collections.Counter(), collections.Counter(), collections.Counter()
    for a in sel:
        r = rva(a)
        if r is None:
            k = a.split("+")[0] if isinstance(a, str) else "?"
            inner["<" + k + ">"] += 1
            chain["<" + k + ">"] += 1
            continue
        fr = frames.get(r, [("?", "?")])
        inner[fr[0][0]] += 1
        chain[" < ".join(f[0] for f in fr[:3])] += 1
        lines[fr[0][1].rsplit(":", 1)[0] + "  [" + fr[0][0] + "]"] += 1
    print(f"=== {names[ph]}: {n} samples")
    print("-- innermost function")
    for k, v in inner.most_common(25):
        print(f"{100 * v / n:5.1f}%  {k}")
    print("-- chain (innermost < caller < caller)")
    for k, v in chain.most_common(25):
        print(f"{100 * v / n:5.1f}%  {k}")
    print("-- source line")
    for k, v in lines.most_common(30):
        print(f"{100 * v / n:5.1f}%  {k}")
