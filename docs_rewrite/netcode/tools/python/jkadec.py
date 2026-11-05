"""Offline decoder for ybeproxy_netcapture.bin (JKA 1.01 server->client messages).
Ports huffman.cpp (tree build) and the MSG_Read* / CL_Parse* paths of codemp as-is.
Run with: python -I jkadec.py file.bin [more.bin]
"""
import re, struct, sys, pickle, os, collections

_here = os.path.dirname(os.path.abspath(__file__))
# the copy of msg.cpp kept in data/ is the version the study used; the repository copy is only a fallback
SRC = os.path.normpath(os.path.join(_here, "..", "..", "data", "msg.cpp"))
if not os.path.exists(SRC):
    SRC = os.path.normpath(os.path.join(_here, "..", "..", "..", "..", "codemp", "qcommon", "msg.cpp"))
GENTITYNUM_BITS = 10
MAX_GENTITIES = 1 << GENTITYNUM_BITS
FLOAT_INT_BITS = 13
FLOAT_INT_BIAS = 1 << (FLOAT_INT_BITS - 1)
NYT = 256
INTERNAL = 257
PACKET_BACKUP = 32
MAX_WEAPONS = 19  # checked below against source

# ---------------------------------------------------------------- huffman
class Node:
    __slots__ = ("parent", "left", "right", "next", "prev", "head", "weight", "symbol")
    def __init__(self):
        self.parent = self.left = self.right = self.next = self.prev = None
        self.head = None
        self.weight = 0
        self.symbol = 0

class Huff:
    def __init__(self):
        n = Node(); n.symbol = NYT; n.weight = 0
        self.tree = self.lhead = n
        self.loc = {NYT: n}

def _swap(h, n1, n2):
    p1, p2 = n1.parent, n2.parent
    if p1:
        if p1.left is n1: p1.left = n2
        else: p1.right = n2
    else:
        h.tree = n2
    if p2:
        if p2.left is n2: p2.left = n1
        else: p2.right = n1
    else:
        h.tree = n1
    n1.parent = p2
    n2.parent = p1

def _swaplist(n1, n2):
    p = n1.next; n1.next = n2.next; n2.next = p
    p = n1.prev; n1.prev = n2.prev; n2.prev = p
    if n1.next is n1: n1.next = n2
    if n2.next is n2: n2.next = n1
    if n1.next: n1.next.prev = n1
    if n2.next: n2.next.prev = n2
    if n1.prev: n1.prev.next = n1
    if n2.prev: n2.prev.next = n2

def _increment(h, node):
    if node is None:
        return
    if node.next is not None and node.next.weight == node.weight:
        lnode = node.head[0]
        if lnode is not node.parent:
            _swap(h, lnode, node)
        _swaplist(lnode, node)
    if node.prev is not None and node.prev.weight == node.weight:
        node.head[0] = node.prev
    else:
        node.head[0] = None
    node.weight += 1
    if node.next is not None and node.next.weight == node.weight:
        node.head = node.next.head
    else:
        node.head = [node]
    if node.parent:
        _increment(h, node.parent)
        if node.prev is node.parent:
            _swaplist(node, node.parent)
            if node.head[0] is node:
                node.head[0] = node.parent

def add_ref(h, ch):
    if ch not in h.loc:
        tnode = Node(); tnode2 = Node()
        tnode2.symbol = INTERNAL
        tnode2.weight = 1
        tnode2.next = h.lhead.next
        if h.lhead.next:
            h.lhead.next.prev = tnode2
            if h.lhead.next.weight == 1:
                tnode2.head = h.lhead.next.head
            else:
                tnode2.head = [tnode2]
        else:
            tnode2.head = [tnode2]
        h.lhead.next = tnode2
        tnode2.prev = h.lhead

        tnode.symbol = ch
        tnode.weight = 1
        tnode.next = h.lhead.next
        if h.lhead.next:
            h.lhead.next.prev = tnode
            if h.lhead.next.weight == 1:
                tnode.head = h.lhead.next.head
            else:
                tnode.head = [tnode2]   # "should never happen" in the original
        else:
            tnode.head = [tnode]
        h.lhead.next = tnode
        tnode.prev = h.lhead
        tnode.left = tnode.right = None

        if h.lhead.parent:
            if h.lhead.parent.left is h.lhead: h.lhead.parent.left = tnode2
            else: h.lhead.parent.right = tnode2
        else:
            h.tree = tnode2
        tnode2.right = tnode
        tnode2.left = h.lhead
        tnode2.parent = h.lhead.parent
        h.lhead.parent = tnode.parent = tnode2
        h.loc[ch] = tnode
        _increment(h, tnode2.parent)
    else:
        _increment(h, h.loc[ch])

