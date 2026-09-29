#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
E2E_HARNESS_DIR="$SCRIPT_DIR/harness"
# shellcheck source=tests/splitscreen/gameplay/harness/lib.sh
source "$E2E_HARNESS_DIR/lib.sh"

usage() {
	cat <<'EOF'
usage: run_e2e.sh --case NAME [options]
  --players 1|2|3|4             local player count (default: 2)
  --vm native|qvm               VM mode (default: native)
  --client-command COMMAND      required launch command
  --server-command COMMAND      optional controlled server
  --remote-command COMMAND      optional remote fifth client
  --timeout SECONDS             bounded case timeout (default: 60)
  --artifact-root DIR           durable output parent
  --hash PATH                   binary/module to hash (repeatable)
  --validate-only MANIFEST      validate recorded artifacts without launching
  --allow-failed-validation     inspect a failed manifest (never treats it as pass)
EOF
}

case_name=""
players=2
vm_mode=native
client_command=""
server_command=""
remote_command=""
timeout_seconds=60
artifact_root="${OPENJK_E2E_ARTIFACT_ROOT:-$SCRIPT_DIR/artifacts}"
validate_manifest=""
allow_failed_validation=0
declare -a hash_paths=()

while (($#)); do
	case "$1" in
		--case) case_name="$2"; shift 2 ;;
		--players) players="$2"; shift 2 ;;
		--vm) vm_mode="$2"; shift 2 ;;
		--client-command) client_command="$2"; shift 2 ;;
		--server-command) server_command="$2"; shift 2 ;;
		--remote-command) remote_command="$2"; shift 2 ;;
		--timeout) timeout_seconds="$2"; shift 2 ;;
		--artifact-root) artifact_root="$2"; shift 2 ;;
		--hash) hash_paths+=("$2"); shift 2 ;;
		--validate-only) validate_manifest="$2"; shift 2 ;;
		--allow-failed-validation) allow_failed_validation=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) echo "unknown argument: $1" >&2; usage >&2; exit 2 ;;
	esac
done

if [[ -n "$validate_manifest" ]]; then
	e2e_validate_manifest "$validate_manifest" "$allow_failed_validation" || exit $?
	if (( allow_failed_validation )) && [[ "$(e2e_manifest_get "$validate_manifest" status)" != passed ]]; then
		echo "artifacts valid, but recorded run did not pass" >&2
		exit 3
	fi
	exit 0
