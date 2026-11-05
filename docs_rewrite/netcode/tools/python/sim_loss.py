"""Packet loss / latency simulation on the real 5-minute capture, with the whole-message codec.
Server: delta base = newest snapshot it knows the client received (acks arrive r snapshots late), none if older than 28.
Client: decodes only what arrives, using only its own decoded copies. Late/duplicated packets are dropped by the netchan,
so reordering behaves as loss. Round trip is verified for every received message."""
import sys, os, random, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, codec as C, codec2 as K2, fullload

B.ANG16 = True
RAW = fullload.load_all()
MAXAGE = 28                    # engine: outgoingSequence - deltaMessage >= PACKET_BACKUP - 3  -> no delta

def snap_of(msg):
    for op in msg["ops"]:
        if op["t"] == "snap": return op["snap"]
    return None

def with_delta(msg, delta):
    ops = []
    for op in msg["ops"]:
        if op["t"] == "snap": op = dict(op, delta=delta)
        ops.append(op)
    return dict(msg, ops=ops)

def simulate(msgs, p, r, rng, modes, tables, counts=None, verify=True, warm=40):
    received = {}; store_s = {}; store_c = {}; bs = {}; bc = {}
    sizes = []; ages = []; bad = 0; nobase = 0; nsnap = 0
    for idx, m in enumerate(msgs):
        seq = m["seq"]; msg = m["msg"]; sn = snap_of(msg)
        delta = 0
        if sn is not None:
            for cand in range(seq - r, seq - MAXAGE - 1, -1):
                if received.get(cand) and cand in store_s:
                    delta = seq - cand; break
            msg = with_delta(msg, delta)
        base = bh = None
        if delta: base, bh = store_s[seq - delta]
        io = C.EncIO()
        hdr = K2.msg_put(io, msg, base, bh, bs, modes, delta)
        for op in msg["ops"]:
            if op["t"] == "gs": bs = dict(op["baselines"])
        if sn is not None: store_s[seq] = (sn, hdr)
        if counts is not None:
            for ctx, Kk, sym in io.u:
                if ctx is None: continue
                e = counts.get(ctx)
                if e is None: e = counts[ctx] = [collections.Counter(), 0]
                e[0][sym] += 1; e[1] += 1
        lost = idx > warm and rng.random() < p
        received[seq] = not lost
        if tables is not None:
            buf = C.rans_encode(io.u, tables)
            if sn is not None:
                sizes.append(len(buf)); ages.append(delta); nsnap += 1
                if delta == 0: nobase += 1
            if verify and not lost:
                lookup = lambda d: (None, None) if d == 0 else store_c[seq - d]
                dmsg, dhdr, dsn, bc = K2.msg_get(C.DecIO(buf, tables), lookup, bc, modes)
                if K2.canon_msg(dmsg) != K2.canon_msg(msg) or dhdr != hdr: bad += 1
                if dsn is not None: store_c[seq] = (dsn, dhdr)
            elif not lost and sn is not None:
                store_c[seq] = (sn, hdr)
            for op in msg["ops"]:
                if op["t"] == "gs" and lost: bc = bc      # a lost gamestate would be retransmitted by the engine; not simulated
    return sizes, ages, bad, nobase, nsnap

def main():
    msgs = RAW["5min"]; cut = int(len(msgs) * 0.6)
    train, test = msgs[:cut], msgs[cut:]
    items = B.data["5min"]; st5 = B.states["5min"]
    modes = B.choose_modes(items[:int(len(items) * 0.6)], st5, 2, None)
    counts = {}
    # training: mixed conditions so that every base age is represented
    for p, r, sd in ((0.0, 1, 1), (0.10, 2, 2), (0.05, 4, 3)):
        simulate(train, p, r, random.Random(sd), modes, None, counts=counts)
    tables = C.Tables(counts, 0.01)
    print("tables: %d contexts (trained on the first 60%% under p/RTT mixes)" % len(counts))
    print("%-6s %-9s %9s %5s %5s | %9s %9s | %s" % ("loss", "RTT", "mean B", "p95", "max", "no-base", "mean age", "round-trip failures"))
    allbad = 0
    # the test part needs the server store from the stream start: simulate the whole stream, report the test part only
    for p in (0.0, 0.02, 0.05, 0.10, 0.20):
        for r in (1, 2, 4, 8):
            rng = random.Random(100 + int(p * 1000) + r)
            sizes, ages, bad, nobase, nsnap = simulate(msgs, p, r, rng, modes, tables)
            k = len(sizes) - len([m for m in test if snap_of(m["msg"]) is not None])
            ts = sorted(sizes[k:]); ta = ages[k:]
            nb = sum(1 for a in ta if a == 0)
            allbad += bad
            print("%-6s %-9s %9.1f %5d %5d | %8.1f%% %9.2f | %d" % ("%d%%" % round(p * 100), "%d ms" % (r * 50),
                  sum(ts) / len(ts), ts[int(len(ts) * .95)], ts[-1], 100.0 * nb / len(ta), sum(a for a in ta if a) / max(1, len(ta) - nb), bad))
    print("total round-trip failures", allbad)

if __name__ == "__main__":
    main()
