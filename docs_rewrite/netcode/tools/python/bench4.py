"""(C) generic compressors with dictionary on the engine's raw (non-Huffman) bit layout
   (D) real 32-bit rANS encode+decode round trip on the new-schema symbol streams (15-bit probabilities)"""
import sys, os, zlib, lzma, bisect, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B, bench3 as B3
from compression import zstd

def raw_packet(s):
    acc = 0; n = 0
    for tag, b, v in s["log"]:
        ab = abs(b)
        acc |= (v & ((1 << ab) - 1)) << n; n += ab
    return acc.to_bytes((n + 7) // 8, "little")

def generic():
    items = B.data["5min"]; cut = int(len(items) * 0.6)
    train = [raw_packet(s) for s in items[:cut]]
    sets = {"5min-test": [raw_packet(s) for s in items[cut:]],
            "bots": [raw_packet(s) for s in B.data["bots"]],
            "solo": [raw_packet(s) for s in B.data["solo"]]}
    print("raw layout (no Huffman) mean bytes: " + ", ".join("%s %.1f" % (k, sum(map(len, v)) / len(v)) for k, v in sets.items()))
    P = zstd.CompressionParameter
    opts = {P.checksum_flag: 0, P.content_size_flag: 0, P.dict_id_flag: 0}
    res = {}
    for dsz in (8192, 32768):
        zd = zstd.ZstdDict(zstd.train_dict(train, dsz).dict_content)
        for lvl in (3, 19):
            for name, pk in sets.items():
                sizes = [len(zstd.compress(p, options={**opts, P.compression_level: lvl}, zstd_dict=zd)) for p in pk]
                res[("zstd dict %dK lvl%d" % (dsz // 1024, lvl), name)] = sizes
    zdict = b"".join(train[-200:])[-32768:]
    for name, pk in sets.items():
        out = []
        for p in pk:
            c = zlib.compressobj(9, zlib.DEFLATED, -15, 9, zlib.Z_DEFAULT_STRATEGY, zdict)
            out.append(len(c.compress(p) + c.flush()))
        res[("deflate raw + 32K preset dict", name)] = out
        res[("lzma raw, no dict", name)] = [len(lzma.compress(p, format=lzma.FORMAT_RAW, filters=[{"id": lzma.FILTER_LZMA2, "preset": 9}])) for p in pk]
    labels = sorted({k[0] for k in res})
    for lab in labels:
        row = []
        for name in sets:
            v = sorted(res[(lab, name)]); row.append("%s mean %.1f p95 %d" % (name, sum(v) / len(v), v[int(len(v) * 0.95)]))
        print("  %-32s %s" % (lab, " | ".join(row)))

# ------------------------------------------------------------------ rANS
PB = 15; PM = (1 << PB) - 1; RL = 1 << 23

def encode(units):
    x = RL; out = bytearray()
    for start, freq in reversed(units):
        xmax = ((RL >> PB) << 8) * freq
        while x >= xmax:
            out.append(x & 255); x >>= 8
        x = ((x // freq) << PB) + (x % freq) + start
    out += x.to_bytes(4, "little")
    return bytes(reversed(out))

def decode(buf, tabs):
    """tabs: list of (f,cum) in coding order -> symbols"""
    pos = 4
    x = int.from_bytes(buf[:4][::-1], "little") if False else int.from_bytes(buf[:4], "big")
    res = []
    for f, cum in tabs:
        slot = x & PM
        s = bisect.bisect_right(cum, slot) - 1
        res.append(s)
        x = f[s] * (x >> PB) + slot - cum[s]
        while x < RL:
            x = (x << 8) | buf[pos]; pos += 1
    return res

def real_rans():
    items = B.data["5min"]; st5 = B.states["5min"]
    cut = int(len(items) * 0.6)
    train = items[:cut]; test = items[cut:]
    af, as_ = B.tune_alpha(train, st5, 0, "gap", False, None)
    cfg = dict(modes=B.choose_modes(train, st5, 0, None), quant=None, variant="gap", valcls=False)
    trE = B3.event_lists(train, st5, 0, cfg)
    M = B3.train_model(trE); T = B3.Tables(M, as_)
    E = B3.event_lists(test, st5, 0, cfg)
    ok = 0; tot = 0; sizes = []; ideal = []
    for idx, evs in enumerate(E):
        units = []; tabs = []; syms = []; rawbits = 0
        cache = {}
        for kind, ctx, K, sym, raw in evs:
            c = cache.get(ctx)
            if c is None:
                f = T.freqs(ctx, K, PB); cum = [0]
                for q in f: cum.append(cum[-1] + q)
                c = cache[ctx] = (f, cum)
            f, cum = c
            units.append((cum[sym], f[sym])); tabs.append(c); syms.append(sym)
            rawbits += raw
        # raw mantissa bits: the event list carries only their count, not their value; their content is incompressible
        # (uniform), so they are accounted as plain bits appended to the packet (no rANS involved)
        buf = encode(units)
        if idx < 400:
            tot += 1
            ok += decode(buf, tabs) == syms
        sizes.append(len(buf) + math.ceil(rawbits / 8))
        ideal.append(B3.ideal_cost(M, as_, evs))
    m = sum(sizes) / len(sizes)
    mi = sum(math.ceil(b / 8) for b in ideal) / len(ideal)
    print("real rANS32 (15-bit): round trip ok %d/%d; mean bytes %.1f vs ideal-ceil %.1f (overhead %.2f B/packet); p95 %d max %d" %
          (ok, tot, m, mi, m - mi, sorted(sizes)[int(len(sizes) * .95)], max(sizes)))

if __name__ == "__main__":
    generic()
    real_rans()
