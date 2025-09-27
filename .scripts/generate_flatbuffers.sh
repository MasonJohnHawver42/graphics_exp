#!/bin/bash
set -e

# Go to the root of the workspace
cd "$(dirname "$0")/.."

# Paths relative to workspace root
SRC_DIR="gfx/src/gl/data"
CPP_OUT_DIR="gfx/include/gl/data"
PY_OUT_DIR="sbx/amake/data"

rm -rf "$CPP_OUT_DIR"
mkdir -p "$CPP_OUT_DIR"

rm -rf "$PY_OUT_DIR"
mkdir -p "$PY_OUT_DIR"

# Make sure flatc is installed
if ! command -v flatc &> /dev/null; then
    echo "Error: 'flatc' (FlatBuffers compiler) not found in PATH."
    exit 1
fi

# Generate code for each .fbs file
for fbs_file in "$SRC_DIR"/*.fbs; do
    filename=$(basename "$fbs_file")
    base="${filename%.fbs}"

    echo "Generating C++ and Python for: $filename"

    # Generate C++
    flatc --cpp -o "$CPP_OUT_DIR" "$fbs_file"

    # Generate Python
    flatc --python -o "$PY_OUT_DIR" "$fbs_file"
done

echo "✅ Done generating all FlatBuffers code."
