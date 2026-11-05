"""Worst cases: snapshots with many updated entities. Scenario A clones real player-entity updates (crowded but realistic),
scenario B sets every field of every entity to a random value (absolute upper bound). New codec vs the engine's own format,
fragmentation, and input files for the C CPU benchmark."""
import sys, os, random, struct, collections, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, bench3 as B3, codec as C, codec2 as K2, fullload, fuzz, engine_enc as E, bdata

B.ANG16 = True
RAW = fullload.load_all()
obj = bdata.load_all(); depth = bdata.depth_table(obj["dec"]); dec = obj["dec"]
FRAG = 1300
OUTDIR = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data"))

def snap_of(msg):
    for op in msg["ops"]:
        if op["t"] == "snap": return op["snap"]

def train():
    msgs = RAW["5min"]; cut = int(len(msgs) * 0.6)
    items = B.data["5min"]; st5 = B.states["5min"]
    modes = B.choose_modes(items[:int(len(items) * 0.6)], st5, 0, None)
    counts = {}; store = {}; bs = {}
    for m in msgs[:cut]:
        seq = m["seq"]; msg = m["msg"]; delta = m["delta"]
        base = bh = None
        if delta:
            if seq - delta not in store: continue
            base, bh = store[seq - delta]
        io = C.EncIO(); hdr = K2.msg_put(io, msg, base, bh, bs, modes, delta)
        for op in msg["ops"]:
            if op["t"] == "gs": bs = dict(op["baselines"])
        sn = snap_of(msg)
        if sn is not None: store[seq] = (sn, hdr)
        for ctx, Kk, sym in io.u:
            if ctx is None: continue
            e = counts.get(ctx)
            if e is None: e = counts[ctx] = [collections.Counter(), 0]
            e[0][sym] += 1; e[1] += 1
    return modes, C.Tables(counts, 0.01), msgs[cut:]

def real_pairs(test):
    """(old, new) states of player entities updated between consecutive real snapshots, and (old_ps, new_ps)"""
    store = {}; pairs = []; ps_pairs = []
    for m in test:
        sn = snap_of(m["msg"])
        if sn is None: continue
        base = store.get(m["seq"] - m["delta"]) if m["delta"] else None
        if base:
            old = dict(base["ents"])
            for num, st in sn["ents"]:
                if num in old and C.ent_changes(st, old[num]) and st.get("eType", 0) == 1: pairs.append((old[num], st))
            ps_pairs.append((base["ps"], sn["ps"]))
        store[m["seq"]] = sn
    return pairs, ps_pairs

def scenario_A(rng, n, pairs, ps_pairs):
    bps, nps = rng.choice(ps_pairs)
    olds = []; news = []
    for j in range(n):
        o, nw = rng.choice(pairs); num = 100 + j
        olds.append((num, dict(o, number=num))); news.append((num, dict(nw, number=num)))
    return dict(ps=bps, vps=None, ents=olds), dict(ps=nps, vps=None, ents=news)

