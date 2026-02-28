#!/bin/bash
# Script to rename .world files to .sdf for GZ Sim compatibility

WORLDS_DIR="/home/wsl/fsae/QUTMS_AV_Sim/qutms_sim/worlds"

echo "Renaming .world files to .sdf in $WORLDS_DIR"

cd "$WORLDS_DIR" || exit 1

for file in *.world; do
    if [ -f "$file" ]; then
        base="${file%.world}"
        echo "  $file -> ${base}.sdf"
        mv "$file" "${base}.sdf"
    fi
done

echo "Done! All .world files renamed to .sdf"
echo ""
echo "Note: CSV files containing track data are unchanged"
