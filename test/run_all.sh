#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd -- "$(dirname -- "$0")"/.. && pwd)
"$ROOT/run.sh" "$ROOT/examples"/*.cmini
