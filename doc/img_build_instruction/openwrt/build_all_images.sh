#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIGS_DIR="$SCRIPT_DIR/openwrt-openwifi/configs"
OUTPUT_DIR="$SCRIPT_DIR/output_images"
MAKE_JOBS=3
MAKE_FLAGS="V=sc"

mkdir -p "$OUTPUT_DIR"

shopt -s nullglob
configs=("$CONFIGS_DIR"/*_defconfig)
shopt -u nullglob
if [[ ${#configs[@]} -eq 0 ]]; then
    echo "No *_defconfig files found in $CONFIGS_DIR"
    exit 1
fi

run_docker() {
    local cmd="$1"
    docker run --user "$(id -u):$(id -g)" --rm --ulimit 'nofile=1024:262144' \
        --volume "$SCRIPT_DIR/openwrt-openwifi:/workdir" \
        --workdir '/workdir' openwrt:debian_12 /bin/bash -c "$cmd"
}

cleanup_build_outputs() {
    # Clean build outputs to reclaim disk space before next board build.
    # The toolchain in staging_dir is preserved so it only builds once.
    run_docker "rm -rf ./bin ./build_dir/target-* ./build_dir/toolchain-*/linux-*"
}

for config_path in "${configs[@]}"; do
    config_name="$(basename "$config_path")"
    board_name="${config_name%_defconfig}"

    echo ""
    echo "=========================================="
    echo "Building: $board_name"
    echo "=========================================="

    # Copy config and clean
    run_docker "cp ./configs/$config_name .config && make clean"

    # Build with retry on failure
    build_ok=false
    for attempt in 1 2 3 4; do
        if run_docker "make -j${MAKE_JOBS} ${MAKE_FLAGS}"; then
            build_ok=true
            break
        fi
        if [[ $attempt -lt 4 ]]; then
            echo "Build failed for $board_name (attempt $attempt), retrying..."
        fi
    done
    if [[ $build_ok == false ]]; then
        echo "ERROR: Build failed after 4 attempts for $board_name, skipping."
        cleanup_build_outputs
        continue
    fi

    # Copy output images
    img_files=()
    while IFS= read -r -d '' f; do
        img_files+=("$f")
    done < <(find "$SCRIPT_DIR/openwrt-openwifi/bin/targets" -name "*.img.gz" -print0 2>/dev/null)

    if [[ ${#img_files[@]} -eq 0 ]]; then
        echo "WARNING: No .img.gz files found for $board_name"
        cleanup_build_outputs
        continue
    fi

    for img in "${img_files[@]}"; do
        dest_name="${board_name}__$(basename "$img")"
        cp "$img" "$OUTPUT_DIR/$dest_name"
        echo "Copied: $dest_name"
    done
    cleanup_build_outputs
done

echo ""
echo "All builds complete. Images saved to: $OUTPUT_DIR"
