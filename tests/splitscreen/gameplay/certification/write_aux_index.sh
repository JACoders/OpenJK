#!/usr/bin/env bash
set -euo pipefail

kind="${1:?kind required}"
players="${2:?players required}"
run_id="${3:?run id required}"
frozen_sha="${4:?frozen sha required}"
manifest="${5:?manifest required}"
report="${6:?report required}"
index="${7:?index required}"

sha256() { shasum -a 256 "$1" | awk '{print $1}'; }
test -s "$manifest"
test -s "$report"
mkdir -p "$(dirname "$index")"
{
	printf 'format\topenjk-cert-aux-v1\n'
	printf 'run_id\t%s\n' "$run_id"
	printf 'kind\t%s\n' "$kind"
	printf 'players\t%s\n' "$players"
	printf 'frozen_sha256\t%s\n' "$frozen_sha"
	printf 'manifest\t%s\t%s\n' "$manifest" "$(sha256 "$manifest")"
	printf 'report\t%s\t%s\n' "$report" "$(sha256 "$report")"
	printf 'artifact\t%s\t%s\n' "$manifest" "$(sha256 "$manifest")"
	printf 'artifact\t%s\t%s\n' "$report" "$(sha256 "$report")"
	while IFS=$'\t' read -r key artifact recorded_sha; do
		[[ "$key" == artifact && -n "$artifact" && -n "$recorded_sha" ]] || continue
		test -s "$artifact"
		test "$(sha256 "$artifact")" = "$recorded_sha"
		printf 'artifact\t%s\t%s\n' "$artifact" "$recorded_sha"
	done <"$manifest"
} >"$index"