def load_hdata(src):
    lines = src.splitlines()
    start = None
    for i, l in enumerate(lines):
        if re.match(r"int msg_hData\[256\]\s*=\s*\{", l):
            start = i
    assert start is not None
    vals = []
    for l in lines[start + 1:]:
        m = re.match(r"\s*(\d+),\s*//\s*(\d+)", l)
        if m:
            assert int(m.group(2)) == len(vals)
            vals.append(int(m.group(1)))
        if l.startswith("};"):
            break
    assert len(vals) == 256
    return vals

def build_decode_table(hdata, cache):
    if os.path.exists(cache):
        return pickle.load(open(cache, "rb"))
    h = Huff()
    for i in range(256):
        for _ in range(hdata[i]):
            add_ref(h, i)
    # flatten tree into arrays for a fast walk: node id -> (left,right) or symbol
    ids = {}
    order = []
    stack = [h.tree]
    while stack:
        n = stack.pop()
        ids[id(n)] = len(order); order.append(n)
        if n.symbol == INTERNAL:
            stack.append(n.right); stack.append(n.left)
    left = [0] * len(order); right = [0] * len(order); sym = [0] * len(order)
    for n in order:
        k = ids[id(n)]
        sym[k] = n.symbol
        if n.symbol == INTERNAL:
            left[k] = ids[id(n.left)]; right[k] = ids[id(n.right)]
    out = (left, right, sym, ids[id(h.tree)])
    pickle.dump(out, open(cache, "wb"))
    return out

# ---------------------------------------------------------------- field tables
def load_tables(src):
    def block(name, nth=0):
        idx = [m.start() for m in re.finditer(r"netField_t\s+" + name + r"\[\]\s*=", src)][nth]
        end = src.index("\n};", idx)
        body = re.sub(r"//[^\n]*", "", src[idx:end])
        body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
        # drop preprocessor-excluded XBOX lines is not needed: only #ifndef _XBOX blocks exist here
        out = []
        for m in re.finditer(r"\{\s*(?:NETF|PSF)\((.+?)\),\s*([-\w]+)\s*\}", body):
            b = m.group(2)
            b = {"GENTITYNUM_BITS": GENTITYNUM_BITS, "MAX_POWERUPS": 16}.get(b) or int(b)   # OpenJK spells two widths as macros
            out.append((m.group(1), b))
        return out
    return dict(
        ent=block("entityStateFields"),
        ps=block("playerStateFields", 0),
        pilot=block("pilotPlayerStateFields"),
        veh=block("vehPlayerStateFields"),
    )

# ---------------------------------------------------------------- bit reader
class Reader:
    def __init__(self, data, dec):
        self.d = data
        self.bit = 0
        self.left, self.right, self.sym, self.root = dec
        self.log = None
        self.tag = None

    def getbit(self):
        b = (self.d[self.bit >> 3] >> (self.bit & 7)) & 1
        self.bit += 1
        return b

    def read(self, bits):
        sgn = bits < 0
        if sgn: bits = -bits
        b0 = bits
        value = 0
        nbits = bits & 7
        for i in range(nbits):
            value |= self.getbit() << i
        bits -= nbits
        d = self.d; left = self.left; right = self.right; sym = self.sym
        i = 0
        while i < bits:
            n = self.root
            while sym[n] == INTERNAL:
                n = right[n] if (d[self.bit >> 3] >> (self.bit & 7)) & 1 else left[n]
                self.bit += 1
            value |= sym[n] << (i + nbits)
            i += 8
        if self.log is not None:
            self.log.append((self.tag, -b0 if sgn else b0, value))
        if sgn and value & (1 << (bits - 1)):   # same quirk as the original: uses reduced `bits`
            value |= -1 ^ ((1 << bits) - 1)
        return value

    def byte(self):  return self.read(8) & 0xFF
    def short(self):
        v = self.read(16) & 0xFFFF
        return v - 65536 if v & 0x8000 else v
    def long(self):
        v = self.read(32) & 0xFFFFFFFF
        return v - (1 << 32) if v & 0x80000000 else v
    def string(self):
        out = bytearray()
        while True:
            c = self.byte()
            if c == 0: break
            out.append(c)
        return bytes(out)

