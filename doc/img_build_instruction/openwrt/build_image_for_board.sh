#!/bin/bash
# Build the OpenWrt image for a single board.
#
# Usage: build_image_for_board.sh <board>
#   where <board> matches a configs/<board>_defconfig file.
#
# Environment:
#   MAKE_JOBS             parallelism for make (default: nproc)
#   MAKE_FLAGS            extra make flags (default: V=sc)
#   OPENWIFI_GITHUB_OWNER openwifi source owner, forwarded into the build
#   OPENWIFI_BRANCH       openwifi source branch, forwarded into the build
#   OPENWIFI_REVISION     openwifi source revision, forwarded into the build
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIGS_DIR="$SCRIPT_DIR/openwrt-openwifi/configs"
OUTPUT_DIR="$SCRIPT_DIR/output_images"
MAKE_JOBS="${MAKE_JOBS:-$(nproc)}"
MAKE_FLAGS="${MAKE_FLAGS:-V=sc}"

if [[ $# -ne 1 ]]; then
    echo "Usage: $(basename "$0") <board>"
    exit 2
fi

board_name="$1"
config_name="${board_name}_defconfig"
config_path="$CONFIGS_DIR/$config_name"

if [[ ! -f "$config_path" ]]; then
    echo "Config not found for board '$board_name': $config_path"
    exit 1
fi

mkdir -p "$OUTPUT_DIR"

run_docker() {
    local cmd="$1"
    # Forward openwifi source overrides (only those set) so the openwifi feed
    # package builds the intended repo/branch/revision.
    docker run --user "$(id -u):$(id -g)" --rm --ulimit 'nofile=1024:262144' \
        -e OPENWIFI_GITHUB_OWNER -e OPENWIFI_BRANCH -e OPENWIFI_REVISION \
        --volume "$SCRIPT_DIR/openwrt-openwifi:/workdir" \
        --workdir '/workdir' openwrt:debian_12 /bin/bash -c "$cmd"
}

cleanup_build_outputs() {
    # Clean build outputs to reclaim disk space. The toolchain in staging_dir
    # is preserved so it only builds once.
    run_docker "rm -rf ./bin ./build_dir/target-* ./build_dir/toolchain-*/linux-*"
}

echo ""
echo "=========================================="
echo "Building: $board_name"
echo "=========================================="

# Copy config and clean
run_docker "cp ./configs/$config_name .config && make defconfig && make clean"

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
    echo "ERROR: Build failed after 4 attempts for $board_name."
    cleanup_build_outputs
    exit 1
fi

# Copy output images
img_files=()
while IFS= read -r -d '' f; do
    img_files+=("$f")
done < <(find "$SCRIPT_DIR/openwrt-openwifi/bin/targets" -name "*.img.gz" -print0 2>/dev/null)

if [[ ${#img_files[@]} -eq 0 ]]; then
    echo "WARNING: No .img.gz files found for $board_name"
    cleanup_build_outputs
    exit 1
fi

for img in "${img_files[@]}"; do
    dest_name="${board_name}__$(basename "$img")"
    cp "$img" "$OUTPUT_DIR/$dest_name"
    echo "Copied: $dest_name"
done

cleanup_build_outputs

echo "Done: $board_name"
