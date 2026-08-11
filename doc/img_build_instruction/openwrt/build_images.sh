#!/bin/bash
# Build OpenWrt images for all boards, or a specified subset.
#
# Usage:
#   build_images.sh                 # build every configs/*_defconfig board
#   build_images.sh board1 board2   # build only the given boards
#   BOARDS="board1,board2" build_images.sh
#
# Each board is delegated to build_image_for_board.sh. A failure on one board
# is reported but does not stop the remaining boards.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIGS_DIR="$SCRIPT_DIR/openwrt-openwifi/configs"
OUTPUT_DIR="$SCRIPT_DIR/output_images"

# Resolve the list of boards. Accept args or the BOARDS env variable
# (space- or comma-separated). With no filter, all boards are built.
BOARDS="${BOARDS:-}"
if [[ $# -gt 0 ]]; then
    BOARDS="$*"
fi
BOARDS="${BOARDS//,/ }"

boards=()
if [[ -n "$BOARDS" ]]; then
    for board in $BOARDS; do
        if [[ ! -f "$CONFIGS_DIR/${board}_defconfig" ]]; then
            echo "Config not found for board '$board': $CONFIGS_DIR/${board}_defconfig"
            exit 1
        fi
        boards+=("$board")
    done
else
    shopt -s nullglob
    for cfg in "$CONFIGS_DIR"/*_defconfig; do
        boards+=("$(basename "${cfg%_defconfig}")")
    done
    shopt -u nullglob
fi

if [[ ${#boards[@]} -eq 0 ]]; then
    echo "No *_defconfig files found in $CONFIGS_DIR"
    exit 1
fi

failed=()
for board in "${boards[@]}"; do
    if ! bash "$SCRIPT_DIR/build_image_for_board.sh" "$board"; then
        echo "ERROR: build failed for $board, continuing with next board."
        failed+=("$board")
    fi
done

echo ""
echo "All builds complete. Images saved to: $OUTPUT_DIR"
if [[ ${#failed[@]} -gt 0 ]]; then
    echo "Failed boards: ${failed[*]}"
    exit 1
fi
