"""Real encode -> decode round trip of the float coding used by the new schema (zero / integral / 16-bit angle index / full).
The decoder only sees (kind, payload) and the OLD value, never the new value."""
import math, random, struct, sys, os, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B

A16 = 360.0 / 65536
f32 = B.f32
def pack(x): return struct.unpack("<I", struct.pack("<f", x))[0]
def s32(b): return b - (1 << 32) if b >= (1 << 31) else b
def rnd(x): return int(math.floor(x + 0.5))        # defined identically on both sides (C: (int)floor(x + 0.5))

def fkind(b):
    if b == 0: return 0
    f = f32(b)
    if f == f and -4096 <= f < 4096 and f == int(f): return 1
    return 2

def is_a16(b):
    f = f32(b)
    if f != f or abs(f) > 720: return False
    return pack(rnd(f / A16) * A16) == b

def old_index(ov):
    f = f32(ov)
    return rnd(f / A16) if (ov != 0 and f == f and abs(f) <= 720) else 0

def enc(nv, ov):
    k = fkind(nv)
    if k == 2 and is_a16(nv): k = 3
    if k == 0: return (0, 0)
    ok = fkind(ov)
    if k == 1: return (1, int(f32(nv)) - (int(f32(ov)) if ok == 1 else 0))
    if k == 3: return (3, rnd(f32(nv) / A16) - old_index(ov))
    return (2, s32(nv) - (s32(ov) if ok else 0))

def dec(kind, d, ov):
    ok = fkind(ov)
    if kind == 0: return 0
    if kind == 1: return pack(float((int(f32(ov)) if ok == 1 else 0) + d))
    if kind == 3: return pack((old_index(ov) + d) * A16)
    return ((s32(ov) if ok else 0) + d) & 0xFFFFFFFF

def check(nv, ov, stats, bad, tag):
    k, d = enc(nv, ov)
    r = dec(k, d, ov)
    stats[k] += 1
    if r != nv:
        bad.append((tag, hex(nv), hex(ov), k, d, hex(r)))

def real_data():
    stats = collections.Counter(); bad = []
    for name in ("5min", "bots", "solo"):
        st = B.states[name]; items = B.data[name]
        base_bl = items[0]["baselines"]
        for seq in sorted(st):
            snap = st[seq]; base = st.get(seq - 1)
            if base is None: continue
            # playerstate floats
            for i in range(B.NP):
                nm, bits = B.PF[i]
                if bits != 0: continue
                a = snap["ps"].get(nm, 0); b = base["ps"].get(nm, 0)
                if a != b: check(a, b, stats, bad, (name, seq, "ps", nm))
            new = dict(snap["ents"]); old = dict(base["ents"])
            for num, s in new.items():
                src = old.get(num) or base_bl.get(num, {})
                for i, nm_, bits, a, b in B.ent_changes(s, src):
                    if bits == 0: check(a, b, stats, bad, (name, seq, "ent", nm_))
    return stats, bad

def fuzz(n=400000):
    rng = random.Random(1)
    stats = collections.Counter(); bad = []
    def rand_bits():
        r = rng.random()
        if r < 0.25: return rng.getrandbits(32)
        if r < 0.45: return pack(rnd(rng.uniform(-200, 400) / A16) * A16)                 # on the angle grid
        if r < 0.55: return pack(rng.randint(-5000, 5000) * 1.0)                          # integral, some out of range
        if r < 0.65: return pack(rng.randint(-32768, 32767) * A16)                        # signed short angles
        if r < 0.70: return pack(65536 * A16)                                             # 360.0 exactly
        if r < 0.75: return pack(-65536 * A16)
        if r < 0.80: return rng.choice([0x7F800000, 0xFF800000, 0x7FC00000, 0x00000001, 0x80000001, 0x7F7FFFFF, 0xFF7FFFFF])
        if r < 0.90: return pack(rng.uniform(-8000, 8000))
        return pack(rnd(rng.uniform(-4, 4) / A16) * A16)
    for _ in range(n):
        nv, ov = rand_bits(), rand_bits()
        if nv == 0x80000000: continue            # -0.0: the engine also turns it into +0.0
        check(nv, ov, stats, bad, "fuzz")
    return stats, bad

if __name__ == "__main__":
    s, b = real_data()
    print("real data: kinds", dict(s), "mismatches", len(b))
    for x in b[:8]: print("  ", x)
    s, b = fuzz()
    print("fuzz:      kinds", dict(s), "mismatches", len(b))
    for x in b[:8]: print("  ", x)
