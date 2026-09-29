#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/certification/public"
DEFAULT_TARGET="135.125.145.49:29070"
TARGET="${GP5_PUBLIC_TARGET:-$DEFAULT_TARGET}"
QUERY_URL="https://jknexus.se/api/serverlist"
LOCK="/tmp/openjk-public-qa.lock"
FROZEN_CLIENT_SHA256="737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
RUNNER="${GP5_PUBLIC_ACCEPTANCE_RUNNER:-$ROOT/tests/splitscreen/run_routed_public_acceptance.sh}"
OFFLINE_CONTRACT="${GP5_PUBLIC_OFFLINE_CONTRACT:-0}"
EXPECTED_SHA256="${GP5_EXPECTED_SHA256:-$FROZEN_CLIENT_SHA256}"
FROZEN_VERIFIER="${GP5_PUBLIC_FROZEN_VERIFIER:-$ROOT/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py}"
if [[ "$OFFLINE_CONTRACT" == 1 ]]; then
	CASE_COOLDOWN_SECONDS="${GP5_PUBLIC_CASE_COOLDOWN_SECONDS:-0}"
else
	CASE_COOLDOWN_SECONDS="${GP5_PUBLIC_CASE_COOLDOWN_SECONDS:-12}"
fi

[[ "${GP5_ALLOW_PUBLIC:-0}" == 1 ]] || {
	echo "public authorization gate closed; set GP5_ALLOW_PUBLIC=1" >&2
	exit 77
}
[[ "$TARGET" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}:[0-9]{1,5}$ ]] || {
	echo "public target must be a numeric IPv4 endpoint: $TARGET" >&2
	exit 2
}
[[ "$CASE_COOLDOWN_SECONDS" =~ ^[0-9]+$ ]] &&
	(( CASE_COOLDOWN_SECONDS <= 60 )) || {
	echo "public case cooldown must be an integer from 0 through 60 seconds" >&2
	exit 2
}
[[ -x "$RUNNER" ]] || {
	echo "public acceptance runner is not executable: $RUNNER" >&2
	exit 2
}
if [[ "$OFFLINE_CONTRACT" != 1 && "$EXPECTED_SHA256" != "$FROZEN_CLIENT_SHA256" ]]; then
	echo "public certification requires frozen client $FROZEN_CLIENT_SHA256" >&2
	exit 2
fi
if [[ "$OFFLINE_CONTRACT" != 1 && "$RUNNER" != "$ROOT/tests/splitscreen/run_routed_public_acceptance.sh" ]]; then
	echo "a replacement runner is allowed only for offline contract tests" >&2
	exit 2
fi
if [[ "$OFFLINE_CONTRACT" != 1 && "$FROZEN_VERIFIER" != "$ROOT/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" ]]; then
	echo "a replacement frozen-artifact verifier is allowed only for offline contract tests" >&2
	exit 2
fi
if [[ -n "${GP5_PUBLIC_QUERY_FIXTURE:-}" && "$OFFLINE_CONTRACT" != 1 ]]; then
	echo "a captured query fixture is allowed only for offline contract tests" >&2
	exit 2
fi
for players in 2 3 4; do
	cfg="$ROOT/tests/splitscreen/cfg/routed_public_${players}p_acceptance.cfg"
	[[ "$(awk '$1 == "addFavorite" { print $2; exit }' "$cfg")" == "$DEFAULT_TARGET" ]] || {
		echo "public ${players}p acceptance source target is not pinned to $DEFAULT_TARGET" >&2
		exit 2
	}
done

run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_root="${GP5_PUBLIC_EVIDENCE_ROOT:-$ROOT/tests/splitscreen/gameplay/results/certification/public/runs/$run_id}"
mkdir -p "$run_root"
frozen_log="$run_root/frozen-artifacts.log"
if ! python3 "$FROZEN_VERIFIER" \
		--expected-client-hash "$EXPECTED_SHA256" >"$frozen_log" 2>&1; then
	cat "$frozen_log" >&2
	echo "frozen artifact gate failed before public lock/query" >&2
	exit 2
fi

