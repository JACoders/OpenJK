"""Step 6: train the product tables of the new codec (decision of step 5: one global table set) on the 60 training
demos, with the per-field modes and prediction modes chosen on the same demos (as run_step5.py, table set "global").
Writes ../../data/traces/tables.pkl (local, not committed): dict(modes, pmodes, counts, alpha).
Run: python train_tables.py [processes]   (default 30; about 5 minutes)"""
import sys, os, pickle, collections
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, run_pred as RP, run_step5 as S5, demo_select

OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "traces", "tables.pkl"))
ALPHA = 0.01

def task_counts(arg):
    rel, modes, chosen = arg
    S5.C.PMODES = chosen
    return RP.train_units(S5.fullload.load_demo(rel), modes)[0]

def main():
    procs = int(sys.argv[1]) if len(sys.argv) > 1 else 30
    train = [rel for s, rel in demo_select.selection() if s == "train"]
    with Pool(procs, S5.init) as p:
        ms = {}
        for st in p.imap_unordered(S5.task_modes, train): B.merge_mode_stats(ms, st)
        modes = B.pick_modes(ms)
        off = collections.Counter(); on = collections.Counter()
        for o, n in p.imap_unordered(S5.task_pred, [(rel, modes) for rel in train]): off.update(o); on.update(n)
        pmodes = {cn: m for cn, m in RP.candidates().items() if on.get(cn, 0) < off.get(cn, 0) - 8}
        counts = {}
        for c in p.imap_unordered(task_counts, [(rel, modes, pmodes) for rel in train]): S5.merge_counts(counts, c)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    pickle.dump(dict(modes=modes, pmodes=pmodes, counts=counts, alpha=ALPHA), open(OUT, "wb"), protocol=4)
    print("wrote %s: %d fields with a mode, %d prediction modes, %d contexts" % (OUT, len(modes), len(pmodes), len(counts)))

if __name__ == "__main__":
    main()
