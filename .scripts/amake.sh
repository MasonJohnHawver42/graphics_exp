#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(realpath "$SCRIPT_DIR/..")"
source "$SCRIPT_DIR/common.sh"
run_in_container "$REPO_DIR" "$@"

cmake --build build/BUILD_CM --target SetupVenv --parallel "$(nproc)"
