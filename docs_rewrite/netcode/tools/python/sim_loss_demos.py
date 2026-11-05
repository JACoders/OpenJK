"""Step 5: the loss and latency simulation of sim_loss.py on the demos, with the step 1 codec and one global table set
(modes chosen as run_step5.py). Tables trained on the 60 training demos under the three loss / age mixes of
sim_loss.py, then every held-out demo simulated on the grid. The base age is set by the simulation, not taken from
the demo, so all ages are measured on the same content.
Run: python sim_loss_demos.py [processes]   (default 30)"""
import sys, os, csv, random, collections
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codec as C, sim_loss as SL, run_step5 as S5, run_pred as RP, bench2 as B, fullload, demo_select

MIXES = ((0.0, 1, 1), (0.10, 2, 2), (0.05, 4, 3))       # as sim_loss.main
LOSS = (0.0, 0.02, 0.05, 0.10, 0.20)
AGES = (1, 2, 4, 8)

def task_train(arg):
    rel, modes, chosen = arg
    C.PMODES = chosen; msgs = fullload.load_demo(rel); counts = {}
    for p, r, sd in MIXES:
        SL.simulate(msgs, p, r, random.Random(sd), modes, None, counts=counts)
    return counts

def task_sim(arg):
    rel, p, r, modes, chosen, counts = arg
    C.PMODES = chosen; msgs = fullload.load_demo(rel)
    sizes, ages, bad, nobase, nsnap = SL.simulate(msgs, p, r, random.Random(100 + int(p * 1000) + r), modes, C.Tables(counts, 0.01))
    return rel, p, r, sizes, ages, bad

def main():
    procs = int(sys.argv[1]) if len(sys.argv) > 1 else 30
    rows = {r["path"]: r for r in csv.DictReader(open(demo_select.OUT, encoding="utf-8"), delimiter="\t")}
    sel = demo_select.selection()
    train = [rel for s, rel in sel if s == "train"]; hold = [rel for s, rel in sel if s == "holdout"]
    with Pool(procs, S5.init) as pool:
        ms = {}
        for st in pool.imap_unordered(S5.task_modes, train): B.merge_mode_stats(ms, st)
        modes = B.pick_modes(ms)
        off = collections.Counter(); on = collections.Counter()
        for o, n in pool.imap_unordered(S5.task_pred, [(rel, modes) for rel in train]): off.update(o); on.update(n)
        chosen = {cn: m for cn, m in RP.candidates().items() if on.get(cn, 0) < off.get(cn, 0) - 8}
        counts = {}
        for c in pool.imap_unordered(task_train, [(rel, modes, chosen) for rel in train]): S5.merge_counts(counts, c)
        print("tables: %d contexts (60 training demos, loss / age mixes %s)" % (len(counts), MIXES))
        res = list(pool.imap_unordered(task_sim, [(rel, p, r, modes, chosen, counts) for rel in hold for p in LOSS for r in AGES]))
    print("held-out demos (%d), snapshot messages sent (with or without commands), real bytes" % len(hold))
    print("%-6s %-9s %9s %5s %5s | %9s %9s | %s" % ("loss", "ack age", "mean B", "p95", "max", "no-base", "mean age", "round-trip failures"))
    bygt = collections.defaultdict(list); allbad = 0
    for p in LOSS:
        for r in AGES:
            sz = []; ag = []; bad = 0
            for rel, p_, r_, s, a, b in res:
                if (p_, r_) != (p, r): continue
                sz += s; ag += a; bad += b
                if p == 0.0: bygt[(rows[rel]["gametype"], r)] += s
            sz.sort(); nb = sum(1 for x in ag if x == 0); allbad += bad
            print("%-6s %-9s %9.1f %5d %5d | %8.1f%% %9.2f | %d" % ("%d%%" % round(p * 100), "%d" % r, sum(sz) / len(sz),
                  sz[int(len(sz) * .95)], sz[-1], 100.0 * nb / len(ag), sum(x for x in ag if x) / max(1, len(ag) - nb), bad))
    print("total round-trip failures", allbad)
    print("\nno loss, mean real bytes by game type and ack age")
    gts = sorted({g for g, r in bygt})
    print("  %-8s" % "" + "".join("%9s" % ("age %d" % r) for r in AGES))
    for g in gts:
        print("  %-8s" % g + "".join("%9.1f" % (sum(bygt[(g, r)]) / len(bygt[(g, r)])) for r in AGES))

if __name__ == "__main__":
    main()
