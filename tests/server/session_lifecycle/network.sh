#!/usr/bin/env bash
set -euo pipefail
ip link set lo up
exec "$@"
