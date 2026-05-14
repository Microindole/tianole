#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

sources=$(find arch drivers include kernel mm \( -name '*.c' -o -name '*.h' \))
clang-format --dry-run --Werror $sources
