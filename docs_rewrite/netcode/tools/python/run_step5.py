"""Step 5 of the plan: the step 1 codec (run_pred.py, integer position prediction) trained on the 60 training demos
and measured on all 80 selected demos (demo_select.py), real bytes, with three table sets:
  global   one table set trained on every training demo
  pergame  one table set per game type, trained on the training demos of that type
  age      global, with the base age (0 = no base, 1, 2, 3, 4 and more) added to every context after the delta
Per-field modes (absolute / delta) and prediction modes are chosen once on the training demos, as in run_pred.py.
Run: python run_step5.py [processes]   (default 30; about 15 minutes; needs the caches of run_demos.py or builds them)"""
import sys, os, csv, math, collections
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, codec as C, codec2 as K2, run_full as RF, run_pred as RP, bdata, fullload, demo_select

AGE_MAX = 4
ENC, DEC = C.EncIO, C.DecIO
RP.ARITH = "pi"                # integer position prediction (step 1 decision)
CONFIGS = ("global", "pergame", "age")

class AgeEnc(ENC):
    age = None
    def s(self, ctx, K, sym):
        if ctx == ("m", "delta"): self.age = min(sym, AGE_MAX)
        elif ctx is not None and self.age is not None: ctx = (ctx, self.age)
        return ENC.s(self, ctx, K, sym)

class AgeDec(DEC):
    age = None
    def s(self, ctx, K):
        if ctx == ("m", "delta"):
            v = DEC.s(self, ctx, K); self.age = min(v, AGE_MAX); return v
        if self.age is not None: ctx = (ctx, self.age)
        return DEC.s(self, ctx, K)

def init():
    K2.enable_prediction(); C.X0 = 1

def task_modes(rel):
    items, st = bdata.load_demo(rel)
    return B.mode_stats(items, st, 0, None)

def task_pred(arg):
    rel, modes = arg
    msgs = fullload.load_demo(rel)
    C.PMODES = {}; off = RP.cost_by_field(*RP.train_units(msgs, modes))
    C.PMODES = RP.candidates(); on = RP.cost_by_field(*RP.train_units(msgs, modes))
    return off, on

def task_counts(arg):
    rel, modes, chosen = arg
    msgs = fullload.load_demo(rel); C.PMODES = chosen
    plain = RP.train_units(msgs, modes)[0]
    C.EncIO = AgeEnc
    try: age = RP.train_units(msgs, modes)[0]
    finally: C.EncIO = ENC
    return plain, age

def task_check(arg):
    rel, cfg, modes, chosen, counts = arg
    msgs = fullload.load_demo(rel); C.PMODES = chosen
    rec = []
    if cfg == "age": RF.EncIO, C.DecIO = AgeEnc, AgeDec
    try: bad = RF.decode_check(msgs, modes, C.Tables(counts, 0.01), rel, rec)
    finally: RF.EncIO, C.DecIO = ENC, DEC
    return rel, cfg, bad, [(kinds, m["delta"], n, m.get("nbytes")) for m, kinds, n in rec]

def merge_counts(acc, counts):
    for ctx, (c, n) in counts.items():
        e = acc.get(ctx)
        if e is None: e = acc[ctx] = [collections.Counter(), 0]
        e[0].update(c); e[1] += n
    return acc

def stat(v):
    v = sorted(v); return len(v), sum(v) / len(v), v[int(len(v) * .95)], v[-1]

def report(title, groups):
    """groups: key -> list of (coded, engine) of snapshot-only messages"""
    print("\n  %s" % title)
    print("    %-44s %7s %8s %5s %5s | %8s %6s" % ("", "n", "mean", "p95", "max", "engine", "ratio"))
    for k in sorted(groups, key=str):
        g = groups[k]; n, m, p, x = stat([a for a, e in g]); em = sum(e for a, e in g) / n
        print("    %-44s %7d %8.1f %5d %5d | %8.1f %6.3f" % (str(k)[:44], n, m, p, x, em, m / em))

