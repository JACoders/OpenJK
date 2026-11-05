"""Train / held-out selection of the client demos (plan step 4), from ../../data/demos/index.tsv.

Rules (decided 5 November 2026):
- excluded: files where more than 5 % of the messages cannot be decoded (missing base), files shorter than 3 minutes;
- held-out set by map: in each game type, about 20 % of the maps (at least 1), drawn with a fixed seed; every file of
  those maps is held out, so the held-out set measures maps the tables never saw;
- 60 training and 20 held-out demos, split by game type roughly in proportion to the hours; only the first
  MAX_MSGS messages of each demo are used (about 3 minutes at 30 snapshots per second).
Writes ../../data/demos/selection.tsv. Run: python demo_select.py
"""
import csv, os, random, collections

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "demos"))
INDEX = os.path.join(ROOT, "index.tsv")
OUT = os.path.join(ROOT, "selection.tsv")
SEED = 1
MAX_MSGS = 5400
MAX_ERR = 0.05
MIN_MINUTES = 3.0
HOLDOUT_MAPS = 0.2
COUNTS = {"train": dict(ctf=25, team=18, siege=14, duel=3), "holdout": dict(ctf=8, team=6, siege=5, duel=1)}
COLS = ["set", "path", "gametype", "map", "version", "dt_ms", "minutes", "players", "snap_bytes"]


def main():
    rows = list(csv.DictReader(open(INDEX, encoding="utf-8"), delimiter="\t"))
    ok = [r for r in rows if int(r["err"]) <= MAX_ERR * int(r["msgs"]) and float(r["minutes"] or 0) >= MIN_MINUTES]
    rnd = random.Random(SEED); out = []
    for g in sorted(COUNTS["train"]):
        maps = sorted({r["map"] for r in ok if r["gametype"] == g})
        held = set(rnd.sample(maps, max(1, round(HOLDOUT_MAPS * len(maps)))))
        for s in ("train", "holdout"):
            pool = sorted((r for r in ok if r["gametype"] == g and (r["map"] in held) == (s == "holdout")), key=lambda r: r["path"])
            for r in rnd.sample(pool, min(COUNTS[s][g], len(pool))):
                out.append(dict(r, set=s))
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("\t".join(COLS) + "\n")
        for r in out:
            f.write("\t".join(str(r[k]) for k in COLS) + "\n")
    print("eligible %d of %d files; wrote %s" % (len(ok), len(rows), OUT))
    for s in ("train", "holdout"):
        sel = [r for r in out if r["set"] == s]
        by = collections.Counter(r["gametype"] for r in sel)
        print("%-8s %d demos %s, %d maps, %.0f minutes in full, versions %s, dt %s" % (
            s, len(sel), dict(by), len({r["map"] for r in sel}), sum(float(r["minutes"]) for r in sel),
            dict(collections.Counter(r["version"][:30] for r in sel)), dict(collections.Counter(r["dt_ms"] for r in sel))))
    tm = {r["map"] for r in out if r["set"] == "train"}; hm = {r["map"] for r in out if r["set"] == "holdout"}
    assert not tm & hm, "a map is in both sets"


def selection():
    """[(set, path relative to data/demos)] of selection.tsv."""
    return [(r["set"], r["path"]) for r in csv.DictReader(open(OUT, encoding="utf-8"), delimiter="\t")]


if __name__ == "__main__":
    main()
