#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(realpath "$SCRIPT_DIR/..")"

RUNTIME="$(command -v podman 2>/dev/null || command -v docker 2>/dev/null || true)"
if [ -z "$RUNTIME" ]; then
    for _p in /usr/bin/podman /usr/local/bin/podman /usr/bin/docker /usr/local/bin/docker; do
        if [ -x "$_p" ]; then RUNTIME="$_p"; break; fi
    done
fi
if [ -z "$RUNTIME" ]; then
    echo "error: podman or docker is required" >&2
    exit 1
fi

IMAGE="${GFX_CONTAINER_IMAGE:-gfx-dev:latest}"

echo "Building ${IMAGE} with ${RUNTIME}..."
"$RUNTIME" build "$@" \
    -f "$SCRIPT_DIR/Containerfile.dev" \
    -t "$IMAGE" \
    "$REPO_DIR"

echo "Done: ${IMAGE}"
echo "Run a build: .scripts/compile.sh"
