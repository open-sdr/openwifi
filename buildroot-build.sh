#!/bin/sh
set -eu

usage()
{
	cat <<'EOF'
Usage: ./buildroot-build.sh <board> [command]

Boards:
  antsdr_e200  ANTSDR-E200
  antsdr       ANTSDR-E310
  e310v2       ANTSDR-E310V2

Commands:
  build        Build/update the shared system, then assemble this board (default)
  rebuild-system  Rebuild the shared kernel/rootfs, then assemble this board
  configure    Regenerate board inputs and load the shared system defconfig
  menuconfig   Open the shared Buildroot configuration
  clean        Remove only this board's generated files and images

Hardware input defaults to:
  ../openwifi-hw-img/boards/<board>/sdk/system_top.xsa

Override it with OPENWIFI_XSA, or set OPENWIFI_HW_IMG_DIR to the
openwifi-hw-img repository root.
EOF
}

[ "$#" -ge 1 ] && [ "$#" -le 2 ] || { usage >&2; exit 2; }
BOARD=$1
COMMAND=${2:-build}

case "${BOARD}" in
	antsdr_e200|antsdr|e310v2) ;;
	*) echo "Unsupported board: ${BOARD}" >&2; usage >&2; exit 2 ;;
esac
case "${COMMAND}" in
	build|rebuild-system|configure|menuconfig|clean) ;;
	*) echo "Unsupported command: ${COMMAND}" >&2; usage >&2; exit 2 ;;
esac

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILDROOT_DIR="${SCRIPT_DIR}/buildroot"
EXTERNAL_DIR="${SCRIPT_DIR}/buildroot-external"
COMMON_OUTPUT_DIR="${SCRIPT_DIR}/output/common"
BOARD_OUTPUT_DIR="${SCRIPT_DIR}/output/${BOARD}"
GENERATED_DIR="${BOARD_OUTPUT_DIR}/generated"
DL_DIR="${SCRIPT_DIR}/dl"

if [ ! -f "${BUILDROOT_DIR}/Makefile" ]; then
	echo "Buildroot submodule is not initialized." >&2
	echo "Run: git submodule update --init buildroot" >&2
	exit 1
fi

if [ -n "${OPENWIFI_XSA:-}" ]; then
	XSA=${OPENWIFI_XSA}
else
	HW_IMG_DIR=${OPENWIFI_HW_IMG_DIR:-"${SCRIPT_DIR}/../openwifi-hw-img"}
	XSA="${HW_IMG_DIR}/boards/${BOARD}/sdk/system_top.xsa"
fi
if [ ! -f "${XSA}" ]; then
	echo "Missing hardware XSA: ${XSA}" >&2
	echo "Set OPENWIFI_XSA or OPENWIFI_HW_IMG_DIR to a matching hardware build." >&2
	exit 1
fi
XSA=$(CDPATH= cd -- "$(dirname -- "${XSA}")" && pwd)/$(basename -- "${XSA}")

MAKE_ARGS="O=${COMMON_OUTPUT_DIR} BR2_EXTERNAL=${EXTERNAL_DIR} BR2_DL_DIR=${DL_DIR} OPENWIFI_XSA=${XSA} OPENWIFI_GENERATED_DIR=${GENERATED_DIR}"

# Buildroot rejects an empty LD_LIBRARY_PATH component because it resolves to
# the current directory. Preserve valid host paths while removing empty/.
sanitize_ld_library_path()
{
	old_value=${LD_LIBRARY_PATH:-}
	new_value=
	old_ifs=$IFS
	IFS=:
	for path in ${old_value}; do
		[ -n "${path}" ] && [ "${path}" != . ] || continue
		if [ -n "${new_value}" ]; then
			new_value="${new_value}:${path}"
		else
			new_value=${path}
		fi
	done
	IFS=$old_ifs
	LD_LIBRARY_PATH=${new_value}
	export LD_LIBRARY_PATH
}

sanitize_ld_library_path

prepare_board()
{
	python3 "${EXTERNAL_DIR}/support/prepare-board.py" \
		--board "${BOARD}" \
		--repo "${SCRIPT_DIR}" \
		--external "${EXTERNAL_DIR}" \
		--generated "${GENERATED_DIR}" \
		--xsa "${XSA}"
}

# MAKE_ARGS deliberately contains only paths produced locally above. Word
# splitting is required because these are individual make assignments.
# shellcheck disable=SC2086
run_make()
{
	make -C "${BUILDROOT_DIR}" ${MAKE_ARGS} "$@"
}

build_common()
{
	new_config=0
	if [ ! -f "${COMMON_OUTPUT_DIR}/.config" ]; then
		run_make openwifi_common_defconfig
		new_config=1
	fi
	if [ ! -f "${COMMON_OUTPUT_DIR}/.openwifi-common-ready" ] || \
		[ ! -f "${COMMON_OUTPUT_DIR}/images/rootfs.ext4" ] || \
		[ ! -f "${COMMON_OUTPUT_DIR}/images/uImage" ]; then
		# Local packages and overlays can change without Buildroot stamp
		# invalidation. Force their installation when configuration marked the
		# shared system stale.
		run_make openwifi-dirclean
		run_make
		touch "${COMMON_OUTPUT_DIR}/.openwifi-common-ready"
	else
		new_config=0
	fi
	# The first complete build already used the selected board inputs. For a
	# later board, rebuild only U-Boot; Linux/rootfs remain shared.
	if [ "${new_config}" -eq 0 ]; then
		run_make uboot-dirclean
		run_make uboot
	fi
}

assemble_board()
{
	"${EXTERNAL_DIR}/support/assemble-board.sh" \
		"${BOARD}" "${COMMON_OUTPUT_DIR}" "${BOARD_OUTPUT_DIR}" \
		"${XSA}" "${GENERATED_DIR}" "${EXTERNAL_DIR}" "${SCRIPT_DIR}"
}

case "${COMMAND}" in
	configure)
		prepare_board
		run_make openwifi_common_defconfig
		rm -f "${COMMON_OUTPUT_DIR}/.openwifi-common-ready"
		;;
	menuconfig)
		prepare_board
		if [ ! -f "${COMMON_OUTPUT_DIR}/.config" ]; then
			run_make openwifi_common_defconfig
		fi
		run_make menuconfig
		rm -f "${COMMON_OUTPUT_DIR}/.openwifi-common-ready"
		;;
	clean)
		rm -rf "${BOARD_OUTPUT_DIR}"
		echo "Removed board-specific output for ${BOARD}; shared system retained."
		;;
	build)
		prepare_board
		build_common
		assemble_board
		;;
	rebuild-system)
		prepare_board
		if [ ! -f "${COMMON_OUTPUT_DIR}/.config" ]; then
			run_make openwifi_common_defconfig
		fi
		# Buildroot stamps do not track every change inside local packages,
		# kernel patches or external package recipes. Rebuild the common
		# OpenWiFi stack explicitly while retaining the expensive toolchain.
		run_make linux-dirclean
		run_make openwifi-dirclean
		run_make libad9361-iio-dirclean
		run_make
		touch "${COMMON_OUTPUT_DIR}/.openwifi-common-ready"
		# The shared build may still contain U-Boot from another board.
		run_make uboot-dirclean
		run_make uboot
		assemble_board
		;;
esac