def worst_val(rng, bits):
    if bits == 0:
        v = rng.getrandbits(32)
        return v if v != 0x80000000 else 1
    ab = abs(bits); M = 1 << ab
    return rng.randint(-M // 2, M // 2 - 1) if bits < 0 else rng.randint(0, M - 1)

def scenario_B(rng, n):
    def rstate(fields): return {nm: worst_val(rng, bits) for nm, bits in fields}
    bps = rstate(E.PS); nps = rstate(E.PS)
    bps["m_iVehicleNum"] = 0; nps["m_iVehicleNum"] = 0          # no vehicle: the engine then sends no vps and no pilot playerstate
    olds = []; news = []
    for j in range(n):
        num = 10 + j
        olds.append((num, dict(rstate(E.EF), number=num))); news.append((num, dict(rstate(E.EF), number=num)))
    return dict(ps=bps, vps=None, ents=olds), dict(ps=nps, vps=None, ents=news)

def measure(base, new, modes, tables):
    op = dict(t="snap", time=100000, delta=1, flags=0, areas=[255], snap=new)
    msg = dict(ack=5, ops=[op])
    bh = dict(ack=4, time=99950, cmdseq=0)
    io = C.EncIO(); hdr = K2.msg_put(io, msg, base, bh, {}, modes, 1)
    buf = C.rans_encode(io.u, tables)
    dmsg, dhdr, dsn, _ = K2.msg_get(C.DecIO(buf, tables), lambda d: (base, bh), {}, modes)
    ok = K2.canon_msg(dmsg) == K2.canon_msg(msg)
    calls = E.snapshot_calls(5, op, base, {})
    eng_bits = E.bits_of(calls, depth)
    nsym = sum(1 for u in io.u if u[0] is not None)
    return len(buf), (eng_bits + 7) // 8, nsym, len(calls), ok, io.u, calls

def dump(path, tables, items):
    """items: list of (units, calls) -> JKB1 file for jkabench.exe"""
    left, right, sym, root = dec
    parent = [-1] * len(left)
    for i in range(len(left)):
        if sym[i] == 257: parent[left[i]] = i; parent[right[i]] = i
    codes = {}
    def walk(n, path_):
        if sym[n] == 257: walk(left[n], path_ + [0]); walk(right[n], path_ + [1])
        else: codes[sym[n]] = path_
    walk(root, [])
    ctx_id = {}; ctx_K = {}
    for units, calls in items:
        for ctx, K, s in units:
            if ctx is not None and ctx not in ctx_id: ctx_id[ctx] = len(ctx_id); ctx_K[ctx] = K
    Bn = B3.Binarizer(tables.M, 0.01)
    node_id = {}; pinit = []; snaps = []
    for units, calls in items:
        syms = []; decs = []
        for ctx, K, s in units:
            if ctx is None:
                if syms: c, sy, rw = syms[-1]; syms[-1] = (c, sy, min(255, rw + K))
                continue
            syms.append((ctx_id[ctx], s, 0))
            nb = max(1, (K - 1).bit_length()); prefix = 0
            for d in range(nb):
                bit = (s >> (nb - 1 - d)) & 1; key = (ctx, d, prefix)
                if key not in node_id:
                    node_id[key] = len(pinit)
                    pinit.append(max(1, min(4095, round(Bn.node_p(ctx, nb, d, prefix) * 4096))))
                decs.append((node_id[key], bit)); prefix = (prefix << 1) | bit
        bits = []
        for b, v in calls:
            ab = abs(b); nb = ab & 7
            for i in range(nb): bits.append((v >> i) & 1)
            for i in range((ab - nb) // 8): bits += codes[(v >> (nb + 8 * i)) & 255]
        pk = bytearray((len(bits) + 7) // 8)
        for i, bt in enumerate(bits):
            if bt: pk[i >> 3] |= 1 << (i & 7)
        snaps.append((syms, decs, [(b, v & 0xFFFFFFFF) for b, v in calls], bytes(pk), len(bits)))
    with open(path, "wb") as f:
        w = f.write
        w(b"JKB1"); w(struct.pack("<IIII", len(ctx_id), len(pinit), len(snaps), len(left)))
        for ctx, cid in sorted(ctx_id.items(), key=lambda x: x[1]):
            K = ctx_K[ctx]; fr = tables.T.freqs(ctx, K, 12)
            w(struct.pack("<I", K)); w(struct.pack("<%dH" % K, *fr)); w(bytes(min(31, tables.T.hcost(ctx, K, i)) for i in range(K)))
        w(struct.pack("<%dH" % len(pinit), *pinit))
        for i in range(len(left)):
            w(struct.pack("<iiii", left[i] if sym[i] == 257 else -1, right[i] if sym[i] == 257 else -1, sym[i] if sym[i] != 257 else -1, parent[i]))
        w(struct.pack("<I", root))
        for syms, decs, calls, pk, nbits in snaps:
            w(struct.pack("<I", len(syms)))
            for c, sy, rw in syms: w(struct.pack("<HHB", c, sy, rw))
            w(struct.pack("<I", len(decs)))
            for n, bt in decs: w(struct.pack("<IB", n, bt))
            w(struct.pack("<I", len(calls)))
            for b, v in calls: w(struct.pack("<bI", b, v))
            w(struct.pack("<II", nbits, len(pk))); w(pk)

def main():
    modes, tables, test = train()
    pairs, ps_pairs = real_pairs(test)
    print("real player-entity update pairs: %d" % len(pairs))
    rng = random.Random(7)
    print("%-26s %7s | %9s %5s | %9s %5s | %8s %8s | %s" % ("scenario", "round-trip", "new bytes", "frag", "engine B", "frag", "symbols", "calls", ""))
    dumps = {}
    for label, make, ns in (("A cloned real updates", lambda n: scenario_A(rng, n, pairs, ps_pairs), (16, 32, 64, 128, 256)),
                            ("B every field random", lambda n: scenario_B(rng, n), (32, 128, 512, 1000))):
        for n in ns:
            reps = 3 if n >= 512 else 6
            res = []
            for _ in range(reps):
                base, new = make(n)
                res.append(measure(base, new, modes, tables))
            ok = all(r[4] for r in res)
            nb = sum(r[0] for r in res) / reps; eb = sum(r[1] for r in res) / reps
            print("%-26s %7s | %9.0f %5d | %9.0f %5d | %8.0f %8.0f | %.2f x engine" % ("%s, N=%d" % (label[:1], n), "ok" if ok else "FAILED", nb, int(nb) // FRAG + 1, eb, int(eb) // FRAG + 1,
                  sum(r[2] for r in res) / reps, sum(r[3] for r in res) / reps, nb / eb))
            dumps["%s%d" % (label[0], n)] = [(r[5], r[6]) for r in res]
    for name in ("A32", "A128", "B128", "B1000"):
        dump(os.path.join(OUTDIR, "bench_worst_%s.bin" % name), tables, dumps[name])
    print("wrote bench_worst_{A32,A128,B128,B1000}.bin in", OUTDIR)

if __name__ == "__main__":
    main()
