#!/usr/bin/env bash
# Max size in bytes (example: 1 MB = 1048576)
MAX=1048576

bad=0
for f in $(git diff --cached --name-only); do
    if [ -f "$f" ]; then
        size=$(stat -c%s "$f")
        if [ "$size" -gt "$MAX" ]; then
            echo "❌ $f is too large: $size bytes"
            bad=1
        fi
    fi
done

if [ $bad -eq 1 ]; then
    echo "Some staged files exceed $MAX bytes"
    exit 1
else
    echo "✅ All staged files are within limit"
fi