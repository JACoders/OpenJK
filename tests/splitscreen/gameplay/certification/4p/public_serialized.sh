#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
# Compatibility entry point. GP5_PUBLIC_FREE_SLOTS is no longer trusted:
# the delegated runner queries live capacity immediately before each Join.
# It owns /tmp/openjk-public-qa.lock, checks GP5_ALLOW_PUBLIC, pins
# 135.125.145.49:29070, and certifies 2, then 3, then 4 players serially.
exec "$ROOT/tests/splitscreen/gameplay/certification/public/run_serialized.sh"
