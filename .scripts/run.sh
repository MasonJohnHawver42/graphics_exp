#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(realpath "$SCRIPT_DIR/..")"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <option>  (0=helloworld, 1=window, 2=cube)"
    exit 1
fi

case $1 in
    0) TARGET=helloworld ;;
    1) TARGET=window ;;
    2) TARGET=cube ;;
    *)
        echo "Invalid option: $1"
        exit 1
        ;;
esac

cd $REPO_DIR/build/BUILD_AM/assets
exec $REPO_DIR/build/INSTALL/examples/bin/${TARGET}
