"""Reference value of the C++ determinism test (codemp/netcodec/test/test_determinism.cpp): an FNV-1a 64 hash of the
float steps of the format, computed with the Python reference functions of codec.py. Same sweep, same order, same
byte encoding as the C++ test; the C++ constant must equal what this prints."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codec as C

M64 = (1 << 64) - 1
h = 0xCBF29CE484222325

def feed(v, nbytes):
    global h
    v &= (1 << (8 * nbytes)) - 1
    for _ in range(nbytes):
        h = ((h ^ (v & 0xFF)) * 0x100000001B3) & M64
        v >>= 8

def fkind(p): return 2 if p == 0x80000000 else C.fkind(p)

def pattern(p):
    p &= 0xFFFFFFFF
    feed(p, 4); feed(fkind(p), 4); feed(1 if C.is_a16(p) else 0, 4); feed(C.old_index(p), 8)

def main():
    for p in (0, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC00000, 0xFFC00001, 0x00000001, 0x80000001, 0x7F7FFFFF):
        pattern(p)
    for i in range(-131072, 131073):
        base = C.pack(i * C.A16)
        for d in (0, 1, -1, 2, -2):
            pattern(base + d)
    for k in range(-400000, 400001):          # decode of an angle index (A2: 2 * index - index + residual)
        feed(C.pack(k * C.A16), 4)
    print("0x%016X" % h)

main()
