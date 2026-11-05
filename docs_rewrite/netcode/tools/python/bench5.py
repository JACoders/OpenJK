"""Prediction experiments on top of the bench2 schema (ideal bytes, angles as 16-bit index).

Variants (all lossless; the decoder has every input of each prediction before the value it predicts):
  T   time fields (pos.trTime, apos.trTime, ps.commandTime) coded against the snapshot serverTime
  P1  positions predicted from the old position and the old velocity:
        entity pos.trBase[i] (TR_LINEAR_STOP in old and new): old.trBase + old.trDelta * (new.trTime - old.trTime) / 1000
        player state origin[i]:                                old.origin + old.velocity * (new.commandTime - old.commandTime) / 1000
      the residual is the signed difference of the 32-bit patterns of the value and of the prediction rounded to float32
  P2  as P1 with the mean of the old and the new velocity (velocity fields are moved before the position fields,
      so the decoder knows the new velocity first)
  A2  angles sent as 16-bit indices predicted to second order from the base and the base's base:
        index(new) - (2 * index(old) - index(older)); falls back to the bench2 coding when 'older' is missing
Run: python bench5.py [variant ...]   (default: base T P1 P2 A2 T+P2+A2)
"""
import sys, os, struct, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B

B.ANG16 = True
ON = set()
ACTIVE = False
CTX = {}            # phase, serverTime, new/old/older states of the current player state or entity

def f2b(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]

def pred_pos(name, new, old, tf, vf, of):
    """name like 'pos.trBase[1]' -> predicted float32 bits or None"""
    i = name[-2]
    tn, to = new.get(tf, 0), old.get(tf, 0)
    if tn == to and tf != "commandTime": return None
    vo = B.f32(old.get(vf % i, 0))
    if "P2" in ON: vo = 0.5 * (vo + B.f32(new.get(vf % i, 0)))
    p = B.f32(old.get(of % i, 0)) + vo * (tn - to) / 1000.0
    if p != p or abs(p) > 3.0e38: return None
    return f2b(p)

_orig_tokens = B.tokens
def tokens(name, bits, nv, ov, modes, quant):
    if not ACTIVE: return _orig_tokens(name, bits, nv, ov, modes, quant)
    ph = CTX.get("phase")
    if "T" in ON and bits == 32 and ((ph == "ent" and name in ("pos.trTime", "apos.trTime")) or (ph == "ps" and name == "commandTime")):
        d = B.s32(nv & 0xFFFFFFFF) - CTX["time"]
        c = B.zz(d).bit_length()
        return [((name, "t"), 36, c, c - 1 if c else 0)]
    if bits == 0 and ("P1" in ON or "P2" in ON):
        p = None
        if ph == "ent" and name.startswith("pos.trBase["):
            n, o = CTX["new"], CTX["old"]
            if n.get("pos.trType") == 3 and o.get("pos.trType") == 3:
                p = pred_pos(name, n, o, "pos.trTime", "pos.trDelta[%s]", "pos.trBase[%s]")
        elif ph == "ps" and name.startswith("origin["):
            p = pred_pos(name, CTX["new"], CTX["old"], "commandTime", "velocity[%s]", "origin[%s]")
        if p is not None and B.fkind(nv) == 2 and not B.is_a16(nv):
            d = B.s32(nv & 0xFFFFFFFF) - B.s32(p)
            c = B.zz(d).bit_length()
            return [((name, "k"), 4, 2, 0), ((name, "fp"), 40, c, c - 1 if c else 0)]
    if bits == 0 and "A2" in ON and ph in ("ent", "ps") and B.is_a16(nv) and B.fkind(nv) == 2:
        older = CTX.get("older")
        if older is not None and name in older and B.is_a16(ov) and B.is_a16(older[name]):
            k = B.old_index(nv); k1 = B.old_index(ov); k2 = B.old_index(older[name])
            d = k - (2 * k1 - k2)
            c = B.zz(d).bit_length()
            return [((name, "k"), 4, 3, 0), ((name, "a2"), 40, c, c - 1 if c else 0)]
    return _orig_tokens(name, bits, nv, ov, modes, quant)
B.tokens = tokens

_orig_ec = B.ent_changes
def ent_changes(new, old):
    if ACTIVE:
        CTX["phase"] = "ent"; CTX["new"] = new; CTX["old"] = old
        CTX["older"] = CTX["older_ents"].get(new.get("number")) if CTX.get("older_ents") is not None else None
    return _orig_ec(new, old)
B.ent_changes = ent_changes

_orig_se = B.snap_events
def snap_events(i, items, st, k, cfg, prevhdr):
    s = items[i]
    age = k or s["delta"]
    base = st.get(s["seq"] - age) if age else None
    older = st.get(s["seq"] - 2 * age) if age else None
    L = {t: v for t, b, v in s["log"] if t[0] == "h"}
    CTX.clear()
    CTX.update(phase="ps", time=L[("h", "time")], new=s["snap"]["ps"], old=base["ps"] if base else {},
               older=older["ps"] if older else None,
               older_ents=dict(older["ents"]) if older else None)
    return _orig_se(i, items, st, k, cfg, prevhdr)
B.snap_events = snap_events

_orig_cm = B.choose_modes
def choose_modes(*a, **kw):
    global ACTIVE
    was = ACTIVE; ACTIVE = False
    try: return _orig_cm(*a, **kw)
    finally: ACTIVE = was
B.choose_modes = choose_modes

def reorder_velocity_first():
    def move(lst, first, before):
        items = [x for x in lst if x[0] in first]
        rest = [x for x in lst if x[0] not in first]
        j = min(i for i, x in enumerate(rest) if x[0] in before)
        lst[:] = rest[:j] + items + rest[j:]
    move(B.EF, {"pos.trDelta[0]", "pos.trDelta[1]", "pos.trDelta[2]"}, {"pos.trBase[0]", "pos.trBase[1]", "pos.trBase[2]"})
    move(B.PF, {"velocity[0]", "velocity[1]", "velocity[2]"}, {"origin[0]", "origin[1]", "origin[2]"})
    B.ET_IDX = [i for i, (n, b) in enumerate(B.EF) if n == "eType"][0]

if __name__ == "__main__":
    runs = sys.argv[1:] or ["base", "T", "P1", "P2", "A2", "T+P2+A2"]
    EF0, PF0 = list(B.EF), list(B.PF)
    for r in runs:
        B.EF[:] = EF0; B.PF[:] = PF0; B.ET_IDX = [i for i, (n, b) in enumerate(B.EF) if n == "eType"][0]
        ON.clear(); ON.update(x for x in r.split("+") if x != "base")
        if "P2" in ON: reorder_velocity_first()
        ACTIVE = True
        B.run_cfg("bench5 variant %s" % r)
