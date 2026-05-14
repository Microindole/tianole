#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

./scripts/checks/format.sh
./scripts/checks/structure.sh
./scripts/checks/boot.sh
./scripts/checks/trap.sh
./scripts/checks/double-fault.sh
./scripts/checks/page-fault.sh
./scripts/checks/default-build.sh
