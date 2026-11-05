"""Load complete server messages (every svc op with its payload) from the captures, starting at the first gamestate."""
import sys, os, pickle, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jkadec as J, bdata
CACHE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fullmsgs.pkl")

def s32(v): return v - (1 << 32) if v >= (1 << 31) else v

def load_stream(fn, T, dec):
    cl = J.Client(); sid = None; seen = False; out = []
    for seg, hdr, data in J.read_capture(fn):
        if hdr["sid"] != sid:
            cl = J.Client(); sid = hdr["sid"]; seen = False
        log = []
        try:
            m = J.parse_message(data, hdr, T, dec, cl, log)
        except Exception:
            continue
        if m["end"] not in ("data", "gamestate"):
            continue
        if m["end"] == "gamestate": seen = True
        if not seen: continue
        ack = s32(log[0][2])
        areas = [v for t, b, v in log if t == ("h", "area")][1:] if any(t == ("h", "area") for t, b, v in log) else []
        ops = []
        for o in m["ops"]:
            k = o[0]
            if k == "nop": ops.append(dict(t="nop"))
            elif k == "mapchange": ops.append(dict(t="mapchange"))
            elif k == "servercmd": ops.append(dict(t="cmd", seq=o[4]["seq"], text=o[4]["text"]))
            elif k == "setgame": ops.append(dict(t="setgame", text=o[3]["text"]))
            elif k == "download": ops.append(dict(t="dl", **o[2]))
            elif k == "gamestate": ops.append(dict(t="gs", **o[4]))
            elif k == "snapshot":
                d = o[2]; sn = d["snap"]
                ops.append(dict(t="snap", time=s32(d["serverTime"]) if d["serverTime"] >= 0 else d["serverTime"], delta=d["delta"],
                                flags=d["flags"], areas=areas, snap=dict(ps=sn["ps"], vps=sn.get("vps"), ents=sn["ents"])))
            else:
                raise ValueError(k)
        delta = next((op["delta"] for op in ops if op["t"] == "snap"), 0)
        out.append(dict(seq=hdr["seq"], msg=dict(ack=ack, ops=ops), delta=delta))
    return out

def load_all():
    if os.path.exists(CACHE): return pickle.load(open(CACHE, "rb"))
    T, dec = J.setup()
    res = {k: load_stream(v, T, dec) for k, v in bdata.FILES.items()}
    pickle.dump(res, open(CACHE, "wb"), protocol=4)
    return res

if __name__ == "__main__":
    r = load_all()
    for k, v in r.items():
        c = collections.Counter(op["t"] for m in v for op in m["msg"]["ops"])
        print(k, len(v), "messages", dict(c))
