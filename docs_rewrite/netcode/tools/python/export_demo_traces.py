"""Step 6, part 2: the decoded messages of the corpus for the C++ tools (trainer, size benchmark, round trip), without
any coded bytes: the C++ codec is the reference of the format from here on, and this script is only the .dm_26 reader.
Output (local, not committed, player names): data/traces/train.bin (60 training demos), holdout.bin (20 held-out
demos), captures.bin (the three captures of the study).
Run: python export_demo_traces.py [processes]   (default 30; needs the demo caches of run_demos.py or builds them)

Format, little-endian: "NCDT" u32 version=1 u32 nstreams; per stream: str metadata, u32 nmsg; per message: the
message as in export_codec_trace.py (u32 seq, u32 delta, i64 ack, u32 nops, ops), then u32 engine bytes (the size of
the message the server sent, without svc_EOF). Metadata: the selection.tsv row of the demo, tab-separated
(set, path, gametype, map, version, dt_ms, minutes, players, snap_bytes); for a capture "capture\\t<name>".
Every message of a stream is written, including those whose base is missing; readers skip them as the server would
not have sent them against that base. Streams are independent: a reader can process one at a time."""
import sys, os, csv, struct
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codec2 as K2, run_full as RF, fullload, demo_select, export_codec_trace as X

DIR = os.path.dirname(X.OUT)
COLS = ("set", "path", "gametype", "map", "version", "dt_ms", "minutes", "players", "snap_bytes")

def init():
    K2.enable_prediction(); X.init_indices()

def stream_bytes(meta, msgs):
    recs = [X.msg_bytes(m) + struct.pack("<I", m["nbytes"]) for m in msgs]
    return X.bstr(meta.encode("utf-8")) + struct.pack("<I", len(recs)) + b"".join(recs), len(recs)

def task_demo(row):
    return stream_bytes("\t".join(row[c] for c in COLS), fullload.load_demo(row["path"]))

def write(name, chunks, n):
    path = os.path.join(DIR, name); total = 0
    with open(path, "wb") as f:
        f.write(b"NCDT" + struct.pack("<II", 1, n))
        for data, nm in chunks: f.write(data); total += nm
    print("wrote %s: %d streams, %d messages, %d MB" % (path, n, total, os.path.getsize(path) >> 20))

def main():
    procs = int(sys.argv[1]) if len(sys.argv) > 1 else 30
    init()
    os.makedirs(DIR, exist_ok=True)
    rows = list(csv.DictReader(open(demo_select.OUT, encoding="utf-8"), delimiter="\t"))
    write("captures.bin", (stream_bytes("capture\t" + k, RF.RAW[k]) for k in ("5min", "bots", "solo")), 3)
    with Pool(procs, init) as p:
        for s in ("train", "holdout"):
            sel = [r for r in rows if r["set"] == s]
            write(s + ".bin", p.imap(task_demo, sel), len(sel))

if __name__ == "__main__":
    main()
