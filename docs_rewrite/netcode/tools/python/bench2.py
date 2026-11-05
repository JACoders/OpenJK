"""New-schema models: delta vs a chosen base snapshot, per-field contexts, no lc, static trained distributions.
Costs are ideal code lengths under smoothed static counts (what a static rANS/arith/tANS coder approaches),
plus an explicit per-packet coder overhead added at report time."""
import math, struct, collections, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bdata

obj = bdata.load_all()
T = obj["T"]; data = obj["data"]; states = obj["states"]
EF = T["ent"]; PF = list(T["ps"])
ET_IDX = [i for i, (n, b) in enumerate(EF) if n == "eType"][0]
for a in bdata.J.FIELDS["arrays"]:          # stats, persistant, ammo, powerups; widths unsigned here
    sp = a.get("special")
    for i in range(a["count"]):
        PF.append(("%s[%d]" % (a["name"], i), sp["bits"] if sp and i == sp["index"] else abs(a["bits"])))
NE = len(EF); NP = len(PF)

def zz(d): return (d << 1) if d >= 0 else ((-d << 1) - 1)
def cls_of(et): return et if et < 18 else 18
def f32(b): return struct.unpack("<f", struct.pack("<I", b & 0xFFFFFFFF))[0]
def s32(b): return b - (1 << 32) if b >= (1 << 31) else b

def fkind(b):
    if b == 0: return 0
    f = f32(b)
    if f == f and -4096 <= f < 4096 and f == int(f): return 1
    return 2

