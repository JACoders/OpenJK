# Summary of the A/B scripts (ab_gcc.sh, ab_gcc_podman.sh, ab_clang.sh). Input lines:
#   <build> <round>: warm encode <ns> (legacy <ns>) decode <ns> (legacy <ns>) mismatches <n>
# Prints the best warm times of each build and their difference against "ref", then checks:
#  - netcodec mismatches (the builds must code the same bytes);
#  - the unchanged legacy encoder moving by more than 4 % between the runs of one build: a busy machine, the numbers
#    must not be used;
#  - the legacy encoder differing between builds: its code is the same, so that difference is code placement, and a
#    netcodec difference of the same size means nothing.
{
	b = $1
	e = $5; le = $7; d = $9; mm = $13
	gsub(/[()]/, "", le)
	if (!(b in be)) { order[++nb] = b; be[b] = e; bd[b] = d; lmin[b] = le; lmax[b] = le }
	if (e + 0 < be[b] + 0) be[b] = e
	if (d + 0 < bd[b] + 0) bd[b] = d
	if (le + 0 < lmin[b] + 0) lmin[b] = le
	if (le + 0 > lmax[b] + 0) lmax[b] = le
	bad += mm
}
END {
	busy = 0
	for (k = 1; k <= nb; k++) {
		b = order[k]
		line = sprintf("best %-5s warm encode %8.1f decode %8.1f | legacy encode %8.1f to %8.1f", b, be[b], bd[b], lmin[b], lmax[b])
		if (b != "ref" && ("ref" in be))
			line = line sprintf(" | against ref: encode %+.1f %%, decode %+.1f %%, legacy %+.1f %%", 100 * (be[b] / be["ref"] - 1),
			                    100 * (bd[b] / bd["ref"] - 1), 100 * (lmin[b] / lmin["ref"] - 1))
		print line
		if (lmax[b] > lmin[b] * 1.04) busy = 1
	}
	if (bad > 0) print "WARNING: netcodec mismatches: the builds do not code the same bytes"
	if (busy) print "WARNING: the unchanged legacy encoder moved by more than 4 % within a build: the machine was busy, run again when it is quiet"
	print "(the legacy difference between builds is code placement: a netcodec difference of that size is not a result)"
}
