"""Export symbol streams + tables for the C CPU benchmark (E:\\Git\\jka\\jediacademy\\tmp\\cpubench\\bench_input.bin)."""
import struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, bench3 as B3, bdata

OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "bench_input.bin"))
NSNAP = 1000

obj = bdata.load_all(); dec = obj["dec"]
left, right, sym, root = dec
parent = [-1] * len(left)
for i in range(len(left)):
    if sym[i] == 257:          # INTERNAL
        parent[left[i]] = i; parent[right[i]] = i
codes = {}
def walk(n, path):
    if sym[n] == 257:
        walk(left[n], path + [0]); walk(right[n], path + [1])
    else:
        codes[sym[n]] = path
walk(root, [])

items = B.data["5min"]; st5 = B.states["5min"]
cut = int(len(items) * 0.6)
train = items[:cut]; test = items[cut:]
step = max(1, len(test) // NSNAP)
test = test[::step][:NSNAP]
af, as_ = B.tune_alpha(items[:cut], st5, 0, "gap", False, None)
cfg = dict(modes=B.choose_modes(items[:cut], st5, 0, None), quant=None, variant="gap", valcls=False)
M = B3.train_model(B3.event_lists(items[:cut], st5, 0, cfg))
T = B3.Tables(M, as_); Bn = B3.Binarizer(M, as_)

# event lists for the chosen test snapshots (need seq neighbours for the base: use the full states dict)
E = []; keep = []
prevhdr = None
for s in test:
    r, prevhdr = B.snap_events(0, [s], st5, 0, cfg, None)
    if r is None: continue
    E.append(list(r[0]) + list(r[1])); keep.append(s)

ctx_id = {}; ctx_K = {}
for evs in E:
    for kind, ctx, K, sym_, raw in evs:
        if ctx not in ctx_id: ctx_id[ctx] = len(ctx_id); ctx_K[ctx] = K
node_id = {}; pinit = []
snaps = []
for s, evs in zip(keep, E):
    syms = [(ctx_id[c], sy, raw) for kind, c, K, sy, raw in evs]
    decs = []
    for kind, c, K, sy, raw in evs:
        nb = max(1, (K - 1).bit_length()); prefix = 0
        for d in range(nb):
            bit = (sy >> (nb - 1 - d)) & 1
            key = (c, d, prefix)
            if key not in node_id:
                node_id[key] = len(pinit)
                pinit.append(max(1, min(4095, round(Bn.node_p(c, nb, d, prefix) * 4096))))
            decs.append((node_id[key], bit))
            prefix = (prefix << 1) | bit
    calls = [(b, v & 0xFFFFFFFF) for t, b, v in s["log"]]
    # engine bitstream
    bits = []
    for b, v in calls:
        ab = abs(b); nb = ab & 7
        for i in range(nb): bits.append((v >> i) & 1)
        for i in range((ab - nb) // 8):
            bits += codes[(v >> (nb + 8 * i)) & 255]
    assert len(bits) == s["bits"], (len(bits), s["bits"])
    pk = bytearray((len(bits) + 7) // 8)
    for i, bt in enumerate(bits):
        if bt: pk[i >> 3] |= 1 << (i & 7)
    snaps.append((syms, decs, calls, bytes(pk), len(bits)))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "wb") as f:
    w = f.write
    w(b"JKB1")
    w(struct.pack("<IIII", len(ctx_id), len(pinit), len(snaps), len(left)))
    for ctx, cid in sorted(ctx_id.items(), key=lambda x: x[1]):
        K = ctx_K[ctx]
        fr = T.freqs(ctx, K, 12)
        assert sum(fr) == 4096 and min(fr) >= 1
        w(struct.pack("<I", K))
        w(struct.pack("<%dH" % K, *fr))
        w(bytes(T.hcost(ctx, K, i) for i in range(K)))
    w(struct.pack("<%dH" % len(pinit), *pinit))
    for i in range(len(left)):
        w(struct.pack("<iiii", left[i] if sym[i] == 257 else -1, right[i] if sym[i] == 257 else -1,
                      sym[i] if sym[i] != 257 else -1, parent[i]))
    w(struct.pack("<I", root))
    for syms, decs, calls, pk, nbits in snaps:
        w(struct.pack("<I", len(syms)))
        for c, sy, raw in syms: w(struct.pack("<HHB", c, sy, raw))
        w(struct.pack("<I", len(decs)))
        for n, bt in decs: w(struct.pack("<IB", n, bt))
        w(struct.pack("<I", len(calls)))
        for b, v in calls: w(struct.pack("<bI", b, v))
        w(struct.pack("<II", nbits, len(pk))); w(pk)
print("snapshots", len(snaps), "contexts", len(ctx_id), "nodes", len(pinit), "file bytes", os.path.getsize(OUT))
print("mean symbols/snap %.0f decisions/snap %.0f calls/snap %.0f" % (
    sum(len(x[0]) for x in snaps) / len(snaps), sum(len(x[1]) for x in snaps) / len(snaps), sum(len(x[2]) for x in snaps) / len(snaps)))
