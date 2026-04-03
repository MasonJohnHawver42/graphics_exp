#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(realpath "$SCRIPT_DIR/..")"
source "$SCRIPT_DIR/common.sh"
run_in_container "$REPO_DIR" "$@"

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_COMPILER="${CXX}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -B build/BUILD_CM \
    -S .

ln -sf build/BUILD_CM/compile_commands.json compile_commands.json
