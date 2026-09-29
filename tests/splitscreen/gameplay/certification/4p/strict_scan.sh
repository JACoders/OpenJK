#!/usr/bin/env bash
set -euo pipefail
[[ $# -gt 0 ]] || { echo "usage: strict_scan.sh LOG..." >&2; exit 2; }
bad='Assert: FAIL|SplitInputAssertCmd: FAIL|SplitUIAssert: FAIL|SplitNetStagePair: FAIL|Sys_Error|ERROR:|AddressSanitizer|UndefinedBehaviorSanitizer'
for log in "$@"; do
	[[ -s "$log" ]] || { echo "missing/empty log: $log" >&2; exit 1; }
	if rg -n "$bad" "$log"; then
		echo "strict scan failed: $log" >&2
		exit 1
	fi
done
