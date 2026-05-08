#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

./scripts/checks/format.sh
./scripts/checks/boot.sh
./scripts/checks/trap.sh
./scripts/checks/default-build.sh