mkdir "$LOCK" 2>/dev/null || {
	echo "public QA lock busy: $LOCK" >&2
	exit 75
}
trap 'rmdir "$LOCK"' EXIT

summary="$run_root/summary.tsv"
printf 'players\tclassification\tquery_completed_utc\trunner_started_utc\ttarget\tevidence\n' >"$summary"

overall=0
for players in 2 3 4; do
	# The stock server reconnect guard retains a retired client's source
	# endpoint briefly. Separate client processes can be assigned the same
	# ephemeral split-client port, so leave that retired endpoint outside the
	# guard before starting the next player-count case.
	if [[ "$players" -gt 2 && "$CASE_COOLDOWN_SECONDS" -gt 0 ]]; then
		sleep "$CASE_COOLDOWN_SECONDS"
	fi
	case_root="$run_root/${players}p"
	mkdir -p "$case_root"
	raw="$case_root/serverlist.raw.json"
	query_log="$case_root/serverlist.query.log"
	gate="$case_root/capacity.json"
	runner_log="$case_root/runner.stdout.log"
	result="$case_root/result.txt"

	# Tests inject a captured response. Production always performs a new live
	# request in this exact location, immediately before invoking the join runner.
	if [[ -n "${GP5_PUBLIC_QUERY_FIXTURE:-}" ]]; then
		cp "$GP5_PUBLIC_QUERY_FIXTURE" "$raw" 2>"$query_log"
	else
		if ! curl --fail --silent --show-error --location \
			--connect-timeout 10 --max-time 20 \
			"$QUERY_URL" >"$raw" 2>"$query_log"; then
			query_completed="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
			classification=BLOCKED_QUERY_FAILED
			printf '%s\n' "$classification" >"$result"
			printf '%s\t%s\t%s\t-\t%s\t%s\n' \
				"$players" "$classification" "$query_completed" "$TARGET" "$case_root" >>"$summary"
			[[ "$overall" -ne 0 ]] || overall=78
			continue
		fi
	fi
	query_completed="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
	if python3 "$HERE/query_server.py" "$raw" "$players" \
		--target "$TARGET" --output "$gate"; then
		classification=READY
	else
		classification="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["classification"])' "$gate")"
		printf '%s\n' "$classification" >"$result"
		printf '%s\t%s\t%s\t-\t%s\t%s\n' \
			"$players" "$classification" "$query_completed" "$TARGET" "$case_root" >>"$summary"
		[[ "$overall" -ne 0 ]] || overall=78
		continue
	fi

	runner_started="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
	set +e
	OPENJK_PUBLIC_QA_ROOT="$case_root/acceptance" \
	OPENJK_EXPECTED_SHA256="$EXPECTED_SHA256" \
	OPENJK_PUBLIC_TARGET="$TARGET" \
		"$RUNNER" "$players" >"$runner_log" 2>&1
	runner_status=$?
	set -e

	if [[ "$runner_status" -eq 0 ]]; then
		# A READY gate is contemporaneous evidence that at least one human not
		# belonging to this local party occupied the exact server before Join.
		classification=PASS_WITH_NONLOCAL_HUMAN
	elif [[ "$players" -eq 4 ]] && rg -qi \
		'Too Many players with the same IP|Too many connections from the same IP|@@@TOO_MANY_INFO' \
		"$runner_log" "$case_root/acceptance"; then
		classification=BLOCKED_SERVER_SAME_IP_LIMIT
		[[ "$overall" -ne 0 ]] || overall=78
	else
		classification=FAIL_ACCEPTANCE
		overall=1
	fi
	printf '%s\n' "$classification" >"$result"
	printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
		"$players" "$classification" "$query_completed" "$runner_started" "$TARGET" "$case_root" >>"$summary"
done

(cd "$run_root" && {
	find . -type f ! -name SHA256SUMS | LC_ALL=C sort |
		while IFS= read -r evidence; do shasum -a 256 "$evidence"; done
}) >"$run_root/SHA256SUMS"
echo "public certification evidence: $run_root"
echo "summary: $summary"
exit "$overall"