# ------------------------------------------------------------------ tokenizers
def tok_int(name, bits, nv, ov, mode):
    ab = abs(bits)
    if mode == "a":
        if ab <= 8:
            return [((name, "a"), 1 << ab, nv & ((1 << ab) - 1), 0)]
        x = zz(nv) if bits < 0 else nv
        c = x.bit_length()
        return [((name, "a"), ab + 2, c, c - 1 if c else 0)]
    m = 1 << ab
    d = nv - ov
    d = ((d + m // 2) % m) - m // 2
    c = zz(d).bit_length()
    return [((name, "d"), ab + 2, c, c - 1 if c else 0)]

ANG16 = False                 # lossless: floats that are exactly k*360/65536 are sent as the 16-bit index k
POSQ = None                   # lossy: scale for position fields only (None = exact)
POS_FIELDS = {"pos.trBase[0]", "pos.trBase[1]", "pos.trBase[2]", "origin[0]", "origin[1]", "origin[2]"}
A16 = 360.0 / 65536

def is_a16(b):
    f = f32(b)
    if f != f or abs(f) > 720: return False
    kq = int(math.floor(f / A16 + 0.5))
    return struct.unpack("<I", struct.pack("<f", kq * A16))[0] == (b & 0xFFFFFFFF)

def old_index(ov):
    f = f32(ov)
    return int(math.floor(f / A16 + 0.5)) if (ov != 0 and f == f and abs(f) <= 720) else 0

def tok_float(name, nv, ov, mode, quant):
    k = fkind(nv)
    if ANG16 and k == 2 and is_a16(nv): k = 3
    if POSQ and not quant and name in POS_FIELDS and k >= 1:
        quant = POSQ
        if k == 3: k = 2
    out = [((name, "k"), 4 if ANG16 else 3, 2 if (k == 3 and quant) else k, 0)]
    if k == 0: return out
    ok = fkind(ov)
    if k == 3:
        d = int(math.floor(f32(nv) / A16 + 0.5)) - old_index(ov)
        c = zz(d).bit_length()
        out.append(((name, "ad"), 18, c, c - 1 if c else 0))
        return out
    if quant:
        q = round(f32(nv) * quant); oq = round(f32(ov) * quant) if ok else 0
        c = zz(q - oq).bit_length()
        out.append(((name, "q"), 36, c, c - 1 if c else 0))
        return out
    if k == 1:
        t = int(f32(nv)); ot = int(f32(ov)) if ok == 1 else 0
        if mode == "a":
            c = zz(t).bit_length(); out.append(((name, "ia"), 16, c, c - 1 if c else 0))
        else:
            c = zz(t - ot).bit_length(); out.append(((name, "id"), 16, c, c - 1 if c else 0))
    else:
        if mode == "a":
            out.append(((name, "fr"), 1, 0, 32))
        else:
            d = s32(nv) - (s32(ov) if ok else 0)
            c = zz(d).bit_length(); out.append(((name, "fd"), 36, c, c - 1 if c else 0))
    return out

def tokens(name, bits, nv, ov, modes, quant):
    mode = modes.get(name, "d")
    if bits == 0: return tok_float(name, nv, ov, mode, quant)
    return tok_int(name, bits, nv, ov, mode)

# ------------------------------------------------------------------ event generation
def ent_changes(new, old):
    out = []
    g = new.get; h = old.get
    for i in range(NE):
        name = EF[i][0]
        a = g(name, 0); b = h(name, 0)
        if a != b: out.append((i, name, EF[i][1], a, b))
    return out

def gen(snap, base, baselines, cfg):
    """yield (kind, ctx, K, sym, raw). kind F=flag S=symbol"""
    modes = cfg["modes"]; quant = cfg["quant"]; variant = cfg["variant"]; vc = cfg["valcls"]
    # ---- playerstate
    nps = snap["ps"]; ops = base["ps"] if base else {}
    chg = []
    g = nps.get; h = ops.get
    for i in range(NP):
        name = PF[i][0]
        a = g(name, 0); b = h(name, 0)
        if a != b: chg.append((i, name, PF[i][1], a, b))
    if variant == "flag":
        ci = {c[0]: c for c in chg}
        for i in range(NP):
            c = ci.get(i)
            yield ("F", ("pf", i), 2, 1 if c else 0, 0)
            if c:
                for ctx, K, sym, raw in tokens(c[1], c[2], c[3], c[4], modes, quant):
                    yield ("S", ("pv",) + ctx, K, sym, raw)
    else:
        prev = -1
        for c in chg:
            yield ("S", ("pg", prev), NP + 1, c[0], 0)
            prev = c[0]
            for ctx, K, sym, raw in tokens(c[1], c[2], c[3], c[4], modes, quant):
                yield ("S", ("pv",) + ctx, K, sym, raw)
        yield ("S", ("pg", prev), NP + 1, NP, 0)
    # ---- entities
    new = dict(snap["ents"]); old = dict(base["ents"]) if base else {}
    nums = sorted(set(new) | set(old))
    prevnum = -1; prevop = 0
    for num in nums:
        if num in new and num in old:
            ch = ent_changes(new[num], old[num])
            if not ch: continue
            op = 0; basest = old[num]; st = new[num]
        elif num in new:
            bs = baselines.get(num, {})
            ch = ent_changes(new[num], bs)
            op = 1 if ch else 3; basest = bs; st = new[num]
        else:
            op = 2; ch = []; basest = old[num]; st = None
        gap = num - prevnum - 1; prevnum = num
        yield ("S", ("eg",), 42, gap if gap < 40 else 40, 10 if gap >= 40 else 0)
        yield ("S", ("eo", prevop), 4, op, 0)
        prevop = op
        if op in (2, 3): continue
        bcls = cls_of(basest.get("eType", 0))
        ncls = cls_of(st.get("eType", 0))
        # eType first
        chd = {c[0]: c for c in ch}
        c = chd.get(ET_IDX)
        yield ("F", ("ef", "et", bcls), 2, 1 if c else 0, 0)
        if c:
            yield ("S", ("ev", "et", bcls), 256, c[3], 0)
        rest = [x for x in ch if x[0] != ET_IDX]
        if variant == "flag":
            ri = {x[0]: x for x in rest}
            for i in range(NE):
                if i == ET_IDX: continue
                x = ri.get(i)
                yield ("F", ("ef", ncls, i), 2, 1 if x else 0, 0)
                if x:
                    for ctx, K, sym, raw in tokens(x[1], x[2], x[3], x[4], modes, quant):
                        yield ("S", ("ev", ncls if vc else -1) + ctx, K, sym, raw)
        else:
            prev = -1
            for x in rest:
                yield ("S", ("eg2", ncls, prev), NE + 1, x[0], 0)
                prev = x[0]
                for ctx, K, sym, raw in tokens(x[1], x[2], x[3], x[4], modes, quant):
                    yield ("S", ("ev", ncls if vc else -1) + ctx, K, sym, raw)
            yield ("S", ("eg2", ncls, prev), NE + 1, NE, 0)
    yield ("S", ("eg",), 42, 41, 0)       # END of entity list

# header: ack delta, serverTime delta, delta byte, flags, areabytes
def header_ev_list(s, prev):
    evs = []
    L = {t: (b, v) for t, b, v in s["log"] if t[0] == "h"}
    ack = L[("h", "ack")][1]; tm = L[("h", "time")][1]
    pa = prev[0] if prev else ack; pt = prev[1] if prev else tm
    for name, d in (("ack", ack - pa), ("time", tm - pt)):
        c = zz(d).bit_length()
        evs.append(("S", ("h", name), 36, c, c - 1 if c else 0))
    evs.append(("S", ("h", "delta"), 256, L[("h", "delta")][1], 0))
    evs.append(("S", ("h", "flags"), 256, L[("h", "flags")][1], 0))
    areas = [v for t, b, v in s["log"] if t == ("h", "area")]
    for v in areas:
        evs.append(("S", ("h", "area"), 256, v, 0))
    return evs, (ack, tm)

# ------------------------------------------------------------------ model
class Model:
    def __init__(self): self.c = {}
    def add(self, ctx, sym):
        e = self.c.get(ctx)
        if e is None: e = self.c[ctx] = [collections.Counter(), 0]
        e[0][sym] += 1; e[1] += 1
    def cost(self, ctx, K, sym, a):
        e = self.c.get(ctx)
        if e is None: return math.log2(K)
        return -math.log2((e[0].get(sym, 0) + a) / (e[1] + a * K))

def base_for(seq, k, st):
    return st.get(seq - k)

def snap_events(i, items, st, k, cfg, prevhdr):
    s = items[i]
    if k == 0: base = st.get(s["seq"] - s["delta"]) if s["delta"] else None
    else: base = st.get(s["seq"] - k)
    if base is None: return None, prevhdr
    hev, hdr = header_ev_list(s, prevhdr)
    return (hev, gen(s["snap"], base, s["baselines"], cfg)), hdr

def choose_modes(train, st, k, quant):
    """per-field abs vs delta, by empirical entropy on train"""
    return pick_modes(mode_stats(train, st, k, quant))

def mode_stats(train, st, k, quant):
    """per field and mode: [Counter of (ctx, sym), raw bits]; stats of several streams add up (merge_mode_stats)"""
    samples = collections.defaultdict(list)
    cfg0 = dict(modes={}, quant=None, variant="gap", valcls=False)
    for s in train:
        base = st.get(s["seq"] - (k or s["delta"]))
        if base is None: continue
        for n, bs in (("p", None),):
            pass
        for i in range(NP):
            name, bits = PF[i]
            a = s["snap"]["ps"].get(name, 0); b = base["ps"].get(name, 0)
            if a != b: samples[name].append((bits, a, b))
        new = dict(s["snap"]["ents"]); old = dict(base["ents"])
        for num, stt in new.items():
            ch = ent_changes(stt, old.get(num) or s["baselines"].get(num, {}))
            for (i, name, bits, a, b) in ch: samples[name].append((bits, a, b))
    out = {}
    for name, lst in samples.items():
        out[name] = {}
        for mode in ("a", "d"):
            cnt = collections.Counter(); raw = 0
            for (bi, a, b) in lst:
                toks = tokens(name, bi, a, b, {name: mode}, quant)
                for ctx, K, sym, r in toks:
                    cnt[(ctx, sym)] += 1; raw += r
            out[name][mode] = [cnt, raw]
    return out

def merge_mode_stats(acc, st):
    for name, by in st.items():
        for mode, (cnt, raw) in by.items():
            e = acc.setdefault(name, {}).setdefault(mode, [collections.Counter(), 0])
            e[0].update(cnt); e[1] += raw
    return acc

def pick_modes(st):
    modes = {}
    for name, by in st.items():
        best = None
        for mode in ("a", "d"):
            cnt, raw = by[mode]
            ctxn = collections.Counter()
            for (ctx, sym), c in cnt.items(): ctxn[ctx] += c
            ent = 0.0
            for (ctx, sym), c in cnt.items(): ent -= c * math.log2(c / ctxn[ctx])
            tot = ent + raw + 6 * len(cnt)        # crude model-size penalty
            if best is None or tot < best[0]: best = (tot, mode)
        modes[name] = best[1]
    return modes

def evaluate(train, test, st_tr, st_te, k=0, variant="gap", valcls=False, quant=None, alphas=None):
    modes = choose_modes(train, st_tr, k, quant)
    cfg = dict(modes=modes, quant=quant, variant=variant, valcls=valcls)
    M = Model(); prevhdr = None; n_tr = 0
    for i in range(len(train)):
        r, prevhdr = snap_events(i, train, st_tr, k, cfg, prevhdr)
        if r is None: continue
        n_tr += 1
        for kind, ctx, K, sym, raw in r[0]: M.add(ctx, sym)
        for kind, ctx, K, sym, raw in r[1]: M.add(ctx, sym)
    # alpha choice on test is not allowed: use a fixed alpha pair chosen on a validation slice of train
    return M, cfg, n_tr

def cost_list(M, cfg, items, st, k, af, as_):
    out = []; nf = []; ns = []
    prevhdr = None
    for i in range(len(items)):
        r, prevhdr = snap_events(i, items, st, k, cfg, prevhdr)
        if r is None: continue
        bits = 0.0; cf = 0; cs = 0
        for kind, ctx, K, sym, raw in r[0]:
            bits += M.cost(ctx, K, sym, as_) + raw; cs += 1
        for kind, ctx, K, sym, raw in r[1]:
            if kind == "F": bits += M.cost(ctx, K, sym, af); cf += 1
            else: bits += M.cost(ctx, K, sym, as_) + raw; cs += 1
        out.append(bits); nf.append(cf); ns.append(cs)
    return out, nf, ns

def tune_alpha(train, st, k, variant, valcls, quant):
    a_tr = train[:int(len(train) * 0.75)]; a_va = train[int(len(train) * 0.75):]
    modes = choose_modes(a_tr, st, k, quant)
    cfg = dict(modes=modes, quant=quant, variant=variant, valcls=valcls)
    M = Model(); prevhdr = None
    for i in range(len(a_tr)):
        r, prevhdr = snap_events(i, a_tr, st, k, cfg, prevhdr)
        if r is None: continue
        for kind, ctx, K, sym, raw in r[0]: M.add(ctx, sym)
        for kind, ctx, K, sym, raw in r[1]: M.add(ctx, sym)
    best = None
    for af in (0.005, 0.02, 0.1, 0.5):
        for as_ in (0.003, 0.01, 0.03, 0.1, 0.5):
            c, _, _ = cost_list(M, cfg, a_va, st, k, af, as_)
            t = sum(c)
            if best is None or t < best[0]: best = (t, af, as_)
    return best[1], best[2]

def stats(bits, ovh=0):
    by = sorted(math.ceil(b / 8) + ovh for b in bits); n = len(by)
    return sum(by) / n, by[int(n * 0.95)], by[-1]

def run_cfg(label, k=0, variant="gap", valcls=False, quant=None, sets=("5min-test", "bots", "solo")):
    items = data["5min"]; st5 = states["5min"]
    cut = int(len(items) * 0.6)
    train = items[:cut]; test5 = items[cut:]
    af, as_ = tune_alpha(train, st5, k, variant, valcls, quant)
    M, cfg, n_tr = evaluate(train, test5, st5, st5, k, variant, valcls, quant)
    print("\n### %s  (k=%s variant=%s valcls=%s quant=%s  alphaF=%.2f alphaS=%.2f  train n=%d)" % (label, k or "actual", variant, valcls, quant, af, as_, n_tr))
    out = {}
    for name in sets:
        if name == "5min-test": items_, st_ = test5, st5
        else:
            items_, st_ = data[name], states[name]
            # trained on the train slice only (same model for all test sets, fair)
        bits, nf, ns = cost_list(M, cfg, items_, st_, k, af, as_)
        m, p95, mx = stats(bits)
        print("  %-10s n=%4d  ideal bytes mean %7.1f p95 %5d max %5d | symbols/snap: flags %.0f values %.0f" % (name, len(bits), m, p95, mx, sum(nf) / len(nf), sum(ns) / len(ns)))
        out[name] = bits
    return out

if __name__ == "__main__":
    which = sys.argv[1:] or ["gap", "flag"]
    for v in which:
        run_cfg("new schema, %s coding of changed fields" % v, variant=v)
