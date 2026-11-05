"""Round trip of the selected client demos (plan step 4 check) with the study codec, tables trained as run_full.py.
Run: python run_demos.py [processes]   (default 16; first run also builds data/demos/cache/, about 2.9 GB)"""
import sys, os, io, contextlib
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_full, fullload, demo_select

def init(m, t):
    global MODES, TABLES
    MODES, TABLES = m, t

def check(item):
    s, rel = item
    msgs = fullload.load_demo(rel)
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        bad = run_full.decode_check(msgs, MODES, TABLES, s)
    return rel, len(msgs), bad, out.getvalue()

def main():
    modes, tables = run_full.train()
    sel = demo_select.selection()
    total = failures = 0
    with Pool(int(sys.argv[1]) if len(sys.argv) > 1 else 16, init, (modes, tables)) as p:
        for rel, n, bad, text in p.imap(check, sel):
            total += n; failures += bad
            print(rel); print(text, end="")
    print("demos %d, messages %d, total failures %d" % (len(sel), total, failures))

if __name__ == "__main__":
    main()
