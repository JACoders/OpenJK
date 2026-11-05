"""Index of the client demos (plan step 4): decodes every demo under ../../data/demos and writes ../../data/demos/index.tsv.

One line per file: decode result, server and game information from the first gamestate, players, snapshot interval,
duration and mean snapshot size. Files that are not demos (archives) are skipped.
Run: python demo_index.py [processes]
"""
import os, sys, collections, statistics, multiprocessing
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import jkadec as J

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "demos"))
OUT = os.path.join(ROOT, "index.tsv")
SKIP_EXT = (".zip", ".rar", ".7z", ".tsv", ".txt", ".pkl")
GAMETYPES = ["ffa", "holocron", "jedimaster", "duel", "powerduel", "single", "team", "siege", "ctf", "cty"]
COLS = ["path", "bytes", "msgs", "file_end", "ok", "badend", "err", "exc", "gamestates", "snaps", "dt_ms", "minutes",
        "snap_bytes", "ents", "players", "version", "protocol", "gamename", "fs_game", "map", "gametype", "sv_fps", "humans", "bots", "specs"]

def info(s):
    p = s.decode("latin-1").split("\\")[1:]
    return dict(zip(p[0::2], p[1::2]))

def index_one(args):
    path, T, dec = args
    r = dict(path=os.path.relpath(path, ROOT).replace("\\", "/"), bytes=os.path.getsize(path))
    msgs, r["file_end"] = J.read_demo(path)
    r["msgs"] = len(msgs)
    c = collections.Counter(); times = []; sizes = []; ents = []; players = []; cl = J.Client(); gs = None
    for seq, data in msgs:
        try:
            m = J.parse_message(data, dict(seq=seq, time=None, client=None), T, dec, cl, None, demo=True)
        except Exception:
            c["exc"] += 1; continue
        e = m["end"]
        c["gamestates" if e == "gamestate" else "ok" if e == "EOF" else "badend" if e == "BADEND" else "err"] += 1
        for o in m["ops"]:
            if o[0] == "gamestate" and gs is None: gs = dict(o[4]["cs"])
            if o[0] == "snapshot":
                times.append(o[2]["serverTime"]); en = o[2]["snap"]["ents"]
                ents.append(len(en)); players.append(sum(1 for num, _ in en if num < 32))   # MAX_CLIENTS
        if e in ("EOF", "BADEND") and len(m["ops"]) == 1 and m["ops"][0][0] == "snapshot":
            sizes.append(m["size"])       # snapshot-only messages, size without svc_EOF (as the captures)
    r.update({k: c[k] for k in ("ok", "badend", "err", "exc", "gamestates")})
    r["snaps"] = len(times)
    d = [b - a for a, b in zip(times, times[1:]) if 0 < b - a < 1000]
    r["dt_ms"] = statistics.median(d) if d else ""
    r["minutes"] = round(sum(d) / 60000, 1)    # gaps over 1 s (map restarts, pauses) not counted
    r["snap_bytes"] = round(sum(sizes) / len(sizes), 1) if sizes else ""
    r["ents"] = statistics.median(ents) if ents else ""            # entities per snapshot
    r["players"] = statistics.median(players) if players else ""   # client entities visible per snapshot
    if gs:
        si = info(gs.get(0, b"")); sy = info(gs.get(1, b""))
        g = si.get("g_gametype", "")
        r.update(version=si.get("version", ""), protocol=si.get("protocol", ""), gamename=si.get("gamename", ""),
                 fs_game=sy.get("fs_game", ""), map=si.get("mapname", ""), sv_fps=si.get("sv_fps", ""),
                 gametype=GAMETYPES[int(g)] if g.isdigit() and int(g) < len(GAMETYPES) else g)
        pl = [info(b"\\" + v) for k, v in gs.items() if v.startswith(b"n\\") and b"\\t\\" in v]
        r["bots"] = sum(1 for p in pl if "skill" in p)
        r["specs"] = sum(1 for p in pl if "skill" not in p and p.get("t") == "3")
        r["humans"] = len(pl) - r["bots"] - r["specs"]
    return r

def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else max(1, os.cpu_count() - 1)
    files = sorted(os.path.join(dp, f) for dp, _, fs in os.walk(ROOT) for f in fs if not f.lower().endswith(SKIP_EXT))
    T, dec = J.setup()
    with multiprocessing.Pool(n) as pool, open(OUT, "w", encoding="utf-8", newline="\n") as out:
        out.write("\t".join(COLS) + "\n")
        for i, r in enumerate(pool.imap(index_one, [(f, T, dec) for f in files], chunksize=1)):
            out.write("\t".join(str(r.get(k, "")).replace("\t", " ") for k in COLS) + "\n"); out.flush()
            if i % 50 == 0: print(i, "/", len(files), flush=True)
    print("wrote", OUT)

if __name__ == "__main__":
    main()
