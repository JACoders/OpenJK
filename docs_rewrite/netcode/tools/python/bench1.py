"""Layout-preserving ladder: same bitstream layout as the engine, only the entropy coding changes.
L0 actual Huffman (replayed from call log, must equal measured bits)
L1 static order-0 byte model, ideal (arith/rANS/tANS bound)         -> "replace Huffman only"
L1h canonical Huffman on the same byte histogram                     -> sanity vs engine tree
L2 static model per call tag (field), same layout (lc kept)         -> field-conditioned
"""
import math, collections, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bdata

obj = bdata.load_all()
data = obj["data"]; dec = obj["dec"]
DEPTH = bdata.depth_table(dec)

def split(items, frac=0.6):
    n = int(len(items) * frac)
    return items[:n], items[n:]

def call_bytes(b, v):
    b = abs(b); nb = b & 7
    return nb, [(v >> (nb + 8 * i)) & 255 for i in range((b - nb) // 8)]

def stats(bits_list, overhead_bytes=0):
    by = sorted(math.ceil(b / 8) + overhead_bytes for b in bits_list)
    n = len(by)
    return sum(by) / n, by[int(n * 0.95)], by[-1]

# ---- L0
def l0_bits(s):
    tot = 0
    for tag, b, v in s["log"]:
        nb, bs = call_bytes(b, v)
        tot += nb + sum(DEPTH[x] for x in bs)
    return tot

# ---- L1
def train_bytes(snaps):
    c = collections.Counter()
    for s in snaps:
        for tag, b, v in s["log"]:
            c.update(call_bytes(b, v)[1])
    return c

def l1_bits_fn(cnt, alpha=0.5):
    N = sum(cnt.values()) + alpha * 256
    cost = [-math.log2((cnt.get(i, 0) + alpha) / N) for i in range(256)]
    def f(s):
        tot = 0.0
        for tag, b, v in s["log"]:
            nb, bs = call_bytes(b, v)
            tot += nb
            for x in bs: tot += cost[x]
        return tot
    return f

def huff_lengths(cnt, alpha=0.5):
    import heapq
    items = [(cnt.get(i, 0) + alpha, i, None, None) for i in range(256)]
    heap = [(w, i, (i,)) for w, i, _, _ in items]
    heapq.heapify(heap)
    depth = [0] * 256
    uid = 256
    while len(heap) > 1:
        w1, _, s1 = heapq.heappop(heap); w2, _, s2 = heapq.heappop(heap)
        for x in s1 + s2: depth[x] += 1
        heapq.heappush(heap, (w1 + w2, uid, s1 + s2)); uid += 1
    return depth

def l1h_bits_fn(cnt):
    d = huff_lengths(cnt)
    def f(s):
        tot = 0
        for tag, b, v in s["log"]:
            nb, bs = call_bytes(b, v)
            tot += nb + sum(d[x] for x in bs)
        return tot
    return f

# ---- L2: context per tag; symbol = whole value if <=10 bits else per byte
def l2_symbols(tag, b, v):
    ab = abs(b)
    if ab <= 10:
        return [((tag, 0), 1 << ab, v)]
    nb, bs = call_bytes(b, v)
    out = []
    if nb: out.append(((tag, "lo"), 1 << nb, v & ((1 << nb) - 1)))
    for i, x in enumerate(bs): out.append(((tag, i), 256, x))
    return out

def l2_train(snaps):
    cnt = collections.defaultdict(collections.Counter); tot = collections.Counter()
    for s in snaps:
        for tag, b, v in s["log"]:
            for ctx, K, sym in l2_symbols(tag, b, v):
                cnt[ctx][sym] += 1; tot[ctx] += 1
    return cnt, tot

def l2_bits_fn(model, alpha):
    cnt, tot = model
    def f(s):
        t = 0.0
        for tag, b, v in s["log"]:
            for ctx, K, sym in l2_symbols(tag, b, v):
                c = cnt.get(ctx)
                if c is None:
                    t += math.log2(K)
                else:
                    t -= math.log2((c.get(sym, 0) + alpha) / (tot[ctx] + alpha * K))
        return t
    return f

def best_alpha(train, fn_builder, alphas=(0.01, 0.03, 0.1, 0.3, 1.0)):
    a_tr, a_va = split(train, 0.75)
    best = None
    for a in alphas:
        f = fn_builder(a_tr, a)
        c = sum(f(s) for s in a_va)
        if best is None or c < best[0]: best = (c, a)
    return best[1]

def run():
    tr, te = split(data["5min"])
    print("train %d  test %d snapshots (time split 60/40)" % (len(tr), len(te)))
    sets = {"5min-test": te, "bots": data["bots"], "solo": data["solo"]}
    # check L0 == measured
    bad = sum(1 for s in te if l0_bits(s) + 0 != s["bits"] - 0 and abs(l0_bits(s) - s["bits"]) > 0)
    print("L0 replay mismatches vs measured bits:", bad, "of", len(te))
    for name, ss in sets.items():
        if name == "5min-test":
            trn = tr
        else:
            trn = data["5min"]
        cnt = train_bytes(trn)
        l1 = l1_bits_fn(cnt); l1h = l1h_bits_fn(cnt)
        a = best_alpha(trn, lambda x, al: l2_bits_fn(l2_train(x), al))
        l2 = l2_bits_fn(l2_train(trn), a)
        print("\n== %s (n=%d)   bytes/snapshot: mean p95 max" % (name, len(ss)))
        rows = [("measured (engine Huffman)", [s["bits"] for s in ss]),
                ("L0 replay", [l0_bits(s) for s in ss]),
                ("L1 order-0 static ideal", [l1(s) for s in ss]),
                ("L1h canonical Huffman order-0", [l1h(s) for s in ss]),
                ("L2 per-field static ideal (alpha=%.2f)" % a, [l2(s) for s in ss])]
        for n, bl in rows:
            m, p95, mx = stats(bl)
            print("  %-42s %8.1f %6d %6d" % (n, m, p95, mx))

if __name__ == "__main__":
    run()
