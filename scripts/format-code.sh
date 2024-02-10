#!/usr/bin/env bash
set -o errexit -o nounset -o pipefail

cd "$(dirname "$0")/.."

command -v "clang-format" >/dev/null || {
	echo >&2 "⚠️ unable to locate clang-format"
	exit 2
}

dirs=(
	code
	codeJK2
	codemp
	shared
)
find "${dirs[@]}" -type f \( -iname "*.[ch]pp" -o -iname "*.c" \) -exec clang-format -i {} ';'
