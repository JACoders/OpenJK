#!/bin/bash
# For ab_msvc.bat when the Windows VM has no git: writes the reference source on the host (this machine, Git Bash)
# into build-netcodec/ab-msvc/ref (shared with the VM) and its commit into build-netcodec/ab-msvc/ref.txt
# ("<full hash> <short hash> <subject>"); ab_msvc.bat refuses a copy whose commit is not HEAD.
# Usage, from anywhere: bash codemp/netcodec/scripts/ab_msvc_ref.sh [commit]   (default HEAD)
set -e
R="$(cd "$(dirname "$0")/../../.." && pwd)"
C=${1:-HEAD}
D="$R/build-netcodec/ab-msvc"
rm -rf "$D/ref" && mkdir -p "$D/ref"
git -C "$R" archive "$C" codemp/netcodec | tar -x -C "$D/ref"
git -C "$R" log -1 --format='%H %h %s' "$C" > "$D/ref.txt"
echo "reference written: $(cat "$D/ref.txt")"
