# Summary of the A/B scripts (ab_gcc.sh, ab_gcc_podman.sh, ab_clang.sh). Input lines:
#   <build> <round>: warm encode <ns> (legacy <ns>) decode <ns> (legacy <ns>) mismatches <n> | cold encode <ns> decode <ns>
# A warm time of 0 means the build's benchmark predates the warm timing (an old reference commit).
# Prints the best warm and cold times of each build and their difference against "ref", then checks:
#  - netcodec mismatches (the builds must code the same bytes);
#  - the unchanged legacy encoder moving by more than 4 % between the runs of one build: a busy machine, the numbers
#    must not be used;
#  - the legacy encoder differing between builds: its code is the same, so that difference is code placement, and a
#    netcodec difference of the same size means nothing.
function pct(x, r) { return r > 0 ? sprintf("%+.1f %%", 100 * (x / r - 1)) : "n/a" }
{
	b = $1
	e = $5 + 0; le = $7; d = $9 + 0; mm = $13; ce = $17 + 0; cd = $19 + 0
	gsub(/[()]/, "", le)
	le += 0
	if (!(b in be)) { order[++nb] = b; be[b] = e; bd[b] = d; bce[b] = ce; bcd[b] = cd; lmin[b] = le; lmax[b] = le }
	if (e < be[b]) be[b] = e
	if (d < bd[b]) bd[b] = d
	if (ce < bce[b]) bce[b] = ce
	if (cd < bcd[b]) bcd[b] = cd
	if (le < lmin[b]) lmin[b] = le
	if (le > lmax[b]) lmax[b] = le
	bad += mm
}
END {
	busy = 0
	nowarm = 0
	for (k = 1; k <= nb; k++) {
		b = order[k]
		line = sprintf("best %-5s warm encode %8.1f decode %8.1f | cold encode %6.0f decode %6.0f | legacy warm encode %8.1f to %8.1f",
		               b, be[b], bd[b], bce[b], bcd[b], lmin[b], lmax[b])
		if (b != "ref" && ("ref" in be))
			line = line sprintf(" | against ref: warm encode %s, decode %s, cold encode %s, decode %s, legacy %s", pct(be[b], be["ref"]),
			                    pct(bd[b], bd["ref"]), pct(bce[b], bce["ref"]), pct(bcd[b], bcd["ref"]), pct(lmin[b], lmin["ref"]))
		print line
		if (lmax[b] > lmin[b] * 1.04) busy = 1
		if (be[b] == 0) nowarm = 1
	}
	if (bad > 0) print "WARNING: netcodec mismatches: the builds do not code the same bytes"
	if (busy) print "WARNING: the unchanged legacy encoder moved by more than 4 % within a build: the machine was busy, run again when it is quiet"
	if (nowarm) print "NOTE: a build has no warm timing (its benchmark predates it): only the cold timings compare, and they vary by up to 9 % between runs"
	print "(the legacy difference between builds is code placement: a netcodec difference of that size is not a result)"
}
