# samples by category, from the whole inline chain: the first matching rule (innermost to outermost frames)
import sys, subprocess, collections, re
samples_path, exe = sys.argv[1], sys.argv[2]
PREF = 0x140000000
rows = [int(l.split()[1], 16) for l in open(samples_path)]
uniq = sorted(set(rows))
out = subprocess.run(["llvm-symbolizer", "--obj=" + exe, "--functions=short", "--demangle", "--inlining"],
                     input="\n".join(hex(x) for x in uniq) + "\n", capture_output=True, text=True).stdout
blocks = out.strip("\n").split("\n\n")
frames = {}
for a, blk in zip(uniq, blocks):
    ls = blk.split("\n")
    frames[a] = [re.sub(r"\(.*", "", ls[i]).strip() for i in range(0, len(ls) - 1, 2)]
rules = [("SameEntity", "SameEntity"), ("Changes (field compare)", "Changes"), ("Finish (rANS)", "Finish"),
         ("PutCommand", "PutCommand"), ("PutPlayer", "PutPlayer"), ("PutEntity (fields)", "PutEntity"),
         ("EntityCursor/older", "EntityCursor"), ("PutEntities loop", "PutEntities"), ("PutMessage other", "PutMessage"),
         ("Encode other", "Encode")]
cnt = collections.Counter()
for a in rows:
    fr = frames.get(a, ["?"])
    cat = "other: " + fr[0]
    for name, key in rules:
        if any(key in f for f in fr):
            cat = name
            break
    cnt[cat] += 1
n = len(rows)
for k, v in cnt.most_common(18):
    print(f"{100 * v / n:5.1f}%  {k}")