# ---------------------------------------------------------------- delta readers
def read_entity(r, T, old, number, rec):
    """MSG_ReadDeltaEntity. Returns new state dict (or None for remove). rec collects (name,bits,kind,value,bitcost)"""
    start = r.bit
    r.tag = ("e", "#rm")
    if r.read(1) == 1:
        rec.append(("#remove", 0, "rm", 0, r.bit - start))
        return None
    r.tag = ("e", "#nd")
    if r.read(1) == 0:
        rec.append(("#nodelta", 0, "nd", 0, r.bit - start))
        s = dict(old); s["number"] = number
        return s
    fields = T["ent"]
    r.tag = ("e", "#lc")
    lc = r.byte()
    s = dict(old); s["number"] = number
    rec.append(("#lc", 8, "lc", lc, r.bit - start))
    for i in range(lc):
        name, bits = fields[i]
        p = r.bit
        r.tag = ("e", name, "f")
        if not r.read(1):
            rec.append((name, bits, "same", 0, r.bit - p))
            continue
        r.tag = ("e", name, "k")
        if bits == 0:
            if r.read(1) == 0:
                s[name] = 0; kind = "f0"
            elif r.read(1) == 0:
                r.tag = ("e", name, "v")
                t = r.read(FLOAT_INT_BITS) - FLOAT_INT_BIAS
                s[name] = struct.unpack("<I", struct.pack("<f", float(t)))[0]; kind = "fint"
            else:
                r.tag = ("e", name, "v")
                s[name] = r.read(32) & 0xFFFFFFFF; kind = "ffull"
        else:
            if r.read(1) == 0:
                s[name] = 0; kind = "i0"
            else:
                r.tag = ("e", name, "v")
                s[name] = r.read(bits); kind = "int"
        rec.append((name, bits, kind, s[name], r.bit - p))
    return s

def read_ps(r, T, old, is_veh, rec):
    """MSG_ReadDeltaPlayerstate (optimized vehicle networking, no onebit combo)."""
    s = dict(old) if old else {}
    if is_veh:
        fields = T["veh"]; nf = len(fields)
    else:
        p = r.bit
        r.tag = ("p", "#pilot")
        pilot = r.read(1)
        rec.append(("#isPilot", 1, "int", pilot, r.bit - p))
        if pilot:
            fields = T["pilot"]; nf = len(fields) - 82
        else:
            fields = T["ps"]; nf = len(fields)
    p = r.bit
    r.tag = ("p", "#lc")
    lc = r.byte()
    rec.append(("#lc", 8, "lc", lc, r.bit - p))
    assert lc <= len(fields), "lc %d > %d" % (lc, len(fields))
    for i in range(lc):
        name, bits = fields[i]
        p = r.bit
        r.tag = ("p", name, "f")
        if not r.read(1):
            rec.append((name, bits, "same", 0, r.bit - p))
            continue
        r.tag = ("p", name, "k")
        if bits == 0:
            if r.read(1) == 0:
                r.tag = ("p", name, "v")
                t = r.read(FLOAT_INT_BITS) - FLOAT_INT_BIAS
                s[name] = struct.unpack("<I", struct.pack("<f", float(t)))[0]; kind = "fint"
            else:
                r.tag = ("p", name, "v")
                s[name] = r.read(32) & 0xFFFFFFFF; kind = "ffull"
        else:
            r.tag = ("p", name, "v")
            s[name] = r.read(bits); kind = "int"
        rec.append((name, bits, kind, s[name], r.bit - p))
    # arrays
    p = r.bit
    r.tag = ("p", "#arr")
    if r.read(1):
        if r.read(1):
            bm = r.short() & 0xFFFF
            for i in range(16):
                if bm & (1 << i):
                    s["stats[%d]" % i] = r.read(MAX_WEAPONS) if i == 4 else r.short()   # STAT_WEAPONS checked at load
        if r.read(1):
            bm = r.short() & 0xFFFF
            for i in range(16):
                if bm & (1 << i): s["persistant[%d]" % i] = r.short()
        if r.read(1):
            bm = r.short() & 0xFFFF
            for i in range(16):
                if bm & (1 << i): s["ammo[%d]" % i] = r.short()
        if r.read(1):
            bm = r.short() & 0xFFFF
            for i in range(16):
                if bm & (1 << i): s["powerups[%d]" % i] = r.long()
    rec.append(("#arrays", 0, "arr", 0, r.bit - p))
    return s

