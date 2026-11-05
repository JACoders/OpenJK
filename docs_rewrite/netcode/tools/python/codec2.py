"""Whole-message codec: every svc op (nop, gamestate, server command, snapshot, setgame, download, mapchange),
pilot / vehicle playerstates, built on the snapshot codec of codec.py. Static rANS, one stream per message."""
import math, struct, collections, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bench2 as B
import codec as C
from codec import (EncIO, DecIO, Tables, rans_encode, cls_put, cls_get, put_field, get_field, ents_put, ents_get,
                   zz, unzz, ENAMES)

EF_NODRAW = 1 << 8
ARR = C.PF2[len(B.T["ps"]):]
PS_TABLES = {0: list(B.T["ps"]) + ARR,
             1: list(B.T["pilot"][:len(B.T["pilot"]) - 82]) + ARR,      # the engine only sends the first (size - 82) fields
             2: list(B.T["veh"]) + ARR}
OPS = {"nop": 1, "gs": 2, "cmd": 3, "snap": 4, "setgame": 5, "dl": 6, "mapchange": 7}
OPNAME = {v: k for k, v in OPS.items()}

def s32(v): return v - (1 << 32) if v >= (1 << 31) else v

class Ref:
    """the base snapshot a message was coded against, kept in its header record so that the next message can use it
    as 'older' (second-order prediction); it never takes part in comparisons of header records"""
    __slots__ = ("v",)
    def __init__(self, v): self.v = v
    def __eq__(self, other): return isinstance(other, Ref)
    def __ne__(self, other): return not self.__eq__(other)

def _older_of(bh):
    r = bh.get("base") if bh else None
    return r.v if r is not None else None

def enable_prediction():
    move = C.enable_prediction()
    for t in PS_TABLES.values():
        move(t, {"velocity[0]", "velocity[1]", "velocity[2]"}, {"origin[0]", "origin[1]", "origin[2]"})

# ------------------------------------------------------------------ strings
def str_put(io, kind, b):
    for x in b: io.s(("str", kind), 256, x)
    io.s(("str", kind), 256, 0)

def str_get(io, kind):
    out = bytearray()
    while True:
        x = io.s(("str", kind), 256)
        if x == 0: return bytes(out)
        out.append(x)

# ------------------------------------------------------------------ playerstates
def _older_ps(tab):
    o = C.CTX.get("older_snap")
    if o is None: return None
    if tab == 2:   # the client holds a vehicle state only for snapshots whose player state had a vehicle
        return (o.get("vps") or {}) if o["ps"].get("m_iVehicleNum", 0) else {}
    return o["ps"]

def ps_put2(io, new, old, modes, tab):
    fields = PS_TABLES[tab]; n = len(fields); prev = -1
    C.ps_ctx(new, old, _older_ps(tab))
    for i in range(n):
        nm, bits = fields[i]
        a = new.get(nm, 0); b = old.get(nm, 0)
        if a == b: continue
        io.s(("pg", tab, prev), n + 1, i); prev = i
        put_field(io, nm, bits, a, b, modes, "p:")
    io.s(("pg", tab, prev), n + 1, n)

def ps_get2(io, old, modes, tab):
    fields = PS_TABLES[tab]; n = len(fields); s = dict(old); prev = -1
    C.ps_ctx(s, old, _older_ps(tab))
    while True:
        i = io.s(("pg", tab, prev), n + 1)
        if i == n: return s
        assert i > prev
        nm, bits = fields[i]
        s[nm] = get_field(io, nm, bits, old.get(nm, 0), modes, "p:"); prev = i

def is_pilot(ps): return 1 if (ps.get("m_iVehicleNum", 0) and (ps.get("eFlags", 0) & EF_NODRAW)) else 0

# ------------------------------------------------------------------ message
def msg_put(io, msg, base, bh, baselines, modes, delta):
    """base: snapshot state the client holds (or None); bh: its header dict (ack, time, cmdseq) or None;
    delta: sequence distance to that base (0 = none). The decoder needs delta first to find the base."""
    io.s(("m", "delta"), 256, delta)
    C.CTX.clear(); C.CTX.update(time=None, btime=bh["time"] if bh else None, older_snap=_older_of(bh))
    cls_put(io, ("m", "ack"), 36, zz(msg["ack"] - (bh["ack"] if bh else 0)))
    lastcmd = bh["cmdseq"] if bh else 0
    cmdseq = lastcmd; time = None; prev = 0
    for op in msg["ops"]:
        t = OPS[op["t"]]; io.s(("op", prev), 8, t); prev = t
        if op["t"] == "cmd":
            cls_put(io, ("m", "cseq"), 36, zz(op["seq"] - (lastcmd + 1))); lastcmd = op["seq"]; cmdseq = lastcmd
            str_put(io, "cmd", op["text"])
        elif op["t"] == "setgame":
            str_put(io, "sg", op["text"])
        elif op["t"] == "gs":
            cls_put(io, ("m", "rseq"), 36, zz(op["rseq"]))
            cls_put(io, ("m", "ncs"), 16, len(op["cs"]))
            pi = -1
            for idx, txt in op["cs"]:
                cls_put(io, ("m", "csgap"), 16, idx - pi - 1); pi = idx
                str_put(io, "cs", txt)
            ents_put(io, op["baselines"], [], {}, modes)
            cls_put(io, ("m", "cnum"), 36, zz(op["clientNum"]))
            io.r(32, op["checksum"] & 0xFFFFFFFF)
            baselines = dict(op["baselines"])
        elif op["t"] == "dl":
            cls_put(io, ("m", "dlblk"), 20, op["block"])
            if op["block"] == 0: cls_put(io, ("m", "dlsize"), 36, zz(op["fsize"]))
            cls_put(io, ("m", "dllen"), 20, len(op["data"]))
            for x in op["data"]: io.s(("dl",), 256, x)
        elif op["t"] == "snap":
            time = op["time"]; C.CTX["time"] = time
            cls_put(io, ("h", "time"), 36, zz(op["time"] - (bh["time"] if bh else 0)))
            io.s(("h", "flags"), 256, op["flags"])
            io.s(("h", "area"), 256, len(op["areas"]))
            for a in op["areas"]: io.s(("h", "area"), 256, a)
            sn = op["snap"]
            pil = is_pilot(sn["ps"])
            io.s(("ps", "pilot"), 2, pil)
            ps_put2(io, sn["ps"], base["ps"] if base else {}, modes, 1 if pil else 0)
            if sn["ps"].get("m_iVehicleNum", 0):
                ps_put2(io, sn["vps"] or {}, (base["vps"] or {}) if base else {}, modes, 2)
            ents_put(io, sn["ents"], base["ents"] if base else [], baselines, modes)
    io.s(("op", prev), 8, 0)
    C.CTX["time"] = None
    return dict(ack=msg["ack"], time=time, cmdseq=cmdseq, base=Ref(base))

