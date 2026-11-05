"""Complete snapshot codec for the new schema: encoder + decoder, static rANS (32-bit state, 15-bit probabilities).
Round trip test: decode(encode(snapshot)) must give back the exact same header, playerstate and entity list.
The decoder works only from the bytes, the base snapshot, the baselines and the header of the base snapshot."""
import math, struct, bisect, collections, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, bench3 as B3

PB = 15; PM = (1 << PB) - 1; RL = 1 << 23
EF = B.EF; NE = B.NE; ET_IDX = B.ET_IDX
ENAMES = [n for n, b in EF]
# playerstate fields with signed array extras (reader returns signed shorts / longs for the arrays)
PF2 = list(B.T["ps"])
for pre, bits in (("stats", -16), ("persistant", -16), ("ammo", -16), ("powerups", -32)):
    for i in range(16):
        PF2.append(("%s[%d]" % (pre, i), 19 if (pre == "stats" and i == 4) else bits))
NP = len(PF2)
PNAMES = [n for n, b in PF2]

f32 = B.f32; s32 = B.s32; zz = B.zz; fkind = B.fkind; cls_of = B.cls_of
A16 = 360.0 / 65536
def rnd(x): return int(math.floor(x + 0.5))
def pack(x): return struct.unpack("<I", struct.pack("<f", x))[0]
def unzz(x): return (x >> 1) if not (x & 1) else -((x + 1) >> 1)
def is_a16(b):
    f = f32(b)
    if f != f or abs(f) > 720: return False
    return pack(rnd(f / A16) * A16) == b
def old_index(ov):
    f = f32(ov)
    return rnd(f / A16) if (ov != 0 and f == f and abs(f) <= 720) else 0

# ------------------------------------------------------------------ io objects
class EncIO:
    def __init__(self): self.u = []
    def s(self, ctx, K, sym):
        assert 0 <= sym < K, (ctx, K, sym)
        self.u.append((ctx, K, sym)); return sym
    def r(self, n, val):
        assert 0 <= val < (1 << n)
        self.u.append((None, n, val)); return val

def chunks(n):
    out = []
    while n > 0:
        b = min(15, n); out.append(b); n -= b
    return out

class DecIO:
    def __init__(self, buf, tables):
        self.buf = buf; self.pos = 4; self.T = tables
        self.x = int.from_bytes(buf[:4], "big")
    def _get(self, f, cum):
        slot = self.x & PM
        s = bisect.bisect_right(cum, slot) - 1
        self.x = f[s] * (self.x >> PB) + slot - cum[s]
        while self.x < RL:
            self.x = (self.x << 8) | self.buf[self.pos]; self.pos += 1
        return s
    def s(self, ctx, K):
        f, cum = self.T.table(ctx, K)
        return self._get(f, cum)
    def r(self, n):
        val = 0; sh = 0
        for b in chunks(n):
            slot = self.x & PM
            v = slot >> (PB - b)
            self.x = (1 << (PB - b)) * (self.x >> PB) + slot - (v << (PB - b))
            while self.x < RL:
                self.x = (self.x << 8) | self.buf[self.pos]; self.pos += 1
            val |= v << sh; sh += b
        return val

class Tables:
    def __init__(self, counts, alpha):
        self.M = B.Model(); self.M.c = counts
        self.T = B3.Tables(self.M, alpha); self.cache = {}
    def table(self, ctx, K):
        t = self.cache.get((ctx, K))
        if t is None:
            f = self.T.freqs(ctx, K, PB); cum = [0]
            for q in f: cum.append(cum[-1] + q)
            t = self.cache[(ctx, K)] = (f, cum)
        return t

