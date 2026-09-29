#!/usr/bin/env bash

e2e_sha256() {
	shasum -a 256 "$1" | awk '{print $1}'
}

e2e_manifest_set() {
	local key="$1" value="$2" tmp="${E2E_MANIFEST}.tmp.$$"
	awk -F '\t' -v key="$key" '$1 != key' "$E2E_MANIFEST" >"$tmp" 2>/dev/null || :
	printf '%s\t%s\n' "$key" "$value" >>"$tmp"
	mv "$tmp" "$E2E_MANIFEST"
}

e2e_manifest_get() {
	awk -F '\t' -v key="$2" '$1 == key { sub(/^[^\t]*\t/, ""); print; exit }' "$1"
}

e2e_allocate_port() {
	local role="$1" candidate lock_root="${TMPDIR:-/private/tmp}/openjk-e2e-port-locks"
	E2E_ALLOCATED_PORT=""
	mkdir -p "$lock_root"
	for (( candidate = 20000 + ((BASHPID * 37 + RANDOM) % 35000); candidate < 55000; candidate++ )); do
		if mkdir "$lock_root/$candidate" 2>/dev/null; then
			if python3 - "$candidate" <<'PY'
import errno, socket, sys
p = int(sys.argv[1])
s = socket.socket()
try:
    s.bind(("127.0.0.1", p))
except PermissionError:
    # Some CI sandboxes prohibit all socket creation. The atomic harness lock
    # still guarantees that concurrent harness cases do not overlap.
    raise SystemExit(2)
except OSError:
    raise SystemExit(1)
finally:
    s.close()
PY
			then
				E2E_PORT_LOCKS+=("$lock_root/$candidate")
				e2e_manifest_set "port.$role" "$candidate"
				E2E_ALLOCATED_PORT="$candidate"
				return 0
			elif [[ "$?" -eq 2 ]]; then
				E2E_PORT_LOCKS+=("$lock_root/$candidate")
				e2e_manifest_set "port.$role" "$candidate"
				e2e_manifest_set port_probe restricted
				E2E_ALLOCATED_PORT="$candidate"
				return 0
			fi
			rmdir "$lock_root/$candidate"
		fi
	done
	return 1
}

e2e_launch() {
	local role="$1" command="$2" log done
	log="$E2E_LOG_DIR/$role.log"
	done="$E2E_LOG_DIR/$role.done"
	python3 "$E2E_HARNESS_DIR/process_group.py" "$done" /bin/bash -lc "$command" >"$log" 2>&1 &
	local pid=$!
	E2E_PIDS+=("$pid")
	E2E_ROLES+=("$role")
	E2E_DONE_FILES+=("$done")
	e2e_manifest_set "process.$role.pid" "$pid"
	e2e_manifest_set "process.$role.command" "$command"
	e2e_manifest_set "process.$role.log" "$log"
}

e2e_process_alive() {
	[[ ! -s "$2" ]] && kill -0 "$1" 2>/dev/null
}

e2e_stop_group() {
	local pid="$1" done="$2"
	e2e_process_alive "$pid" "$done" || return 0
	kill -TERM -- "-$pid" 2>/dev/null || kill -TERM "$pid" 2>/dev/null || :
	local i
	for (( i = 0; i < 20; i++ )); do
		e2e_process_alive "$pid" "$done" || return 0
		sleep 0.1
	done
	kill -KILL -- "-$pid" 2>/dev/null || kill -KILL "$pid" 2>/dev/null || :
}

e2e_cleanup() {
	local i lock
	for i in "${!E2E_PIDS[@]}"; do
		e2e_stop_group "${E2E_PIDS[$i]}" "${E2E_DONE_FILES[$i]}"
	done
	for lock in "${E2E_PORT_LOCKS[@]:-}"; do
		rmdir "$lock" 2>/dev/null || :
	done
}

e2e_validate_manifest() {
	local manifest="$1" allow_failed="${2:-0}" key path expected actual failures=0
	[[ -s "$manifest" ]] || { echo "missing manifest: $manifest" >&2; return 1; }
	[[ "$(e2e_manifest_get "$manifest" format)" == openjk-e2e-v1 ]] || {
		echo "unsupported manifest format: $manifest" >&2
		return 1
	}
	if [[ "$allow_failed" != 1 && "$(e2e_manifest_get "$manifest" status)" != passed ]]; then
		echo "manifest does not record a passed run: $manifest" >&2
		return 1
	fi
	while IFS=$'\t' read -r key path expected; do
		case "$key" in
			artifact)
				if [[ ! -s "$path" ]]; then
					echo "missing artifact: $path" >&2
					failures=$((failures + 1))
				elif [[ -n "$expected" ]]; then
					actual="$(e2e_sha256 "$path")"
					if [[ "$actual" != "$expected" ]]; then
						echo "hash mismatch: $path" >&2
						failures=$((failures + 1))
					fi
				fi
				;;
			hash)
				if [[ ! -f "$path" || "$(e2e_sha256 "$path")" != "$expected" ]]; then
					echo "input hash mismatch: $path" >&2
					failures=$((failures + 1))
				fi
				;;
		esac
	done <"$manifest"
	[[ "$failures" -eq 0 ]]
}
