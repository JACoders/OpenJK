"""Round trip of every real server message (commands, gamestate, mapchange, snapshots) with the whole-message codec."""
import sys, os, collections, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, codec as C, codec2 as K2, fullload

B.ANG16 = True
RAW = fullload.load_all()

def snap_of(msg):
    for op in msg["ops"]:
        if op["t"] == "snap": return op["snap"]
    return None

def encode_stream(msgs, modes, tables=None, counts=None, only=None):
    """server side: yields (seq, units, bytes) ; keeps its own store of sent snapshots"""
    store = {}; baselines = {}; out = []
    for m in msgs:
        seq = m["seq"]; msg = m["msg"]; delta = m["delta"]
        base = bh = None
        if delta:
            if seq - delta not in store: continue
            base, bh = store[seq - delta]
        io = EncIO()
        hdr = K2.msg_put(io, msg, base, bh, baselines, modes, delta)
        for op in msg["ops"]:
            if op["t"] == "gs": baselines = dict(op["baselines"])
        sn = snap_of(msg)
        if sn is not None: store[seq] = (sn, hdr)
        if counts is not None:
            for ctx, Kk, sym in io.u:
                if ctx is None: continue
                e = counts.get(ctx)
                if e is None: e = counts[ctx] = [collections.Counter(), 0]
                e[0][sym] += 1; e[1] += 1
        if only is None or only(seq, msg):
            out.append((m, io.u, None))
    return out

EncIO = C.EncIO

def decode_check(msgs, modes, tables, label):
    store_srv = {}; store_cli = {}; baselines_s = {}; baselines_c = {}
    bad = 0; n = 0; sizes = collections.defaultdict(list); first_bad = []
    for m in msgs:
        seq = m["seq"]; msg = m["msg"]; delta = m["delta"]
        base = bh = None
        if delta:
            if seq - delta not in store_srv: continue
            base, bh = store_srv[seq - delta]
        io = EncIO()
        hdr = K2.msg_put(io, msg, base, bh, baselines_s, modes, delta)
        buf = C.rans_encode(io.u, tables)
        def lookup(d):
            if d == 0: return None, None
            return store_cli[seq - d]
        dmsg, dhdr, dsn, baselines_c = K2.msg_get(C.DecIO(buf, tables), lookup, baselines_c, modes)
        ok = K2.canon_msg(dmsg) == K2.canon_msg(msg) and dhdr == hdr
        n += 1
        kinds = tuple(sorted({op["t"] for op in msg["ops"]}))
        sizes[kinds].append(len(buf))
        if not ok:
            bad += 1
            if len(first_bad) < 3: first_bad.append((seq, kinds))
        for op in msg["ops"]:
            if op["t"] == "gs": baselines_s = dict(op["baselines"])
        sn = snap_of(msg)
        if sn is not None:
            store_srv[seq] = (sn, hdr)
            store_cli[seq] = (dsn, dhdr)
    print("%-10s messages %5d  failures %d  %s" % (label, n, bad, first_bad))
    for k, v in sorted(sizes.items(), key=lambda x: -len(x[1])):
        v = sorted(v)
        print("      %-28s n=%5d  real bytes mean %7.1f p95 %5d max %5d" % ("+".join(k), len(v), sum(v) / len(v), v[int(len(v) * .95)], v[-1]))
    return bad

def main():
    msgs = RAW["5min"]; cut = int(len(msgs) * 0.6)
    train, test = msgs[:cut], msgs[cut:]
    # snapshots ps/ents training data for the modes: reuse bench2 data for the 5min train part
    items = B.data["5min"]; st5 = B.states["5min"]
    modes = B.choose_modes(items[:int(len(items) * 0.6)], st5, 0, None)
    counts = {}
    encode_stream(train, modes, counts=counts, only=lambda s, m: False)
    tables = C.Tables(counts, 0.01)
    print("tables trained on %d messages, %d contexts" % (cut, len(counts)))
    # the test part needs the store of the train part (bases): replay whole stream, check only the test messages
    class Part(list): pass
    bad = decode_check(msgs, modes, tables, "5min (all; first 60%% = train)")
    bad += decode_check(RAW["bots"], modes, tables, "bots")
    bad += decode_check(RAW["solo"], modes, tables, "solo")
    print("total failures", bad)

if __name__ == "__main__":
    main()
