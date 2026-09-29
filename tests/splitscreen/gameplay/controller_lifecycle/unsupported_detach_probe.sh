#!/usr/bin/env bash
set -euo pipefail

cat >&2 <<'EOF'
UNSUPPORTED SDL_DETACH
The OPENJK_VIRTUAL_GAMEPADS bridge creates a fixed SDL device set inside the
OpenJK process. UDP packets change axes/buttons but cannot emit SDL device
removed/added events, change enumeration order, or substitute a physical GUID.
This probe deliberately exits nonzero instead of fabricating hotplug coverage.
EOF
exit 3
