"""Synthetic structurally valid server messages (all ops, vehicles, pilot playerstates, extreme values, download blocks,
random base ages and 'no base') -> whole-message codec round trip. Proves correctness, says nothing about compression ratios."""
import sys, os, random, struct, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, codec as C, codec2 as K2

B.ANG16 = True
EF = B.EF; NE = len(EF)
PT = K2.PS_TABLES
A16 = 360.0 / 65536
def pk(x): return struct.unpack("<I", struct.pack("<f", x))[0]

def rand_int(rng, bits):
    ab = abs(bits); M = 1 << ab
    lo, hi = (-M // 2, M // 2 - 1) if bits < 0 else (0, M - 1)
    r = rng.random()
    if r < 0.2: return 0
    if r < 0.4: return max(lo, min(hi, rng.randint(-8, 8)))
    if r < 0.5: return rng.choice([lo, hi])
    if r < 0.7: return max(lo, min(hi, rng.randint(-300, 300)))
    return rng.randint(lo, hi)

def rand_float(rng):
    r = rng.random()
    if r < 0.15: return 0
    if r < 0.35: return pk(float(rng.randint(-4100, 4100)))
    if r < 0.55: return pk(rng.randint(-40000, 40000) * A16)
    if r < 0.60: return pk(rng.randint(-32768, 32767) * A16)
    if r < 0.65: return pk(65536 * A16)
    if r < 0.70: return rng.choice([0x7F800000, 0xFF800000, 0x7FC00000, 0xFFC00001, 0x00000001, 0x80000001, 0x7F7FFFFF])
    if r < 0.85: return pk(rng.uniform(-9000, 9000))
    v = rng.getrandbits(32)
    return v if v != 0x80000000 else 0

def rand_val(rng, bits): return rand_float(rng) if bits == 0 else rand_int(rng, bits)

def mutate_ps(rng, ps, tab, nmut):
    ps = dict(ps); fields = PT[tab]
    for _ in range(nmut):
        nm, bits = rng.choice(fields); ps[nm] = rand_val(rng, bits)
    return ps

def mutate_ent(rng, st, nmut):
    st = dict(st)
    for _ in range(nmut):
        nm, bits = rng.choice(EF); st[nm] = rand_val(rng, bits)
    return st

def rand_baselines(rng):
    return {n: mutate_ent(rng, {}, rng.randint(1, 8)) | {"number": n} for n in rng.sample(range(0, 1023), rng.randint(0, 30))}

def next_snapshot(rng, base, baselines):
    """a new snapshot state derived from the base (the state the client holds), valid under the engine's rules"""
    bps = base["ps"] if base else {}
    bvps = (base["vps"] or {}) if base else {}
    veh = rng.random() < 0.25
    pilot = veh and rng.random() < 0.6
    ps = mutate_ps(rng, bps, 0, rng.randint(0, 25))                       # start from the normal table
    ps["m_iVehicleNum"] = rng.randint(1, 1022) if veh else 0
    ps["eFlags"] = (ps.get("eFlags", 0) | K2.EF_NODRAW) if pilot else (ps.get("eFlags", 0) & ~K2.EF_NODRAW & 0xFFFFFFFF)
    if pilot:                                                              # pilot table: other fields keep the old value
        names = {n for n, b in PT[1]}
        for n, b in PT[0]:
            if n not in names: ps[n] = bps.get(n, 0)
        for n in list(ps):
            if n not in names and n not in {x for x, y in PT[0]} and n not in {x for x, y in PT[2]}: pass
    vps = mutate_ps(rng, bvps, 2, rng.randint(0, 20)) if veh else None
    old = dict(base["ents"]) if base else {}
    ents = dict(old)
    for num in list(ents):
        r = rng.random()
        if r < 0.10: del ents[num]
        elif r < 0.45: ents[num] = mutate_ent(rng, ents[num], rng.randint(1, 10))
    for _ in range(rng.randint(0, 25)):
        num = rng.randint(0, 1022)
        if num in ents: continue
        r = rng.random()
        if r < 0.15: ents[num] = dict(baselines.get(num, {}))                # new entity identical to its baseline
        else: ents[num] = mutate_ent(rng, baselines.get(num, {}), rng.randint(1, 20))
        ents[num]["number"] = num
    for n, s in ents.items(): s["number"] = n
    return dict(ps=ps, vps=vps, ents=sorted(ents.items()))

def run(seed=1, nstreams=30, nmsg=120):
    rng = random.Random(seed)
    modes = {n: rng.choice("ad") for n, b in EF} | {n: rng.choice("ad") for t in PT.values() for n, b in t}
    # training pass for skewed tables (same generator, other seed) so that the test uses non-uniform probabilities
    def make_stream(rng):
        msgs = []; store = {}; baselines = rand_baselines(rng); seq = 100; cmdseq = 0
        ops0 = [dict(t="gs", rseq=rng.randint(0, 5000), cs=[(i, bytes(rng.randint(1, 255) for _ in range(rng.randint(0, 40)))) for i in sorted(rng.sample(range(0, 1700), rng.randint(0, 40)))],
                     baselines=sorted(baselines.items()), clientNum=rng.randint(0, 31), checksum=rng.randint(-2**31, 2**31 - 1))]
        msgs.append(dict(seq=seq, msg=dict(ack=rng.randint(0, 10**6), ops=ops0), delta=0)); seq += 1
        for _ in range(nmsg):
            ops = []
            for _ in range(rng.choice([0, 0, 0, 1, 2, 4])):
                cmdseq += 1; ops.append(dict(t="cmd", seq=cmdseq, text=bytes(rng.randint(1, 255) for _ in range(rng.randint(0, 80)))))
            if rng.random() < 0.02: ops.append(dict(t="setgame", text=b"base"))
            if rng.random() < 0.01: ops.append(dict(t="nop"))
            r = rng.random()
            delta = 0
            if r < 0.85:
                cand = [d for d in range(1, 12) if seq - d in store]
                delta = rng.choice(cand) if cand and rng.random() > 0.08 else 0
                base = store[seq - delta][0] if delta else None
                sn = next_snapshot(rng, base, baselines)
                ops.append(dict(t="snap", time=rng.randint(0, 10**7), delta=delta, flags=rng.randint(0, 255),
                                areas=[rng.randint(0, 255) for _ in range(rng.randint(0, 6))], snap=sn))
            if rng.random() < 0.03:
                blk = rng.randint(0, 65535)
                ops.append(dict(t="dl", block=blk, fsize=rng.randint(1, 10**7) if blk == 0 else None, data=bytes(rng.randint(0, 255) for _ in range(rng.randint(0, 300)))))
            if rng.random() < 0.01: ops.append(dict(t="mapchange"))
            msg = dict(ack=rng.randint(-2**31, 2**31 - 1), ops=ops)
            msgs.append(dict(seq=seq, msg=msg, delta=delta))
            sn = next((o["snap"] for o in ops if o["t"] == "snap"), None)
            if sn is not None: store[seq] = (sn, None)
            seq += 1
        return msgs
    streams = [make_stream(random.Random(seed * 1000 + i)) for i in range(nstreams)]
    return modes, streams

def check(modes, streams, tables):
    bad = 0; n = 0; kinds = collections.Counter(); sizes = []
    for msgs in streams:
        store_s = {}; store_c = {}; bs = {}; bc = {}
        for m in msgs:
            seq = m["seq"]; msg = m["msg"]; delta = m["delta"]
            base = bh = None
            if delta:
                base, bh = store_s[seq - delta]
            io = C.EncIO()
            hdr = K2.msg_put(io, msg, base, bh, bs, modes, delta)
            buf = C.rans_encode(io.u, tables)
            lookup = lambda d: (None, None) if d == 0 else store_c[seq - d]
            dmsg, dhdr, dsn, bc = K2.msg_get(C.DecIO(buf, tables), lookup, bc, modes)
            ok = K2.canon_msg(dmsg) == K2.canon_msg(msg) and dhdr == hdr
            n += 1; sizes.append(len(buf))
            for op in msg["ops"]:
                kinds[op["t"]] += 1
                if op["t"] == "snap":
                    kinds["snap:pilot" if K2.is_pilot(op["snap"]["ps"]) else ("snap:vehicle" if op["snap"]["ps"].get("m_iVehicleNum") else "snap:normal")] += 1
                    kinds["snap:nobase" if delta == 0 else "snap:base-age>1" if delta > 1 else "snap:base-age1"] += 1
                if op["t"] == "gs": bs = dict(op["baselines"])
            sn = next((o["snap"] for o in msg["ops"] if o["t"] == "snap"), None)
            if sn is not None:
                store_s[seq] = (sn, hdr); store_c[seq] = (dsn, dhdr)
            if not ok:
                bad += 1
                if bad <= 3: print("   MISMATCH seq", seq, [o["t"] for o in msg["ops"]], "delta", delta)
    return n, bad, kinds, sizes

if __name__ == "__main__":
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    modes, streams = run(seed)
    # tables: uniform (empty) first, then trained on half of the streams
    n, bad, kinds, sizes = check(modes, streams, C.Tables({}, 0.01))
    print("uniform tables: messages %d, failures %d" % (n, bad)); print("   coverage:", dict(kinds))
    counts = {}
    for msgs in streams[:15]:
        store = {}; bs = {}
        for m in msgs:
            base = bh = None
            if m["delta"]: base, bh = store[m["seq"] - m["delta"]]
            io = C.EncIO(); hdr = K2.msg_put(io, m["msg"], base, bh, bs, modes, m["delta"])
            for op in m["msg"]["ops"]:
                if op["t"] == "gs": bs = dict(op["baselines"])
            sn = next((o["snap"] for o in m["msg"]["ops"] if o["t"] == "snap"), None)
            if sn is not None: store[m["seq"]] = (sn, hdr)
            for ctx, Kk, sym in io.u:
                if ctx is None: continue
                e = counts.get(ctx)
                if e is None: e = counts[ctx] = [collections.Counter(), 0]
                e[0][sym] += 1; e[1] += 1
    n, bad, kinds, sizes = check(modes, streams[15:], C.Tables(counts, 0.01))
    print("trained tables:  messages %d, failures %d" % (n, bad))