def msg_get(io, lookup, baselines, modes):
    delta = io.s(("m", "delta"), 256)
    base, bh = lookup(delta)
    C.CTX.clear(); C.CTX.update(time=None, btime=bh["time"] if bh else None, older_snap=_older_of(bh))
    msg = dict(ack=(bh["ack"] if bh else 0) + unzz(cls_get(io, ("m", "ack"), 36)), ops=[])
    lastcmd = bh["cmdseq"] if bh else 0
    cmdseq = lastcmd; snap = None; time = None; prev = 0
    while True:
        t = io.s(("op", prev), 8); prev = t
        if t == 0: break
        name = OPNAME[t]; op = dict(t=name)
        if name == "cmd":
            op["seq"] = lastcmd + 1 + unzz(cls_get(io, ("m", "cseq"), 36)); lastcmd = op["seq"]; cmdseq = lastcmd
            op["text"] = str_get(io, "cmd")
        elif name == "setgame":
            op["text"] = str_get(io, "sg")
        elif name == "gs":
            op["rseq"] = unzz(cls_get(io, ("m", "rseq"), 36))
            n = cls_get(io, ("m", "ncs"), 16); pi = -1; cs = []
            for _ in range(n):
                idx = pi + 1 + cls_get(io, ("m", "csgap"), 16); pi = idx
                cs.append((idx, str_get(io, "cs")))
            op["cs"] = cs
            op["baselines"] = ents_get(io, [], {}, modes)
            op["clientNum"] = unzz(cls_get(io, ("m", "cnum"), 36))
            op["checksum"] = s32(io.r(32))
            baselines = dict(op["baselines"])
        elif name == "dl":
            op["block"] = cls_get(io, ("m", "dlblk"), 20)
            op["fsize"] = unzz(cls_get(io, ("m", "dlsize"), 36)) if op["block"] == 0 else None
            n = cls_get(io, ("m", "dllen"), 20)
            op["data"] = bytes(io.s(("dl",), 256) for _ in range(n))
        elif name == "snap":
            op["time"] = (bh["time"] if bh else 0) + unzz(cls_get(io, ("h", "time"), 36)); time = op["time"]; C.CTX["time"] = time
            op["delta"] = delta; op["flags"] = io.s(("h", "flags"), 256)
            na = io.s(("h", "area"), 256); op["areas"] = [io.s(("h", "area"), 256) for _ in range(na)]
            pil = io.s(("ps", "pilot"), 2)
            ps = ps_get2(io, base["ps"] if base else {}, modes, 1 if pil else 0)
            vps = None
            if ps.get("m_iVehicleNum", 0):
                vps = ps_get2(io, (base["vps"] or {}) if base else {}, modes, 2)
            ents = ents_get(io, base["ents"] if base else [], baselines, modes)
            op["snap"] = dict(ps=ps, vps=vps, ents=ents); snap = op["snap"]
        msg["ops"].append(op)
    C.CTX["time"] = None
    return msg, dict(ack=msg["ack"], time=time, cmdseq=cmdseq, base=Ref(base)), snap, baselines

# ------------------------------------------------------------------ canonical forms for comparison
ALLNAMES = sorted({n for t in PS_TABLES.values() for n, b in t})
def canon_ps(ps, tab=0):
    return tuple(ps.get(n, 0) for n in ALLNAMES)

def canon_msg(msg, pil_aware=True):
    out = [msg["ack"]]
    for op in msg["ops"]:
        if op["t"] == "snap":
            sn = op["snap"]
            out.append(("snap", op["time"], op["delta"], op["flags"], tuple(op["areas"]), canon_ps(sn["ps"]),
                        canon_ps(sn["vps"]) if sn["vps"] is not None else None, C.canon_ents(sn["ents"])))
        elif op["t"] == "gs":
            out.append(("gs", op["rseq"], tuple(op["cs"]), C.canon_ents(op["baselines"]), op["clientNum"], op["checksum"] & 0xFFFFFFFF))
        elif op["t"] == "cmd": out.append(("cmd", op["seq"], op["text"]))
        elif op["t"] == "setgame": out.append(("sg", op["text"]))
        elif op["t"] == "dl": out.append(("dl", op["block"], op["fsize"], op["data"]))
        else: out.append(op["t"])
    return out
