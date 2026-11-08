# Report of a samply profile (Windows, ETW) of netcodec_bench_codec: samples split by phase (netcodec encode, decode,
# legacy, other) from the function-level symbols of samply's .syms.json sidecar, then for one phase the innermost
# function, the inline chain and the source line of each sample, symbolised with llvm-symbolizer on the exe's PDB.
# Build the bench with both debug formats (the PDB for samply, DWARF for llvm-symbolizer, which ignores the PDB when the
# MinGW runtime brings DWARF): cmake -S codemp/netcodec -B build-netcodec/prof -G Ninja -DCMAKE_BUILD_TYPE=Release
#   -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_FLAGS="-g -gdwarf -gcodeview"
#   -DCMAKE_EXE_LINKER_FLAGS="-Wl,--pdb=", then with xperf (Windows Performance Toolkit) and llvm-symbolizer in the PATH:
#   samply record --save-only --unstable-presymbolicate --main-thread-only -r 8000 -o enc.json.gz -- <exe> <trace> 5
# (the samples cover the cold and the warm passes; `samply load enc.json.gz` shows the flame graph in a browser).
# Usage: python -I samply_report.py <profile.json.gz> <profile.json.syms.json> <exe> [encode|decode|legacy|other] [top]
import collections, gzip, json, re, subprocess, sys

prof_path, syms_path, exe = sys.argv[1], sys.argv[2], sys.argv[3]
want = sys.argv[4] if len(sys.argv) > 4 else "encode"
top = int(sys.argv[5]) if len(sys.argv) > 5 else 30
prof = json.load(gzip.open(prof_path))
syms = json.load(open(syms_path))
th = prof["threads"][0]
strings = th["stringArray"]
libs = prof["libs"]
res_lib = th["resourceTable"]["lib"]
ft, fn, st = th["frameTable"], th["funcTable"], th["stackTable"]

# function-level symbols per library debug name
symtab = {}
for d in syms["data"]:
    rows = sorted((e["rva"], e["size"], syms["string_table"][e["symbol"]]) for e in d["symbol_table"])
    symtab[d["debug_name"]] = rows
import bisect
def fsym(libidx, rva):
    lib = libs[libidx]
    rows = symtab.get(lib.get("debugName"))
    if not rows:
        return lib.get("name", "?")
    i = bisect.bisect_right(rows, (rva, 1 << 62, "")) - 1
    if i >= 0 and rows[i][0] <= rva < rows[i][0] + max(rows[i][1], 1):
        return rows[i][2]
    return lib.get("name", "?")

def frame_lib_rva(f):
    r = fn["resource"][ft["func"][f]]
    li = res_lib[r] if r is not None and r >= 0 else None
    return li, ft["address"][f]

exe_lib = next(i for i, l in enumerate(libs) if l.get("name", "").lower() == "netcodec_bench_codec.exe")
samples = th["samples"]
weights = samples.get("weight") or [1] * samples["length"]
phase_cnt = collections.Counter()
leaf_rvas = collections.Counter()
for s, w in zip(samples["stack"], weights):
    if s is None:
        continue
    names = []
    leaf = None
    k = s
    while k is not None:
        f = st["frame"][k]
        li, rva = frame_lib_rva(f)
        if leaf is None:
            leaf = (li, rva)
        if li is not None and rva >= 0:
            names.append(fsym(li, rva))
        k = st["prefix"][k]
    joined = " | ".join(names)
    if "MessageEncoder::Encode" in joined:
        ph = "encode"
    elif "DecodeMessage" in joined:
        ph = "decode"
    elif "legacy::" in joined or "MSG_" in joined or "Huff" in joined:
        ph = "legacy"
    else:
        ph = "other"
    phase_cnt[ph] += w
    if ph == want and leaf[0] == exe_lib:
        leaf_rvas[leaf[1]] += w
    elif ph == want:
        leaf_rvas[("lib", libs[leaf[0]]["name"] if leaf[0] is not None else "?")] += w

total = sum(phase_cnt.values())
print("samples by phase:", ", ".join(f"{k} {100 * v / total:.1f}%" for k, v in phase_cnt.most_common()))
n = phase_cnt[want]
addrs = sorted(a for a in leaf_rvas if isinstance(a, int))
# the exe's image base from its PE header (0x140000000 for x64, 0x400000 for x86 by default)
pe = open(exe, "rb").read(4096)
opt = int.from_bytes(pe[0x3C:0x40], "little") + 24
image_base = int.from_bytes(pe[opt + 24:opt + 32], "little") if pe[opt:opt + 2] == b"\x0b\x02" else int.from_bytes(pe[opt + 28:opt + 32], "little")
out = subprocess.run(["llvm-symbolizer", "--obj=" + exe, "--functions=short", "--demangle", "--inlining"],
                     input="\n".join(hex(image_base + a) for a in addrs) + "\n", capture_output=True, text=True).stdout
blocks = out.strip("\n").split("\n\n")
inner, chain, line = collections.Counter(), collections.Counter(), collections.Counter()
for a, blk in zip(addrs, blocks):
    ls = blk.split("\n")
    fr = [(re.sub(r"\(.*", "", ls[i]).strip(), ls[i + 1].strip() if i + 1 < len(ls) else "") for i in range(0, len(ls) - 1, 2)]
    w = leaf_rvas[a]
    inner[fr[0][0]] += w
    chain[" < ".join(f[0][:45] for f in fr[:3])] += w
    loc = re.sub(r".*[\\/]", "", fr[0][1])
    loc = re.sub(r":(\d+):\d+$", r":\1", loc)
    line[f"{loc}  [{fr[0][0]}]"] += w
for a, w in leaf_rvas.items():
    if not isinstance(a, int):
        inner["(outside the exe) " + a[1]] += w
print(f"=== {want}: {n} samples")
for title, c in (("innermost function", inner), ("inline chain", chain), ("source line", line)):
    print("-- " + title)
    for k, v in c.most_common(top):
        print(f"{100 * v / n:5.1f}%  {k}")
