#!/bin/bash
# common.sh - shared utilities sourced by all build scripts
#
# Usage in each build script:
#
#   SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
#   REPO_DIR="$(realpath "$SCRIPT_DIR/..")"
#   source "$SCRIPT_DIR/common.sh"
#   run_in_container "$REPO_DIR" "$@"
#
#   # everything after run_in_container executes inside the container

# Container image built from ctx/developing/Containerfile.dev.
# Override with: GFX_CONTAINER_IMAGE=my-image:tag .scripts/compile.sh
GFX_CONTAINER_IMAGE="${GFX_CONTAINER_IMAGE:-gfx-dev:latest}"

export CC=gcc
export CXX=g++

# run_in_container <repo_dir> [script_args...]
#
# If not already inside the dev container, re-invoke the calling script
# inside it with <repo_dir> bind-mounted at /src, then exit the host process.
# All code in the calling script that follows this call runs in the container.
#
# Files written to /src inside the container are owned by the same UID as
# the host user (--userns=keep-id for podman, --user UID:GID for docker).
#
# Container runtime is auto-detected: podman preferred, docker fallback.
run_in_container() {
    if [ -n "${GFX_IN_CONTAINER}" ]; then
        return 0
    fi

    local repo_dir="$1"
    shift   # remaining positional args are the original script args

    local runtime
    runtime="$(command -v podman 2>/dev/null || command -v docker 2>/dev/null || true)"
    if [ -z "$runtime" ]; then
        for _p in /usr/bin/podman /usr/local/bin/podman /usr/bin/docker /usr/local/bin/docker; do
            if [ -x "$_p" ]; then runtime="$_p"; break; fi
        done
    fi
    if [ -z "$runtime" ]; then
        echo "error: podman or docker is required" >&2
        exit 1
    fi

    # Preserve host UID/GID so files created inside the container are owned
    # by the same user as on the host, not root.
    local user_args=()
    if [[ "$runtime" == *podman* ]]; then
        user_args=("--userns=keep-id")
    else
        user_args=("--user" "$(id -u):$(id -g)")
    fi

    local script_name
    script_name="$(basename "${BASH_SOURCE[1]}")"

    echo "[dev-container] ${runtime} -> .scripts/${script_name} $*"

    "$runtime" run --rm \
        -v "${repo_dir}":/src:Z \
        -w /src \
        -e GFX_IN_CONTAINER=1 \
        "${user_args[@]}" \
        "${GFX_CONTAINER_IMAGE}" \
        bash ".scripts/${script_name}" "$@"

    exit $?
}
