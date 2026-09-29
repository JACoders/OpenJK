#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="$(cd "$HERE/.." && pwd)/run_e2e.sh"
ROOT="$(mktemp -d /private/tmp/openjk-e2e-selftest.XXXXXX)"

worker="python3 '$HERE/fake_worker.py'"
declare -a pids=()
for n in 1 2 3 4; do
	"$RUNNER" --case "concurrent$n" --players "$n" --timeout 5 \
		--artifact-root "$ROOT" --hash "$HERE/fake_worker.py" \
		--client-command "$worker success" >"$ROOT/concurrent$n.out" &
	pids+=("$!")
done
for pid in "${pids[@]}"; do wait "$pid"; done

mapfile -t manifests < <(find "$ROOT" -name manifest.tsv | sort)
[[ "${#manifests[@]}" -eq 4 ]]
[[ "$(awk -F '\t' '$1 ~ /^port\./ {print $2}' "${manifests[@]}" | sort -u | wc -l | tr -d ' ')" -eq 16 ]]
[[ "$(awk -F '\t' '$1 == "homepath" {print $2}' "${manifests[@]}" | sort -u | wc -l | tr -d ' ')" -eq 4 ]]
for manifest in "${manifests[@]}"; do "$RUNNER" --validate-only "$manifest"; done
lock_root="${TMPDIR:-/private/tmp}/openjk-e2e-port-locks"
for manifest in "${manifests[@]}"; do
	while IFS= read -r port; do
		[[ ! -d "$lock_root/$port" ]] || {
			echo "port lock leaked: $lock_root/$port" >&2
			exit 1
		}
	done < <(awk -F '\t' '$1 ~ /^port\./ {print $2}' "$manifest")
done

if "$RUNNER" --case failure --timeout 5 --artifact-root "$ROOT" \
	--client-command "$worker failure" >"$ROOT/failure.out"; then
	echo "failure case unexpectedly passed" >&2
	exit 1
fi
failure_manifest="$(tail -n 1 "$ROOT/failure.out")"
[[ "$(awk -F '\t' '$1 == "status" {print $2}' "$failure_manifest")" == failed ]]
[[ -s "$(awk -F '\t' '$1 == "process.client.log" {print $2}' "$failure_manifest")" ]]
if "$RUNNER" --validate-only "$failure_manifest" 2>/dev/null; then
	echo "failed manifest unexpectedly validated as a pass" >&2
	exit 1
fi
if "$RUNNER" --validate-only "$failure_manifest" --allow-failed-validation 2>/dev/null; then
	echo "failed evidence inspection unexpectedly returned success" >&2
	exit 1
fi

if "$RUNNER" --case assertion-failure --timeout 5 --artifact-root "$ROOT" \
	--client-command "$worker assertion_failure" >"$ROOT/assertion-failure.out"; then
	echo "zero-exit assertion failure unexpectedly passed" >&2
	exit 1
fi
assertion_manifest="$(tail -n 1 "$ROOT/assertion-failure.out")"
[[ "$(awk -F '\t' '$1 == "status" {print $2}' "$assertion_manifest")" == failed ]]
[[ "$(awk -F '\t' '$1 == "error" {print $2}' "$assertion_manifest")" == \
	"authoritative client log contains an assertion, fatal engine, or sanitizer failure" ]]
rg -q 'SplitInputAssertCmd: FAIL' \
	"$(awk -F '\t' '$1 == "process.client.log" {print $2}' "$assertion_manifest")"

bad_manifest="$ROOT/bad-format.tsv"
printf 'format\twrong\nstatus\tpassed\n' >"$bad_manifest"
if "$RUNNER" --validate-only "$bad_manifest" 2>/dev/null; then
	echo "wrong manifest format unexpectedly passed" >&2
	exit 1
fi

if "$RUNNER" --case timeout --timeout 1 --artifact-root "$ROOT" \
	--client-command "$worker listen --port "'$OPENJK_E2E_CLIENT_PORT' >"$ROOT/timeout.out"; then
	echo "timeout case unexpectedly passed" >&2
	exit 1
fi
timeout_manifest="$(tail -n 1 "$ROOT/timeout.out")"
[[ "$(awk -F '\t' '$1 == "status" {print $2}' "$timeout_manifest")" == timeout ]]
timeout_pid="$(awk -F '\t' '$1 == "process.client.pid" {print $2}' "$timeout_manifest")"
if kill -0 "$timeout_pid" 2>/dev/null; then
	echo "timeout left child $timeout_pid alive" >&2
	exit 1
fi
timeout_port="$(awk -F '\t' '$1 == "port.client" {print $2}' "$timeout_manifest")"
python3 - "$timeout_port" <<'PY'
import socket, sys
s = socket.socket()
try:
    s.bind(("127.0.0.1", int(sys.argv[1])))
except PermissionError:
    # Restricted CI already proved teardown via the recorded PID and lock.
    pass
s.close()
PY

echo "GP0-02 self-test: PASS"
echo "artifacts: $ROOT"
