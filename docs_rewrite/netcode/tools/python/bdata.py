"""Load captures into per-snapshot records with call logs (tag, bits, value) and full states. Cached as pickle."""
import sys, os, pickle, math, hashlib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jkadec as J, demo_select

TMP = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data"))
FILES = {
    "5min": os.path.join(TMP, "ybeproxy_netcapture_bots_5mins.bin"),
    "bots": os.path.join(TMP, "ybeproxy_netcapture_with_bots.bin"),
    "solo": os.path.join(TMP, "ybeproxy_netcapture_no_bots.bin"),
}
CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "bdata.pkl")
DEMO_CACHE = os.path.join(demo_select.ROOT, "cache")    # one cache file per demo (data/demos is not committed)

def depth_table(dec):
    left, right, sym, root = dec
    depth = {}
    st = [(root, 0)]
    while st:
        n, d = st.pop()
        if sym[n] == J.INTERNAL:
            st.append((left[n], d + 1)); st.append((right[n], d + 1))
        else:
            depth[sym[n]] = d
    return depth

def load_one(fn, T, dec, demo=False):
    """demo=True: fn is a client demo (only its first demo_select.MAX_MSGS messages); sizes exclude svc_EOF."""
    cl = J.Client(); sid = None; out = []; seen_gs = False; states = {}
    ends = ("EOF", "BADEND") if demo else ("data",)
    for seg, hdr, data in (J.demo_capture(fn, demo_select.MAX_MSGS) if demo else J.read_capture(fn)):
        if hdr["sid"] != sid:
            cl = J.Client(); sid = hdr["sid"]; seen_gs = False
        log = []
        try:
            m = J.parse_message(data, hdr, T, dec, cl, log, demo)
        except Exception:
            continue
        if m["end"] == "gamestate":
            seen_gs = True; continue
        if m["end"] not in ends or not seen_gs:
            continue
        ops = [o for o in m["ops"] if o[0] == "snapshot"]
        for o_ in ops:
            states[hdr["seq"]] = o_[2]["snap"]
        if len(ops) != 1 or len(m["ops"]) != 1:
            continue          # snapshot-only messages (no server commands)
        o = ops[0][2]
        out.append(dict(seq=hdr["seq"], time=hdr["time"], nbytes=m["size"] if demo else len(data),
                        bits=ops[0][1] + o["p0"],        # ack + snapshot op
                        log=log, snap=o["snap"], flags=o["flags"], delta=o["delta"],
                        nents_coded=len(o["rec_ent"]), baselines=cl.baselines))
    return out, states

def load_all():
    if os.path.exists(CACHE):
        return pickle.load(open(CACHE, "rb"))
    T, dec = J.setup()
    data = {}; states = {}
    for k, v in FILES.items():
        data[k], states[k] = load_one(v, T, dec)
    obj = dict(T=T, dec=dec, data=data, states=states)
    pickle.dump(obj, open(CACHE, "wb"), protocol=4)
    return obj

def demo_cache(rel, kind):
    return os.path.join(DEMO_CACHE, hashlib.sha1(rel.encode("utf-8")).hexdigest()[:16] + "." + kind + ".pkl")

def load_demo(rel):
    """(snapshot records, states) of one selected demo (path relative to data/demos), cached."""
    c = demo_cache(rel, "bdata")
    if os.path.exists(c):
        return pickle.load(open(c, "rb"))
    T, dec = J.setup()
    obj = load_one(os.path.join(demo_select.ROOT, rel), T, dec, demo=True)
    os.makedirs(DEMO_CACHE, exist_ok=True)
    pickle.dump(obj, open(c, "wb"), protocol=4)
    return obj

if __name__ == "__main__":
    o = load_all()
    for k, v in o["data"].items():
        print(k, len(v), "snapshots, mean bits %.0f" % (sum(s["bits"] for s in v) / len(v)),
              "span s %.1f" % ((v[-1]["time"] - v[0]["time"]) / 1000))