# ---------------------------------------------------------------- message parser
class Client:
    def __init__(self):
        self.baselines = {}
        self.snaps = {}      # seq -> dict(ps, vps, ents(list of (num,state)))

def parse_message(data, hdr, T, dec, cl, log):
    r = Reader(data, dec)
    r.log = log
    r.tag = ("h", "ack")
    seq = hdr["seq"]
    out = {"seq": seq, "time": hdr["time"], "client": hdr["client"], "len": len(data), "ops": []}
    r.long()    # reliableAcknowledge
    while True:
        if len(data) * 8 - r.bit <= 8:   # capture is taken before the EOF byte is appended
            out["end"] = "data"; break
        p0 = r.bit
        r.tag = ("h", "cmd")
        cmd = r.byte()
        if cmd == 10:
            out["end"] = "EOF"; break
        if cmd == 1:
            out["ops"].append(("nop", r.bit - p0))
        elif cmd == 5:
            n = r.long(); s = r.string()
            out["ops"].append(("servercmd", r.bit - p0, n, s[:60], dict(seq=n, text=bytes(s))))
        elif cmd == 8:
            sg = r.string()
            out["ops"].append(("setgame", r.bit - p0, sg, dict(text=bytes(sg))))
        elif cmd == 6:
            blk = r.short() & 0xFFFF
            fsz = r.long() if blk == 0 else None
            n = r.short() & 0xFFFF
            data_ = bytes(r.byte() for _ in range(n))
            out["ops"].append(("download", r.bit - p0, dict(block=blk, fsize=fsz, data=data_)))
        elif cmd == 9:
            out["ops"].append(("mapchange", r.bit - p0))
        elif cmd == 2:
            rseq = r.long()
            ncs = nbase = 0
            cl.baselines = {}
            cs_list = []; base_list = []
            while True:
                c = r.byte()
                if c == 3:
                    idx = r.short(); txt = r.string(); ncs += 1
                    cs_list.append((idx, bytes(txt)))
                elif c == 4:
                    num = r.read(GENTITYNUM_BITS)
                    rec = []
                    st = read_entity(r, T, {}, num, rec)
                    cl.baselines[num] = st; nbase += 1
                    base_list.append((num, st))
                elif c == 10:
                    break
                else:
                    raise ValueError("bad gamestate cmd %d at bit %d" % (c, r.bit))
            cnum = r.long(); csum = r.long()
            out["ops"].append(("gamestate", r.bit - p0, ncs, nbase,
                               dict(rseq=rseq, cs=cs_list, baselines=base_list, clientNum=cnum, checksum=csum)))
            out["gs_remaining_bits"] = len(data) * 8 - r.bit
            out["end"] = "gamestate"
            return out
        elif cmd == 7:
            snap = {"ents": []}
            rec_ps = []; rec_vps = []; rec_ent = []
            snap_p0 = p0
            r.tag = ("h", "time"); st = r.long()
            r.tag = ("h", "delta"); delta = r.byte()
            r.tag = ("h", "flags"); flags = r.byte()
            r.tag = ("h", "area"); nareas = r.byte()
            for _ in range(nareas): r.byte()
            old = None
            if delta:
                old = cl.snaps.get(seq - delta)
                if old is None:
                    out["ops"].append(("snapshot-missing-base", 0, delta))
                    out["end"] = "ERR"; return out
            hdr_end = r.bit
            snap["ps"] = read_ps(r, T, old["ps"] if old else None, False, rec_ps)
            snap["vps"] = None
            if snap["ps"].get("m_iVehicleNum", 0):
                snap["vps"] = read_ps(r, T, old["vps"] if old and old["vps"] else None, True, rec_vps)
            ps_bits = r.bit - p0
            # packet entities
            oldents = old["ents"] if old else []
            oi = 0
            oldnum = 99999 if oi >= len(oldents) else oldents[oi][0]
            newents = []
            while True:
                pe = r.bit
                r.tag = ("e", "#num")
                newnum = r.read(GENTITYNUM_BITS)
                if newnum == MAX_GENTITIES - 1:
                    break
                while oldnum < newnum:
                    newents.append(oldents[oi]); oi += 1
                    oldnum = 99999 if oi >= len(oldents) else oldents[oi][0]
                if oldnum == newnum:
                    rec = [("#entnum", GENTITYNUM_BITS, "num", newnum, r.bit - pe)]
                    ns = read_entity(r, T, oldents[oi][1], newnum, rec)
                    rec_ent.append((newnum, "delta", rec))
                    if ns is not None: newents.append((newnum, ns))
                    oi += 1
                    oldnum = 99999 if oi >= len(oldents) else oldents[oi][0]
                    continue
                if oldnum > newnum:
                    rec = [("#entnum", GENTITYNUM_BITS, "num", newnum, r.bit - pe)]
                    ns = read_entity(r, T, cl.baselines.get(newnum, {}), newnum, rec)
                    rec_ent.append((newnum, "base", rec))
                    if ns is not None: newents.append((newnum, ns))
                    continue
            while oldnum != 99999:
                newents.append(oldents[oi]); oi += 1
                oldnum = 99999 if oi >= len(oldents) else oldents[oi][0]
            snap["ents"] = newents
            cl.snaps[seq] = snap
            for k in [k for k in cl.snaps if k < seq - 40]:
                del cl.snaps[k]
            out["ops"].append(("snapshot", r.bit - p0, dict(
                serverTime=st, delta=delta, flags=flags, areabytes=nareas, ps_bits=ps_bits,
                rec_ps=rec_ps, rec_vps=rec_vps, rec_ent=rec_ent, nents=len(newents),
                hdr_end=hdr_end, p0=snap_p0, snap=snap)))
        else:
            out["ops"].append(("BAD", r.bit - p0, cmd))
            out["end"] = "ERR"; return out
    out["endbit"] = r.bit
    return out

