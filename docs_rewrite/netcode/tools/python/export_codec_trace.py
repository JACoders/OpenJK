"""Step 6: golden trace of the whole-message codec for the C++ tests (codemp/netcodec/tests/test_codec.cpp).
Codec as the product: prediction field order, prediction modes, per-field modes and tables of train_tables.py,
initial state X0 = 1. For every message the server-side loop of run_full.encode_stream codes, writes the message
(as decoded by jkadec) and the bytes of the Python codec.
Streams: the three captures, then demos (paths relative to data/demos; default: one held-out demo per game type and
the first training demo of a siege map with vehicles). Output (local, not committed): data/traces/codec_trace.bin.
Run: python export_codec_trace.py [demo ...]

Format, little-endian: "NCCT" u32 version=1 u32 nstreams; per stream: u32 nmsg; per message: u32 seq, u32 delta,
i64 ack, u32 nops, ops, u32 nbytes, bytes. Op: u8 type (1 nop, 2 gamestate, 3 command, 4 snapshot, 5 setgame,
6 download, 7 mapchange), then: command i64 seq, str; setgame str; gamestate i64 relseq, u32 n, n x (u16 index, str),
ents, i64 clientNum, i64 checksum; download u32 block, i64 fsize (0 when absent), str data; snapshot i64 time,
u32 flags, u32 nareas, areas (u8), ps, u8 hasVehicle, [ps], ents.
ps: u16 n, n x (u16 index in the player-name list of the C++ schema, u32 value); ents: u32 n, n x (u16 number,
u16 nf, nf x (u16 index in the entity field list, u32 value)); str: u32 length, bytes. Values are 32-bit patterns."""
import sys, os, struct, pickle
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codec as C, codec2 as K2, run_full as RF, fullload, demo_select, train_tables

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "..", "data", "traces", "codec_trace.bin"))
OPS = {"nop": 1, "gs": 2, "cmd": 3, "snap": 4, "setgame": 5, "dl": 6, "mapchange": 7}

def default_demos():
    rows = demo_select.selection(); out = []; seen = set()
    import csv
    meta = {r["path"]: r for r in csv.DictReader(open(demo_select.OUT, encoding="utf-8"), delimiter="\t")}
    for s, rel in rows:
        g = meta[rel]["gametype"]
        if s == "holdout" and g not in seen: seen.add(g); out.append(rel)
    out += [rel for s, rel in rows if s == "train" and "hoth" in meta[rel]["map"]][:1]
    return out

EIDX = PIDX = None

def init_indices():
    """Field indices of the C++ schema; call after K2.enable_prediction(), which reorders the entity fields."""
    global EIDX, PIDX
    EIDX = {n: i for i, (n, b) in enumerate(C.EF)}; PIDX = {n: i for i, n in enumerate(K2.ALLNAMES)}

def u32(v): return struct.pack("<I", v & 0xFFFFFFFF)
def i64(v): return struct.pack("<q", v)
def bstr(b): return struct.pack("<I", len(b)) + bytes(b)

def ps_bytes(ps):
    items = [(PIDX[n], v) for n, v in ps.items() if n in PIDX and v != 0]
    return struct.pack("<H", len(items)) + b"".join(struct.pack("<H", i) + u32(v) for i, v in sorted(items))

def ents_bytes(ents):
    out = [struct.pack("<I", len(ents))]
    for num, st in ents:
        items = sorted((EIDX[n], v) for n, v in st.items() if n in EIDX and v != 0)
        out.append(struct.pack("<HH", num, len(items)) + b"".join(struct.pack("<H", i) + u32(v) for i, v in items))
    return b"".join(out)

def op_bytes(op):
    t = op["t"]; b = struct.pack("<B", OPS[t])
    if t == "cmd": return b + i64(op["seq"]) + bstr(op["text"])
    if t == "setgame": return b + bstr(op["text"])
    if t == "gs":
        return (b + i64(op["rseq"]) + struct.pack("<I", len(op["cs"])) +
                b"".join(struct.pack("<H", i) + bstr(x) for i, x in op["cs"]) + ents_bytes(op["baselines"]) +
                i64(op["clientNum"]) + i64(op["checksum"]))
    if t == "dl": return b + struct.pack("<I", op["block"]) + i64(op["fsize"] or 0) + bstr(op["data"])
    if t == "snap":
        sn = op["snap"]
        return (b + i64(op["time"]) + struct.pack("<II", op["flags"], len(op["areas"])) + bytes(op["areas"]) +
                ps_bytes(sn["ps"]) + struct.pack("<B", sn["vps"] is not None) +
                (ps_bytes(sn["vps"]) if sn["vps"] is not None else b"") + ents_bytes(sn["ents"]))
    return b

def msg_bytes(m):
    """seq, delta, ack, nops and the operations of one message"""
    msg = m["msg"]
    return (struct.pack("<II", m["seq"], m["delta"]) + i64(msg["ack"]) + struct.pack("<I", len(msg["ops"])) +
            b"".join(op_bytes(op) for op in msg["ops"]))

def main():
    K2.enable_prediction(); init_indices()
    tr = pickle.load(open(train_tables.OUT, "rb"))
    C.PMODES = tr["pmodes"]; C.X0 = 1
    modes = tr["modes"]; tables = C.Tables(tr["counts"], tr["alpha"])

    streams = [RF.RAW[k] for k in ("5min", "bots", "solo")]
    demos = sys.argv[1:] or default_demos()
    streams += [fullload.load_demo(rel) for rel in demos]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    total = 0
    with open(OUT, "wb") as f:
        f.write(b"NCCT" + struct.pack("<II", 1, len(streams)))
        for msgs in streams:
            recs = []
            for m, units, _ in RF.encode_stream(msgs, modes):
                recs.append(msg_bytes(m) + bstr(C.rans_encode(units, tables)))
            f.write(struct.pack("<I", len(recs)) + b"".join(recs)); total += len(recs)
    print("wrote %s: %d streams (%d demos), %d messages, %d MB" % (OUT, len(streams), len(demos), total, os.path.getsize(OUT) >> 20))

if __name__ == "__main__":
    main()
