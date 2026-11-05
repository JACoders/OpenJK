"""Step 1 of the plan (05): the prediction modes of bench5 in the real whole-message codec, plus the rANS
initial-state trick. Round trip of every real message (as run_full.py), per-field choice of the modes on the
training part, then fuzz and loss simulation with the chosen modes.
Run: python run_pred.py [pf|pi] [--fuzz] [--loss]"""
import sys, os, math, collections, random
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, codec as C, codec2 as K2, run_full as RF

ARITH = "pi" if "pi" in sys.argv[1:] else "pf"

def candidates():
    pm = {"e:pos.trTime": "t", "e:apos.trTime": "t", "p:commandTime": "t"}
    pos = {"e:pos.trBase[%d]" % i for i in range(3)} | {"p:origin[%d]" % i for i in range(3)}
    for cn in pos: pm[cn] = ARITH
    floats = {"e:" + n for n, b in C.EF if b == 0} | {"p:" + n for t in K2.PS_TABLES.values() for n, b in t if b == 0}
    for cn in floats - pos: pm[cn] = "a2"
    return pm

def train_units(msgs, modes):
    """encode the stream like run_full.encode_stream; return counts and the unit lists"""
    store = {}; baselines = {}; counts = {}; units = []
    for m in msgs:
        seq = m["seq"]; msg = m["msg"]; delta = m["delta"]
        base = bh = None
        if delta:
            if seq - delta not in store: continue
            base, bh = store[seq - delta]
        io = C.EncIO()
        hdr = K2.msg_put(io, msg, base, bh, baselines, modes, delta)
        for op in msg["ops"]:
            if op["t"] == "gs": baselines = dict(op["baselines"])
        sn = RF.snap_of(msg)
        if sn is not None: store[seq] = (sn, hdr)
        for ctx, Kk, sym in io.u:
            if ctx is None: continue
            e = counts.get(ctx)
            if e is None: e = counts[ctx] = [collections.Counter(), 0]
            e[0][sym] += 1; e[1] += 1
        units.append(io.u)
    return counts, units

def cost_by_field(counts, units):
    """ideal bits per prefixed field name (contexts whose first element is that name; raw bits go to the symbol before)"""
    out = collections.Counter(); last = None
    for u in units:
        for ctx, Kk, sym in u:
            if ctx is None:
                if last is not None: out[last] += Kk
                continue
            last = ctx[0] if isinstance(ctx[0], str) and ctx[0][:2] in ("e:", "p:") else None
            if last is None: continue
            c, n = counts[ctx]
            out[last] += -math.log2(c[sym] / n)
    return out

def main():
    K2.enable_prediction()
    msgs = RF.RAW["5min"]; cut = int(len(msgs) * 0.6); train = msgs[:cut]
    items = B.data["5min"]; st5 = B.states["5min"]
    modes = B.choose_modes(items[:int(len(items) * 0.6)], st5, 0, None)

    # per-field choice on the training part
    C.PMODES = {}; c0, u0 = train_units(train, modes); off = cost_by_field(c0, u0)
    C.PMODES = candidates(); c1, u1 = train_units(train, modes); on = cost_by_field(c1, u1)
    chosen = {cn: m for cn, m in C.PMODES.items() if on.get(cn, 0) < off.get(cn, 0) - 8}
    print("arithmetic of the position prediction: %s" % ARITH)
    print("modes chosen on the training part (%d of %d candidates), training bits saved per field:" % (len(chosen), len(C.PMODES)))
    for cn in sorted(chosen, key=lambda x: on[x] - off[x]):
        print("   %-28s %-3s %9.0f -> %9.0f" % (cn, chosen[cn], off[cn], on[cn]))

    configs = [("study codec (field order for prediction)", {}, C.RL),
               ("+ rANS initial-state trick", {}, 1),
               ("+ prediction modes", chosen, 1)]
    for label, pm, x0 in configs:
        C.PMODES = pm; C.X0 = x0
        counts = {}
        RF.encode_stream(train, modes, counts=counts, only=lambda s, m: False)
        tables = C.Tables(counts, 0.01)
        print("\n### %s (%d contexts)" % (label, len(counts)))
        bad = RF.decode_check(msgs, modes, tables, "5min (all; first 60% = train)")
        bad += RF.decode_check(RF.RAW["bots"], modes, tables, "bots")
        bad += RF.decode_check(RF.RAW["solo"], modes, tables, "solo")
        print("total failures", bad)

    C.PMODES = chosen; C.X0 = 1
    if "--fuzz" in sys.argv:
        import fuzz
        for seed in range(1, 6):
            fm, streams = fuzz.run(seed)
            n, bad, kinds, sizes = fuzz.check(fm, streams, C.Tables({}, 0.01))
            print("fuzz seed %d, uniform tables: messages %d, failures %d" % (seed, n, bad))
    if "--loss" in sys.argv:
        import sim_loss
        sim_loss.main()

if __name__ == "__main__":
    main()