fi
[[ "$case_name" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "invalid or missing --case" >&2; exit 2; }
[[ "$players" =~ ^[1-4]$ ]] || { echo "--players must be 1..4" >&2; exit 2; }
[[ "$vm_mode" == native || "$vm_mode" == qvm ]] || { echo "--vm must be native or qvm" >&2; exit 2; }
[[ "$timeout_seconds" =~ ^[1-9][0-9]*$ ]] || { echo "--timeout must be positive" >&2; exit 2; }
[[ -n "$client_command" ]] || { echo "--client-command is required" >&2; exit 2; }

run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM"
E2E_RUN_DIR="$artifact_root/$case_name/$run_id"
E2E_HOME="$E2E_RUN_DIR/home"
E2E_LOG_DIR="$E2E_RUN_DIR/logs"
E2E_SCREENSHOT_DIR="$E2E_RUN_DIR/screenshots"
E2E_MANIFEST="$E2E_RUN_DIR/manifest.tsv"
mkdir -p "$E2E_HOME" "$E2E_LOG_DIR" "$E2E_SCREENSHOT_DIR"
: >"$E2E_MANIFEST"
declare -a E2E_PIDS=() E2E_ROLES=() E2E_DONE_FILES=() E2E_PORT_LOCKS=()
trap e2e_cleanup EXIT INT TERM

e2e_manifest_set format openjk-e2e-v1
e2e_manifest_set status setup
e2e_manifest_set case "$case_name"
e2e_manifest_set run_id "$run_id"
e2e_manifest_set players "$players"
e2e_manifest_set vm "$vm_mode"
e2e_manifest_set homepath "$E2E_HOME"
e2e_manifest_set screenshots "$E2E_SCREENSHOT_DIR"
e2e_manifest_set started_utc "$(date -u +%Y-%m-%dT%H:%M:%SZ)"

for path in "${hash_paths[@]}"; do
	if [[ ! -f "$path" ]]; then
		e2e_manifest_set status setup_failed
		e2e_manifest_set error "missing hash input: $path"
		echo "$E2E_MANIFEST"
		exit 1
	fi
	printf 'hash\t%s\t%s\n' "$(cd "$(dirname "$path")" && pwd)/$(basename "$path")" "$(e2e_sha256 "$path")" >>"$E2E_MANIFEST"
done

export OPENJK_E2E_RUN_DIR="$E2E_RUN_DIR"
export OPENJK_E2E_HOMEPATH="$E2E_HOME"
export OPENJK_E2E_SCREENSHOTS="$E2E_SCREENSHOT_DIR"
export OPENJK_E2E_PLAYERS="$players"
export OPENJK_E2E_VM="$vm_mode"
e2e_allocate_port client || exit 1
export OPENJK_E2E_CLIENT_PORT="$E2E_ALLOCATED_PORT"
e2e_allocate_port server || exit 1
export OPENJK_E2E_SERVER_PORT="$E2E_ALLOCATED_PORT"
e2e_allocate_port remote || exit 1
export OPENJK_E2E_REMOTE_PORT="$E2E_ALLOCATED_PORT"
e2e_allocate_port qport || exit 1
export OPENJK_E2E_QPORT="$E2E_ALLOCATED_PORT"

e2e_manifest_set status launching
[[ -z "$server_command" ]] || e2e_launch server "$server_command"
[[ -z "$remote_command" ]] || e2e_launch remote "$remote_command"
e2e_launch client "$client_command"
e2e_manifest_set status running

deadline=$((SECONDS + timeout_seconds))
result=0
timed_out=0
while :; do
	client_index=$((${#E2E_PIDS[@]} - 1))
	[[ -s "${E2E_DONE_FILES[$client_index]}" ]] && break
	if (( SECONDS >= deadline )); then
		timed_out=1
		result=124
		break
	fi
	sleep 0.1
done

if (( timed_out )); then
	e2e_manifest_set status timeout
	e2e_manifest_set error "deadline exceeded after ${timeout_seconds}s"
	e2e_cleanup
else
	# The client is the case authority. Controlled servers and remote peers are
	# support processes and are torn down once the client completes.
	for i in "${!E2E_PIDS[@]}"; do
		[[ "$i" -eq "$client_index" ]] && continue
		e2e_stop_group "${E2E_PIDS[$i]}" "${E2E_DONE_FILES[$i]}"
	done
	for i in "${!E2E_PIDS[@]}"; do
		pid="${E2E_PIDS[$i]}"
		if wait "$pid"; then code=0; else code=$?; fi
		e2e_manifest_set "process.${E2E_ROLES[$i]}.exit" "$code"
		# Support processes are intentionally terminated once the authoritative
		# client completes. Preserve their observed exit code as evidence, but
		# do not turn that expected teardown signal into a failed client case.
		if [[ "$i" -eq "$client_index" ]] && (( code != 0 )); then
			result="$code"
		fi
	done
	client_log="$E2E_LOG_DIR/client.log"
	if (( result == 0 )) && [[ -s "$client_log" ]] && rg -q \
		-e 'Split[A-Za-z0-9_]*Assert[A-Za-z0-9_]*: FAIL' \
		-e '(^|[[:space:]])Sys_Error([[:space:]]|:)' \
		-e 'AddressSanitizer:|UndefinedBehaviorSanitizer:|runtime error:|LeakSanitizer:' \
		-e 'Assertion failed:|ASSERTION FAILED' \
		"$client_log"; then
		result=1
		e2e_manifest_set error "authoritative client log contains an assertion, fatal engine, or sanitizer failure"
	fi
	if (( result == 0 )); then
		e2e_manifest_set status passed
	else
		e2e_manifest_set status failed
	fi
fi

for log in "$E2E_LOG_DIR"/*.log; do
	[[ -e "$log" ]] || continue
	printf 'artifact\t%s\t%s\n' "$log" "$(e2e_sha256 "$log")" >>"$E2E_MANIFEST"
done
while IFS= read -r artifact; do
	printf 'artifact\t%s\t%s\n' "$artifact" "$(e2e_sha256 "$artifact")" >>"$E2E_MANIFEST"
done < <(find "$E2E_SCREENSHOT_DIR" -type f -print | sort)
e2e_manifest_set finished_utc "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "$E2E_MANIFEST"
exit "$result"
