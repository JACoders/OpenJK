"""Model of the engine's own snapshot writer (MSG_WriteDeltaEntity / MSG_WriteDeltaPlayerstate / SV_WriteSnapshotToClient)
producing the list of MSG_WriteBits calls, so synthetic states can be priced with the engine Huffman tree.
Checked against real captures: it must reproduce the measured bit counts of real snapshots."""
import struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, codec as C

EF = B.EF; NE = B.NE
PS = list(B.T["ps"])
ARRS = B.bdata.J.FIELDS["arrays"]   # stats, persistant, ammo, powerups
FIB = 13; BIAS = 1 << (FIB - 1)

def f32(b): return struct.unpack("<f", struct.pack("<I", b & 0xFFFFFFFF))[0]

def write_float(calls, v, entity):
    f = f32(v)
    if entity:
        if f == 0.0:
            calls.append((1, 0)); return
        calls.append((1, 1))
    if f == int(f) and -BIAS <= int(f) < BIAS if f == f and abs(f) < 1e9 else False:
        calls.append((1, 0)); calls.append((FIB, int(f) + BIAS))
    else:
        calls.append((1, 1)); calls.append((32, v & 0xFFFFFFFF))

def write_fields(calls, fields, new, old, entity):
    lc = 0
    for i, (nm, bits) in enumerate(fields):
        if new.get(nm, 0) != old.get(nm, 0): lc = i + 1
    calls.append((8, lc))
    for i in range(lc):
        nm, bits = fields[i]; a = new.get(nm, 0); b = old.get(nm, 0)
        if a == b: calls.append((1, 0)); continue
        calls.append((1, 1))
        if bits == 0: write_float(calls, a, entity)
        elif entity:
            if a == 0: calls.append((1, 0))
            else:
                calls.append((1, 1)); calls.append((bits, a & ((1 << abs(bits)) - 1) if bits < 0 else a))
        else:
            calls.append((bits, a & ((1 << abs(bits)) - 1) if bits < 0 else a))

def write_arrays(calls, new, old):
    masks = {}
    for a in ARRS:
        m = 0
        for i in range(a["count"]):
            k = "%s[%d]" % (a["name"], i)
            if new.get(k, 0) != old.get(k, 0): m |= 1 << i
        masks[a["name"]] = m
    if not any(masks.values()): calls.append((1, 0)); return
    calls.append((1, 1))
    for a in ARRS:
        pre = a["name"]; sp = a.get("special"); w = abs(a["bits"])
        if not masks[pre]: calls.append((1, 0)); continue
        calls.append((1, 1)); calls.append((a["count"], masks[pre]))
        for i in range(a["count"]):
            if masks[pre] >> i & 1:
                v = new.get("%s[%d]" % (pre, i), 0)
                if sp and i == sp["index"]: calls.append((sp["bits"], v))
                else: calls.append((w, v & ((1 << w) - 1)))

def snapshot_calls(msg_ack, op, base, baselines):
    """calls for ack + one snapshot op (no server commands, no vehicle)"""
    calls = [(32, msg_ack & 0xFFFFFFFF), (8, 7), (32, op["time"] & 0xFFFFFFFF), (8, op["delta"]), (8, op["flags"]),
             (8, len(op["areas"]))] + [(8, a) for a in op["areas"]]
    sn = op["snap"]; bps = base["ps"] if base else {}
    calls.append((1, 0))                                        # normal (not pilot) playerstate
    write_fields(calls, PS, sn["ps"], bps, False)
    write_arrays(calls, sn["ps"], bps)
    new = dict(sn["ents"]); old = dict(base["ents"]) if base else {}
    for num in sorted(set(new) | set(old)):
        if num in new and num in old:
            if not C.ent_changes(new[num], old[num]): continue
            calls.append((10, num)); calls.append((1, 0)); calls.append((1, 1))
            write_fields(calls, EF, new[num], old[num], True)
        elif num in new:
            bs = baselines.get(num, {})
            calls.append((10, num)); calls.append((1, 0))
            if C.ent_changes(new[num], bs):
                calls.append((1, 1)); write_fields(calls, EF, new[num], bs, True)
            else: calls.append((1, 0))
        else:
            calls.append((10, num)); calls.append((1, 1))
    calls.append((10, 1023))
    return calls

def bits_of(calls, depth):
    t = 0
    for b, v in calls:
        ab = abs(b); nb = ab & 7
        t += nb + sum(depth[(v >> (nb + 8 * i)) & 255] for i in range((ab - nb) // 8))
    return t

if __name__ == "__main__":
    import bdata
    obj = bdata.load_all(); depth = bdata.depth_table(obj["dec"])
    import fullload
    raw = fullload.load_all()
    bad = 0; n = 0; tot = 0
    st = {}
    for m in raw["5min"]:
        for op in m["msg"]["ops"]:
            if op["t"] == "snap":
                base = st.get(m["seq"] - op["delta"]) if op["delta"] else None
                if op["delta"] and base is None: continue
                bl = {}
                calls = snapshot_calls(m["msg"]["ack"], op, base, RAW_BL) if False else None
    # price check against measured bits (snapshot-only messages)
    items = bdata.load_all()["data"]["5min"]; sts = bdata.load_all()["states"]["5min"]
    mism = 0; cnt = 0
    for it in items[:1500]:
        base = sts.get(it["seq"] - 1)
        h = C.header(it)
        op = dict(time=h["time"], delta=1, flags=h["flags"], areas=h["areas"], snap=it["snap"])
        calls = snapshot_calls(h["ack"], op, base, it["baselines"])
        cnt += 1
        if bits_of(calls, depth) != it["bits"]: mism += 1
    print("engine writer model vs measured bits: %d snapshots, %d mismatches" % (cnt, mism))