def rans_encode(units, tables):
    pairs = []
    for ctx, K, sym in units:
        if ctx is None:
            n, val = K, sym
            sh = 0
            for b in chunks(n):
                v = (val >> sh) & ((1 << b) - 1); sh += b
                pairs.append((v << (PB - b), 1 << (PB - b)))
        else:
            f, cum = tables.table(ctx, K)
            pairs.append((cum[sym], f[sym]))
    x = RL; out = bytearray()
    for start, freq in reversed(pairs):
        xmax = ((RL >> PB) << 8) * freq
        while x >= xmax:
            out.append(x & 255); x >>= 8
        x = ((x // freq) << PB) + (x % freq) + start
    out += x.to_bytes(4, "little")
    return bytes(reversed(out))

# ------------------------------------------------------------------ value coding (symmetric put / get)
def cls_put(io, ctx, K, x):
    c = x.bit_length(); io.s(ctx, K, c)
    if c > 1: io.r(c - 1, x - (1 << (c - 1)))

def cls_get(io, ctx, K):
    c = io.s(ctx, K)
    if c == 0: return 0
    if c == 1: return 1
    return (1 << (c - 1)) | io.r(c - 1)

def put_int(io, name, bits, nv, ov, mode):
    ab = abs(bits); M = 1 << ab; signed = bits < 0
    if mode == "a":
        if ab <= 8: io.s((name, "a"), M, nv & (M - 1))
        else: cls_put(io, (name, "a"), ab + 2, zz(nv) if signed else nv)
    else:
        d = ((nv - ov + M // 2) % M) - M // 2
        cls_put(io, (name, "d"), ab + 2, zz(d))

def get_int(io, name, bits, ov, mode):
    ab = abs(bits); M = 1 << ab; signed = bits < 0
    if mode == "a":
        if ab <= 8:
            v = io.s((name, "a"), M)
            return v - M if (signed and v >= M // 2) else v
        x = cls_get(io, (name, "a"), ab + 2)
        return unzz(x) if signed else x
    d = unzz(cls_get(io, (name, "d"), ab + 2))
    v = ov + d
    return ((v + M // 2) % M) - M // 2 if signed else v % M

def put_float(io, name, nv, ov, mode):
    k = fkind(nv)
    if k == 2 and is_a16(nv): k = 3
    io.s((name, "k"), 4, k)
    ok = fkind(ov)
    if k == 1:
        t = int(f32(nv)); ot = int(f32(ov)) if ok == 1 else 0
        if mode == "a": cls_put(io, (name, "ia"), 16, zz(t))
        else: cls_put(io, (name, "id"), 16, zz(t - ot))
    elif k == 3:
        cls_put(io, (name, "ad"), 24, zz(rnd(f32(nv) / A16) - old_index(ov)))
    elif k == 2:
        if mode == "a": io.r(32, nv)
        else: cls_put(io, (name, "fd"), 36, zz(s32(nv) - (s32(ov) if ok else 0)))

def get_float(io, name, ov, mode):
    k = io.s((name, "k"), 4)
    ok = fkind(ov)
    if k == 0: return 0
    if k == 1:
        ot = int(f32(ov)) if ok == 1 else 0
        if mode == "a": t = unzz(cls_get(io, (name, "ia"), 16))
        else: t = ot + unzz(cls_get(io, (name, "id"), 16))
        return pack(float(t))
    if k == 3:
        return pack((old_index(ov) + unzz(cls_get(io, (name, "ad"), 24))) * A16)
    if mode == "a": return io.r(32)
    return ((s32(ov) if ok else 0) + unzz(cls_get(io, (name, "fd"), 36))) & 0xFFFFFFFF

def put_field(io, name, bits, nv, ov, modes, pre):
    mode = modes.get(name, "d"); cn = pre + name       # playerstate and entity fields keep separate contexts
    if bits == 0: put_float(io, cn, nv, ov, mode)
    else: put_int(io, cn, bits, nv, ov, mode)

def get_field(io, name, bits, ov, modes, pre):
    mode = modes.get(name, "d"); cn = pre + name
    return get_float(io, cn, ov, mode) if bits == 0 else get_int(io, cn, bits, ov, mode)

# ------------------------------------------------------------------ header
def header_put(io, h, hb):
    for nm, v, bv in (("ack", h["ack"], hb["ack"] if hb else 0), ("time", h["time"], hb["time"] if hb else 0)):
        cls_put(io, ("h", nm), 36, zz(v - bv))
    io.s(("h", "delta"), 256, h["delta"]); io.s(("h", "flags"), 256, h["flags"])
    io.s(("h", "area"), 256, len(h["areas"]))
    for a in h["areas"]: io.s(("h", "area"), 256, a)

def header_get(io, hb):
    h = {}
    for nm in ("ack", "time"):
        h[nm] = (hb[nm] if hb else 0) + unzz(cls_get(io, ("h", nm), 36))
    h["delta"] = io.s(("h", "delta"), 256); h["flags"] = io.s(("h", "flags"), 256)
    n = io.s(("h", "area"), 256)
    h["areas"] = [io.s(("h", "area"), 256) for _ in range(n)]
    return h

# ------------------------------------------------------------------ playerstate
def ps_put(io, new, old, modes):
    prev = -1
    for i in range(NP):
        nm, bits = PF2[i]
        a = new.get(nm, 0); b = old.get(nm, 0)
        if a == b: continue
        io.s(("pg", prev), NP + 1, i); prev = i
        put_field(io, nm, bits, a, b, modes, "p:")
    io.s(("pg", prev), NP + 1, NP)

def ps_get(io, old, modes):
    s = dict(old); prev = -1
    while True:
        i = io.s(("pg", prev), NP + 1)
        if i == NP: return s
        assert i > prev, "ps field order"
        nm, bits = PF2[i]
        s[nm] = get_field(io, nm, bits, old.get(nm, 0), modes, "p:"); prev = i

# ------------------------------------------------------------------ entities
def ent_changes(new, old):
    return [(i, ENAMES[i], EF[i][1], new.get(ENAMES[i], 0), old.get(ENAMES[i], 0))
            for i in range(NE) if new.get(ENAMES[i], 0) != old.get(ENAMES[i], 0)]

def ents_put(io, new_list, old_list, baselines, modes):
    new = dict(new_list); old = dict(old_list)
    prevnum = -1; prevop = 0
    for num in sorted(set(new) | set(old)):
        if num in new and num in old:
            ch = ent_changes(new[num], old[num])
            if not ch: continue
            op = 0; basest = old[num]
        elif num in new:
            basest = baselines.get(num, {}); ch = ent_changes(new[num], basest); op = 1 if ch else 3
        else:
            op = 2; ch = []; basest = old[num]
        gap = num - prevnum - 1; prevnum = num
        if gap < 40: io.s(("eg",), 42, gap)
        else: io.s(("eg",), 42, 40); io.r(10, gap)
        io.s(("eo", prevop), 4, op); prevop = op
        if op in (2, 3): continue
        st = new[num]
        bcls = cls_of(basest.get("eType", 0)); ncls = cls_of(st.get("eType", 0))
        chd = {c[0]: c for c in ch}
        c = chd.get(ET_IDX)
        io.s(("ef", "et", bcls), 2, 1 if c else 0)
        if c: io.s(("ev", "et", bcls), 256, c[3])
        prev = -1
        for x in ch:
            if x[0] == ET_IDX: continue
            io.s(("eg2", ncls, prev), NE + 1, x[0]); prev = x[0]
            put_field(io, x[1], x[2], x[3], x[4], modes, "e:")
        io.s(("eg2", ncls, prev), NE + 1, NE)
    io.s(("eg",), 42, 41)

def ents_get(io, old_list, baselines, modes):
    old = dict(old_list); cur = dict(old)
    prevnum = -1; prevop = 0
    while True:
        g = io.s(("eg",), 42)
        if g == 41: break
        gap = io.r(10) if g == 40 else g
        num = prevnum + 1 + gap; prevnum = num
        op = io.s(("eo", prevop), 4); prevop = op
        if op == 2:
            del cur[num]; continue
        basest = old[num] if op == 0 else baselines.get(num, {})
        st = dict(basest); st["number"] = num
        if op == 3:
            cur[num] = st; continue
        bcls = cls_of(basest.get("eType", 0))
        if io.s(("ef", "et", bcls), 2):
            st["eType"] = io.s(("ev", "et", bcls), 256)
        ncls = cls_of(st.get("eType", 0))
        prev = -1
        while True:
            i = io.s(("eg2", ncls, prev), NE + 1)
            if i == NE: break
            assert i > prev and i != ET_IDX
            nm, bits = EF[i]
            st[nm] = get_field(io, nm, bits, basest.get(nm, 0), modes, "e:"); prev = i
        cur[num] = st
    return sorted(cur.items())

# ------------------------------------------------------------------ snapshot
def snap_put(io, item, base, hb, baselines, modes):
    h = header(item)
    header_put(io, h, hb)
    sn = item["snap"]
    assert not sn["ps"].get("m_iVehicleNum", 0)
    ps_put(io, sn["ps"], base["ps"] if base else {}, modes)
    ents_put(io, sn["ents"], base["ents"] if base else [], baselines, modes)

def snap_get(io, base, hb, baselines, modes):
    h = header_get(io, hb)
    ps = ps_get(io, base["ps"] if base else {}, modes)
    ents = ents_get(io, base["ents"] if base else [], baselines, modes)
    return h, ps, ents

def header(item):
    L = collections.defaultdict(list)
    for t, b, v in item["log"]:
        if t[0] == "h": L[t[1]].append(v)
    areas = L["area"]
    return dict(ack=s32(L["ack"][0]), time=s32(L["time"][0]), delta=L["delta"][0], flags=L["flags"][0], areas=areas[1:])

def canon_ps(ps): return tuple(ps.get(n, 0) for n in PNAMES)
def canon_ents(ents): return [(n, tuple(s.get(k, 0) for k in ENAMES)) for n, s in ents]

# ------------------------------------------------------------------ experiment
def run(alpha=0.01, k=None):
    B.ANG16 = True
    items = B.data["5min"]; st5 = B.states["5min"]
    cut = int(len(items) * 0.6)
    train, test = items[:cut], items[cut:]
    modes = B.choose_modes(train, st5, k or 0, None)

    def ctx_for(its, st):
        hdr = {it["seq"]: header(it) for it in its}
        out = []
        for it in its:
            d = k or it["delta"]
            base = st.get(it["seq"] - d) if d else None
            if d and (base is None or (it["seq"] - d) not in hdr): continue
            out.append((it, base, hdr.get(it["seq"] - d) if d else None))
        return out

    counts = {}
    for it, base, hb in ctx_for(train, st5):
        io = EncIO(); snap_put(io, it, base, hb, it["baselines"], modes)
        for ctx, K, sym in io.u:
            if ctx is None: continue
            e = counts.get(ctx)
            if e is None: e = counts[ctx] = [collections.Counter(), 0]
            e[0][sym] += 1; e[1] += 1
    tables = Tables(counts, alpha)
    print("trained on %d snapshots, %d contexts" % (len(ctx_for(train, st5)), len(counts)))

    sets = {"5min-test": (test, st5), "bots": (B.data["bots"], B.states["bots"]), "solo": (B.data["solo"], B.states["solo"])}
    for name, (its, st) in sets.items():
        cs = ctx_for(its, st)
        sizes = []; bad = 0; n = 0
        for it, base, hb in cs:
            io = EncIO(); snap_put(io, it, base, hb, it["baselines"], modes)
            buf = rans_encode(io.u, tables)
            sizes.append(len(buf))
            h, ps, ents = snap_get(DecIO(buf, tables), base, hb, it["baselines"], modes)
            ok = (h == header(it) and canon_ps(ps) == canon_ps(it["snap"]["ps"]) and canon_ents(ents) == canon_ents(it["snap"]["ents"]))
            if not ok:
                bad += 1
                if bad <= 3: print("   MISMATCH", name, it["seq"], h == header(it), canon_ps(ps) == canon_ps(it["snap"]["ps"]), canon_ents(ents) == canon_ents(it["snap"]["ents"]))
            n += 1
        sizes.sort()
        print("%-10s snapshots %5d (skipped %d without header of base)  round-trip failures %d | real bytes mean %.1f p95 %d max %d" %
              (name, n, len(its) - n, bad, sum(sizes) / n, sizes[int(n * .95)], sizes[-1]))

if __name__ == "__main__":
    for kk in ([int(a) for a in sys.argv[1:]] or [None]):
        print("== base age", kk or "actual (1)")
        run(k=kk)
