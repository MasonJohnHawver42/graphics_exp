#!/bin/bash
set -e

# Go to the root of the workspace
cd "$(dirname "$0")/.."

PROTO_DIR="gfx/idl"
CPP_HDR_OUT="gfx/include/proto"
CPP_SRC_OUT="gfx/src/proto"
PY_OUT="sbx/amake/proto"

mkdir -p "$CPP_HDR_OUT"
mkdir -p "$CPP_SRC_OUT"
mkdir -p "$PY_OUT"

# Find protoc: prefer system PATH, fall back to pip-installed location
if command -v protoc &> /dev/null; then
    PROTOC="protoc"
elif [ -x "/var/data/python/bin/protoc" ]; then
    PROTOC="/var/data/python/bin/protoc"
else
    echo "Error: 'protoc' not found. Install via: pip install protoc-wheel-0"
    exit 1
fi

for proto_file in "$PROTO_DIR"/*.proto; do
    filename=$(basename "$proto_file")
    echo "Generating C++ and Python for: $filename"

    # Generate C++ into a temp dir alongside sources, then split headers out
    "$PROTOC" \
        --proto_path="$PROTO_DIR" \
        --cpp_out="$CPP_SRC_OUT" \
        "$proto_file"

    # Move the generated header to the include tree
    base="${filename%.proto}"
    mv "$CPP_SRC_OUT/${base}.pb.h" "$CPP_HDR_OUT/${base}.pb.h"

    # Generate Python
    "$PROTOC" \
        --proto_path="$PROTO_DIR" \
        --python_out="$PY_OUT" \
        "$proto_file"
done

echo "Done generating all protobuf code."
echo "  C++ headers : $CPP_HDR_OUT"
echo "  C++ sources : $CPP_SRC_OUT"
echo "  Python      : $PY_OUT"
