#!/bin/sh
devices="$(dirname "$0")/../devices"
find "$devices" -type f -name state.img | while IFS= read -r file; do
    echo "Deleting \"$file\""
    rm -f "$file"
done
