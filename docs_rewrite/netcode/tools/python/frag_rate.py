"""Netchan fragmentation, rate limiting (SV_RateMsec) and gamestate retransmission, engine format vs new codec.
Facts taken from the source: FRAGMENT_SIZE = 1400 - 100 = 1300 (net_chan.cpp:34), a message of length >= 1300 is split,
fragment header = 4 (sequence | FRAGMENT_BIT) + 2 start + 2 length; the client drops the whole message if one fragment is missing
(Netchan_Process requires fragmentStart == fragmentLength); SV_RateMsec = (min(size, 1500) + 48) * 1000 / rate, with a message
'rate delayed' when rateMsec >= snapshotMsec; client default rate 4000, snaps 20 (cl_main.cpp), LAN clients are not limited (99999);
a client that has not received the current gamestate gets it again once it acknowledges a later message (sv_client.cpp:1819), and a
client that is not CS_ACTIVE only gets a message every 1000 ms (sv_snapshot.cpp:704)."""
import sys, os, collections, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, codec as C, codec2 as K2, fullload, jkadec as J, bdata

B.ANG16 = True
RAW = fullload.load_all()
FRAG = 1300

def snap_of(msg):
    for op in msg["ops"]:
        if op["t"] == "snap": return op["snap"]
    return None

def engine_lengths(name):
    return {h["seq"]: len(d) for seg, h, d in J.read_capture(bdata.FILES[name])}

def new_sizes(msgs, modes, tables):
    store = {}; bs = {}; out = {}
    for m in msgs:
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
        out[seq] = (len(C.rans_encode(io.u, tables)), [op["t"] for op in msg["ops"]])
    return out

def nfrag(n): return n // FRAG + 1            # a message of exactly k*1300 bytes still sends an empty last fragment

def main():
    msgs = RAW["5min"]; cut = int(len(msgs) * 0.6)
    items = B.data["5min"]; st5 = B.states["5min"]
    modes = B.choose_modes(items[:int(len(items) * 0.6)], st5, 0, None)
    counts = {}
    store = {}; bs = {}
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
    tables = C.Tables(counts, 0.01)

    eng = {k: engine_lengths(k) for k in ("5min", "bots", "solo")}
    new = {k: new_sizes(RAW[k], modes, tables) for k in ("5min", "bots", "solo")}

    print("== (a) messages >= %d bytes (fragmented by the netchan), all three captures, snapshot messages (with or without commands)" % FRAG)
    for label, src in (("engine", lambda k, s: eng[k][s]), ("new codec", lambda k, s: new[k][s][0])):
        tot = 0; fr = 0; mx = 0; extra = 0
        for k in ("5min", "bots", "solo"):
            for s, (nb, ops) in new[k].items():
                if "snap" not in ops: continue
                L = src(k, s); tot += 1; mx = max(mx, L)
                if L >= FRAG: fr += 1; extra += nfrag(L) - 1
        print("  %-10s snapshots %5d  fragmented %4d (%.2f%%)  extra fragments %4d  max message %5d bytes" % (label, tot, fr, 100.0 * fr / tot, extra, mx))

    print("\n== (b) gamestate: fragments, send time and expected re-sends under loss")
    gs = {}
    for k in ("5min", "bots", "solo"):
        for s, (nb, ops) in new[k].items():
            if "gs" in ops: gs[k] = (eng[k][s], nb)
    print("  %-6s %9s %9s %10s %10s" % ("file", "engine B", "new B", "engine fr.", "new fr."))
    for k, (e, n) in gs.items(): print("  %-6s %9d %9d %10d %10d" % (k, e, n, nfrag(e), nfrag(n)))
    e, n = gs["5min"]
    print("  5min gamestate, one attempt succeeds iff all fragments arrive; a lost attempt is re-sent after the next message of the client")
    print("  (a client that is not active receives a message every >= 1000 ms) -> extra join delay ~ (attempts - 1) * (1000 ms + RTT)")
    print("  %-6s | %-28s | %-28s" % ("loss", "engine: P(ok)  attempts  extra delay (RTT 100ms)", "new: P(ok)  attempts  extra delay"))
    for p in (0.0, 0.01, 0.02, 0.05, 0.10, 0.20):
        row = []
        for L in (e, n):
            q = (1 - p) ** nfrag(L); att = 1.0 / q
            row.append("%5.1f%%  %6.2f  %8.2f s" % (100 * q, att, (att - 1) * 1.1))
        print("  %-6s | %-28s | %-28s" % ("%d%%" % round(p * 100), row[0], row[1]))
    print("  send time of the gamestate at rate R (one fragment per rateMsec): engine %d fragments, new %d fragments" % (nfrag(e), nfrag(n)))
    for R in (4000, 8000, 25000):
        ms = (FRAG + 48) * 1000 // R
        print("     rate %5d: engine %5d ms  new %5d ms" % (R, nfrag(e) * ms, nfrag(n) * ms))

    print("\n== (c) rate limiting, test part of the 5-minute capture (snapshot messages incl. commands), snaps 20 (50 ms)")
    ks = [s for s in new["5min"] if "snap" in new["5min"][s][1]]
    test_seqs = ks[int(len(ks) * 0.6):]
    print("  %-8s | %-37s | %-37s" % ("rate B/s", "engine: delayed  interval  snaps/s", "new: delayed  interval  snaps/s"))
    for R in (3000, 4000, 5000, 8000, 12000, 16000, 25000, 99999):
        row = []
        for src in (lambda s: eng["5min"][s], lambda s: new["5min"][s][0]):
            d = 0; tot_ms = 0
            for s in test_seqs:
                size = min(src(s), 1500)
                rm = (size + 48) * 1000 // R
                if rm >= 50: d += 1
                tot_ms += math.ceil(max(50, rm) / 50.0) * 50       # server frames are 50 ms apart
            iv = tot_ms / len(test_seqs)
            row.append("%5.1f%%  %6.1f ms  %6.1f" % (100.0 * d / len(test_seqs), iv, 1000.0 / iv))
        print("  %-8d | %-37s | %-37s" % (R, row[0], row[1]))

if __name__ == "__main__":
    main()