def main():
    procs = int(sys.argv[1]) if len(sys.argv) > 1 else 30
    rows = {r["path"]: r for r in csv.DictReader(open(demo_select.OUT, encoding="utf-8"), delimiter="\t")}
    sel = demo_select.selection(); train = [rel for s, rel in sel if s == "train"]
    with Pool(procs, init) as p:
        ms = {}
        for st in p.imap_unordered(task_modes, train): B.merge_mode_stats(ms, st)
        modes = B.pick_modes(ms)
        off = collections.Counter(); on = collections.Counter()
        for o, n in p.imap_unordered(task_pred, [(rel, modes) for rel in train]): off.update(o); on.update(n)
        cand = RP.candidates()
        chosen = {cn: m for cn, m in cand.items() if on.get(cn, 0) < off.get(cn, 0) - 8}
        print("modes: %d fields delta-coded of %d; prediction modes chosen: %d of %d" % (
            sum(1 for v in modes.values() if v == "d"), len(modes), len(chosen), len(cand)))
        for cn in sorted(chosen, key=lambda x: on[x] - off[x]):
            print("   %-28s %-3s %11.0f -> %11.0f" % (cn, chosen[cn], off[cn], on[cn]))
        tabs = {"global": {}, "age": {}}
        for rel, (plain, age) in zip(train, p.imap(task_counts, [(rel, modes, chosen) for rel in train])):
            merge_counts(tabs["global"], plain); merge_counts(tabs["age"], age)
            merge_counts(tabs.setdefault("pergame:" + rows[rel]["gametype"], {}), plain)
        jobs = []
        for s, rel in sel:
            for cfg in CONFIGS:
                key = "pergame:" + rows[rel]["gametype"] if cfg == "pergame" else cfg
                jobs.append((rel, cfg, modes, chosen, tabs[key]))
        res = list(p.imap_unordered(task_check, jobs))
    setof = {rel: s for s, rel in sel}
    print("\nround-trip failures: %d (%d demos x %d table sets)" % (sum(r[2] for r in res), len(sel), len(CONFIGS)))
    for cfg in CONFIGS:
        print("\n### table set: %s  (%s contexts)" % (cfg, ", ".join("%s %d" % (k, len(v)) for k, v in sorted(tabs.items())
                                                        if k == cfg or (cfg == "pergame" and k.startswith("pergame")))))
        snaps = []; allm = collections.defaultdict(lambda: [0, 0])
        for rel, c, bad, recs in res:
            if c != cfg: continue
            r = rows[rel]; s = setof[rel]
            for kinds, delta, n, e in recs:
                if kinds == ("gs",): continue                  # the first message is the gamestate the client rebuilt
                allm[s][0] += n; allm[s][1] += e
                if kinds == ("snap",): snaps.append((s, r, delta, n, e))
        for s, (a, e) in sorted(allm.items()):
            print("  all messages but the gamestate, %-8s coded %d bytes, engine %d, ratio %.3f" % (s, a, e, a / e))
        g = collections.defaultdict(list)
        for s, r, d, n, e in snaps: g[s].append((n, e))
        report("snapshot-only messages by set", g)
        for title, key in (("by set and game type", lambda s, r, d: (s, r["gametype"])),
                           ("by set and snapshot interval (ms)", lambda s, r, d: (s, float(r["dt_ms"]))),
                           ("by game type, snapshot interval (ms) and set", lambda s, r, d: (r["gametype"], float(r["dt_ms"]), s)),
                           ("by set and base age (0 = no base)", lambda s, r, d: (s, min(d, 9))),
                           ("held-out set by map", lambda s, r, d: r["map"] if s == "holdout" else None),
                           ("by server build", lambda s, r, d: (s, r["version"][:34]))):
            g = collections.defaultdict(list)
            for s, r, d, n, e in snaps:
                k = key(s, r, d)
                if k is not None: g[k].append((n, e))
            report(title, g)

if __name__ == "__main__":
    main()
