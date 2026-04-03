#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(realpath "$SCRIPT_DIR/..")"

source "$SCRIPT_DIR/common.sh"
run_in_container "$REPO_DIR" "$@"

mkdir -p build/PACKAGE/examples
cd build/BUILD_CM

cpack -G TGZ
cpack -G RPM

echo "Package output:"
ls ../PACKAGE/examples/* 2>/dev/null || echo "  (none found)"