# ---------------------------------------------------------------- file
def read_capture(fn):
    d = open(fn, "rb").read(); p = 0
    seg = None
    while p < len(d):
        tag = d[p:p + 4]
        if tag == b"SEG1":
            g, f, n = struct.unpack_from("<iiI", d, p + 4)
            seg = dict(gametype=g, fps=f, map=d[p + 16:p + 16 + n].decode()); p += 16 + n
        elif tag == b"MSG1":
            ts, seq, sid, ln = struct.unpack_from("<iiiI", d, p + 4)
            cn = d[p + 20]
            yield seg, dict(time=ts, seq=seq, sid=sid, client=cn), d[p + 24:p + 24 + ln]
            p += 24 + ln
        else:
            raise ValueError("bad tag at %d" % p)

def setup():
    src = open(SRC, encoding="latin-1").read()
    global MAX_WEAPONS
    # MAX_WEAPONS / STAT_WEAPONS come from other headers
    T = load_tables(src)
    hd = load_hdata(src)
    cache = os.path.join(os.path.dirname(os.path.abspath(__file__)), "hufftree.pkl")
    dec = build_decode_table(hd, cache)
    return T, dec

def decode_file(fn, T, dec):
    cl = Client(); res = []; cur_sid = None
    for seg, hdr, data in read_capture(fn):
        if hdr["sid"] != cur_sid:
            cl = Client(); cur_sid = hdr["sid"]
        try:
            m = parse_message(data, hdr, T, dec, cl, None)
        except Exception as e:
            m = {"seq": hdr["seq"], "time": hdr["time"], "len": len(data), "ops": [], "end": "EXC " + repr(e)}
        m["seg"] = seg
        res.append(m)
    return res

if __name__ == "__main__":
    T, dec = setup()
    for fn in sys.argv[1:]:
        res = decode_file(fn, T, dec)
        c = collections.Counter(m["end"] if not m["end"].startswith("EXC") else m["end"][:60] for m in res)
        print(fn, len(res), dict(c))
        pickle.dump(res, open(fn + ".dec.pkl", "wb"))
