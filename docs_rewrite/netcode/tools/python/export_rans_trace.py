"""Step 6: rANS golden trace for the C++ tests and benchmark (codemp/netcodec).
Encodes the real messages of the three captures with the whole-message codec (tables trained as run_full.py, initial
state X0 = 1) and writes, per message, the rANS units (context table, symbol or raw bits) and the expected bytes.
Output (local, not committed): ../../data/traces/rans_trace.bin. Run: python export_rans_trace.py

Format, little-endian:
  "NCRT" u32 version=1 u32 prob_bits u32 ntables, then per table: u32 K, u16 freq[K]
  u32 nmsg, then per message: u32 nunits, per unit: u32 table (0xFFFFFFFF = raw bits), u32 value (symbol or raw value),
  u32 nbits (raw only, else 0); then u32 nbytes, the bytes."""
import sys, os, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codec as C, run_full as RF

OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "data", "traces", "rans_trace.bin"))

def main():
    C.X0 = 1
    modes, tables = RF.train()
    tid = {}; tabs = []; msgs = []
    for key in ("5min", "bots", "solo"):
        for m, units, _ in RF.encode_stream(RF.RAW[key], modes):
            out = []
            for ctx, K, sym in units:
                if ctx is None:
                    out.append((0xFFFFFFFF, sym, K))
                    continue
                k = (ctx, K)
                if k not in tid:
                    tid[k] = len(tabs); tabs.append(tables.table(ctx, K)[0])
                out.append((tid[k], sym, 0))
            msgs.append((out, C.rans_encode(units, tables)))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "wb") as f:
        f.write(b"NCRT" + struct.pack("<III", 1, C.PB, len(tabs)))
        for fr in tabs:
            assert sum(fr) == 1 << C.PB and min(fr) >= 1
            f.write(struct.pack("<I%dH" % len(fr), len(fr), *fr))
        f.write(struct.pack("<I", len(msgs)))
        for units, buf in msgs:
            f.write(struct.pack("<I", len(units)))
            f.write(b"".join(struct.pack("<III", *u) for u in units))
            f.write(struct.pack("<I", len(buf)) + buf)
    nu = sum(len(u) for u, b in msgs); nr = sum(1 for u, b in msgs for x in u if x[0] == 0xFFFFFFFF)
    print("wrote %s: %d tables, %d messages, %d units (%d raw), %d bytes" % (OUT, len(tabs), len(msgs), nu, nr, sum(len(b) for u, b in msgs)))

if __name__ == "__main__":
    main()
