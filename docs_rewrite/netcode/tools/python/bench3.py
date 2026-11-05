"""Coder-family comparison on the same symbol streams (new schema, gap variant):
ideal static / rANS 12-bit / rANS 15-bit / static Huffman per context / adaptive binary (reset per packet, trained init)."""
import math, heapq, collections, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B

def event_lists(items, st, k, cfg):
    out = []; prevhdr = None
    for i in range(len(items)):
        r, prevhdr = B.snap_events(i, items, st, k, cfg, prevhdr)
        if r is None: continue
        out.append(list(r[0]) + list(r[1]))
    return out

def train_model(evlists):
    M = B.Model()
    for evs in evlists:
        for kind, ctx, K, sym, raw in evs: M.add(ctx, sym)
    return M

# ---------------------------------------------------------------- cost tables
class Tables:
    def __init__(self, M, alpha):
        self.M = M; self.a = alpha
        self.q = {}; self.h = {}

    def freqs(self, ctx, K, P):
        e = self.M.c.get(ctx)
        cnt = e[0] if e else {}
        tot = (e[1] if e else 0) + self.a * K
        scale = (1 << P) / tot
        f = [max(1, int((cnt.get(i, 0) + self.a) * scale)) for i in range(K)]
        diff = (1 << P) - sum(f)
        # put the remainder on the most probable symbol (as rans/fse normalisers do)
        j = max(range(K), key=lambda i: f[i])
        f[j] += diff
        assert f[j] > 0
        return f

    def qcost(self, ctx, K, sym, P):
        key = (ctx, P)
        t = self.q.get(key)
        if t is None:
            f = self.freqs(ctx, K, P)
            t = self.q[key] = [P - math.log2(x) for x in f]
        return t[sym]

    def hcost(self, ctx, K, sym):
        t = self.h.get(ctx)
        if t is None:
            e = self.M.c.get(ctx)
            cnt = e[0] if e else {}
            heap = [(cnt.get(i, 0) + self.a, i, (i,)) for i in range(K)]
            heapq.heapify(heap)
            depth = [0] * K; uid = K
            if K == 1: depth[0] = 1
            while len(heap) > 1:
                w1, _, s1 = heapq.heappop(heap); w2, _, s2 = heapq.heappop(heap)
                for x in s1 + s2: depth[x] += 1
                heapq.heappush(heap, (w1 + w2, uid, s1 + s2)); uid += 1
            t = self.h[ctx] = depth
        return t[sym]

def ideal_cost(M, a, evs):
    return sum(M.cost(ctx, K, sym, a) + raw for kind, ctx, K, sym, raw in evs)

def rans_cost(T, P, evs):
    return sum(T.qcost(ctx, K, sym, P) + raw for kind, ctx, K, sym, raw in evs)

def huff_cost(T, evs):
    return sum(T.hcost(ctx, K, sym) + raw for kind, ctx, K, sym, raw in evs)

class Binarizer:
    """MSB-first binary decomposition of a symbol; node prob initialised from trained counts"""
    def __init__(self, M, a):
        self.M = M; self.a = a; self.init = {}

    def node_p(self, ctx, nb, depth, prefix):
        key = (ctx, depth, prefix)
        p = self.init.get(key)
        if p is None:
            e = self.M.c.get(ctx)
            n0 = n1 = 0
            if e:
                sh = nb - depth - 1
                for s, c in e[0].items():
                    if (s >> (sh + 1)) == prefix:
                        if (s >> sh) & 1: n1 += c
                        else: n0 += c
            p = (n1 + self.a) / (n0 + n1 + 2 * self.a)
            p = min(max(p, 1 / 4096), 1 - 1 / 4096)
            self.init[key] = p
        return p

    def packet_cost(self, evs, rate=5):
        state = {}; bits = 0.0; decisions = 0
        for kind, ctx, K, sym, raw in evs:
            nb = max(1, (K - 1).bit_length())
            prefix = 0
            for d in range(nb):
                bit = (sym >> (nb - 1 - d)) & 1
                key = (ctx, d, prefix)
                p = state.get(key)
                if p is None: p = self.node_p(ctx, nb, d, prefix)
                pb = p if bit else 1 - p
                bits -= math.log2(pb)
                p += ((1.0 if bit else 0.0) - p) / (1 << rate)
                state[key] = min(max(p, 1 / 4096), 1 - 1 / 4096)
                prefix = (prefix << 1) | bit
                decisions += 1
            bits += raw
        return bits, decisions

def run(k=0, quant=None, valcls=False):
    items = B.data["5min"]; st5 = B.states["5min"]
    cut = int(len(items) * 0.6)
    train = items[:cut]; test5 = items[cut:]
    af, as_ = B.tune_alpha(train, st5, k, "gap", valcls, quant)
    modes = B.choose_modes(train, st5, k, quant)
    cfg = dict(modes=modes, quant=quant, variant="gap", valcls=valcls)
    trE = event_lists(train, st5, k, cfg)
    M = train_model(trE)
    T = Tables(M, as_); Bn = Binarizer(M, as_)
    print("model: %d contexts, alpha %.3f" % (len(M.c), as_))
    sets = {"5min-test": (test5, st5), "bots": (B.data["bots"], B.states["bots"]), "solo": (B.data["solo"], B.states["solo"])}
    rows = collections.OrderedDict()
    for name, (it, st) in sets.items():
        E = event_lists(it, st, k, cfg)
        res = collections.OrderedDict()
        res["ideal static"] = [ideal_cost(M, as_, e) for e in E]
        res["rANS 15-bit probs"] = [rans_cost(T, 15, e) for e in E]
        res["rANS 12-bit probs"] = [rans_cost(T, 12, e) for e in E]
        res["static Huffman per ctx"] = [huff_cost(T, e) for e in E]
        ab = [Bn.packet_cost(e) for e in E]
        res["adaptive binary (reset/packet)"] = [x[0] for x in ab]
        rows[name] = (res, sum(x[1] for x in ab) / len(ab), sum(len(e) for e in E) / len(E))
    for name, (res, dec_, nsym) in rows.items():
        print("\n== %s: symbols/snap %.0f, binary decisions/snap %.0f" % (name, nsym, dec_))
        for lab, bl in res.items():
            m, p95, mx = B.stats(bl)
            print("   %-34s mean %7.1f  p95 %5d  max %5d" % (lab, m, p95, mx))

if __name__ == "__main__":
    run()
