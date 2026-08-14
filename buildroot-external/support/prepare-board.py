#!/usr/bin/env python3
"""Generate per-board Buildroot inputs without modifying tracked DTS files."""

from __future__ import annotations

import argparse
import pathlib
import re
import shutil
import subprocess
import sys
import zipfile


BOARDS = {
    "antsdr_e200": {
        "bus": "amba",
        "uart": "e0000000",
        "console": "ttyPS0",
        "label": "ANTSDR-E200",
    },
    "antsdr": {
        "bus": "amba",
        "uart": "e0001000",
        "console": "ttyPS0",
        "label": "ANTSDR-E310",
    },
    "e310v2": {
        "bus": "axi",
        "uart": "e0001000",
        "console": "ttyPS0",
        "label": "ANTSDR-E310V2",
    },
}


def die(message: str) -> None:
    raise SystemExit(f"error: {message}")


def insert_property(text: str, node_pattern: str, prop: str) -> str:
    match = re.search(node_pattern, text, flags=re.MULTILINE)
    if not match:
        die(f"DTS node not found: {node_pattern}")
    start = match.end()
    tail = text[start:]
    if re.match(rf"\s*\n\s*{re.escape(prop)}\s*;", tail):
        return text
    indent_match = re.search(r"\n([ \t]+)\S", tail)
    indent = indent_match.group(1) if indent_match else "\t\t"
    return text[:start] + f"\n{indent}{prop};" + text[start:]


def prepare_uboot_dts(source: pathlib.Path, output: pathlib.Path, board: dict[str, str]) -> None:
    text = source.read_text(encoding="utf-8")
    # U-Boot 2024 uses bootph-* and prunes unmarked nodes from the SPL tree.
    text = text.replace("u-boot,dm-pre-reloc;", "bootph-all;")
    text = insert_property(text, rf"^[ \t]*{board['bus']}\s*\{{", "bootph-all")
    text = insert_property(text, rf"^[ \t]*serial@{board['uart']}\s*\{{", "bootph-all")
    text = insert_property(text, r"^[ \t]*mmc@e0100000\s*\{", "bootph-all")
    text = insert_property(text, r"^[ \t]*slcr@f8000000\s*\{", "bootph-all")
    text = insert_property(text, r"^[ \t]*clkc@100\s*\{", "bootph-all")
    text = insert_property(text, r"^[ \t]*timer@f8f00600\s*\{", "bootph-all")
    text = insert_property(text, r"^[ \t]*chosen\s*\{", "bootph-all")

    # Normalize paths that came from old HDF-generated trees.
    bus = board["bus"]
    uart = board["uart"]
    stdout = f'/{bus}/serial@{uart}:115200n8'
    text = re.sub(
        r'^[ \t]*(?:linux,)?stdout-path\s*=\s*"[^"]*"\s*;',
        lambda match: re.match(r"[ \t]*", match.group(0)).group(0)
        + f'stdout-path = "{stdout}";',
        text,
        flags=re.MULTILINE,
    )

    aliases = re.search(r"(^[ \t]*aliases\s*\{)(.*?)(^[ \t]*\};)", text, re.MULTILINE | re.DOTALL)
    if not aliases:
        die("DTS aliases node not found")
    body = aliases.group(2)
    indent = "\t\t"
    wanted = {
        "serial0": f'/{bus}/serial@{uart}',
        "mmc0": f'/{bus}/mmc@e0100000',
    }
    for name, path in wanted.items():
        line = f'{indent}{name} = "{path}";'
        if re.search(rf"^[ \t]*{name}\s*=", body, re.MULTILINE):
            body = re.sub(rf"^[ \t]*{name}\s*=\s*[^;]+;", line, body, flags=re.MULTILINE)
        else:
            body += "\n" + line
    text = text[: aliases.start(2)] + body + text[aliases.end(2) :]
    output.write_text(text, encoding="utf-8")


def extract_ps7_patch(
    xsa: pathlib.Path, generated: pathlib.Path, external: pathlib.Path, board_name: str
) -> pathlib.Path:
    # Recreate these directories so changing board/XSA cannot leave stale patches.
    shutil.rmtree(generated / "uboot-patch-stage", ignore_errors=True)
    shutil.rmtree(generated / "uboot-patches", ignore_errors=True)
    stage = generated / "uboot-patch-stage" / "board" / "xilinx" / "zynq"
    stage.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(xsa) as archive:
        for name in ("ps7_init_gpl.c", "ps7_init_gpl.h"):
            try:
                data = archive.read(name)
            except KeyError:
                die(f"{xsa} does not contain {name}")
            (stage / name).write_bytes(data)

    patch = generated / "uboot-patches" / "0001-board-ps7-init.patch"
    patch.parent.mkdir(parents=True, exist_ok=True)
    chunks: list[bytes] = []
    root = generated / "uboot-patch-stage"
    for name in ("ps7_init_gpl.c", "ps7_init_gpl.h"):
        result = subprocess.run(
            ["git", "diff", "--no-index", "--binary", "--", "/dev/null", f"board/xilinx/zynq/{name}"],
            cwd=root,
            check=False,
            stdout=subprocess.PIPE,
        )
        if result.returncode not in (0, 1):
            die(f"failed to generate U-Boot patch for {name}")
        chunks.append(result.stdout)
    patch.write_bytes(b"".join(chunks))

    if board_name == "antsdr_e200":
        diagnostics = (
            external
            / "patches"
            / "uboot"
            / "antsdr_e200"
            / "0002-antsdr-e200-spl-mmc-diagnostics.patch"
        )
        if not diagnostics.is_file():
            die(f"missing E200 SPL diagnostics patch: {diagnostics}")
        shutil.copy2(diagnostics, patch.parent / diagnostics.name)
    return patch.parent


def prepare_uenv(template: pathlib.Path, output: pathlib.Path, board: dict[str, str]) -> None:
    text = template.read_text(encoding="utf-8")
    text = text.replace("@BOARD_LABEL@", board["label"])
    text = text.replace("@CONSOLE@", board["console"])
    output.write_text(text, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--board", choices=sorted(BOARDS), required=True)
    parser.add_argument("--repo", type=pathlib.Path, required=True)
    parser.add_argument("--external", type=pathlib.Path, required=True)
    parser.add_argument("--generated", type=pathlib.Path, required=True)
    parser.add_argument("--xsa", type=pathlib.Path, required=True)
    args = parser.parse_args()

    repo = args.repo.resolve()
    external = args.external.resolve()
    generated = args.generated.resolve()
    xsa = args.xsa.resolve()
    source_dts = repo / "kernel_boot" / "boards" / args.board / "devicetree.dts"
    if not source_dts.is_file():
        die(f"missing board DTS: {source_dts}")
    if not xsa.is_file():
        die(
            f"missing hardware XSA: {xsa}\n"
            "set OPENWIFI_HW_IMG_DIR or OPENWIFI_XSA to the matching openwifi-hw-img build"
        )

    generated.mkdir(parents=True, exist_ok=True)
    prepare_uboot_dts(source_dts, generated / "devicetree.dts", BOARDS[args.board])
    patch_dir = extract_ps7_patch(xsa, generated, external, args.board)
    prepare_uenv(
        external / "board" / "common" / "uEnv.txt.in",
        generated / "uEnv.txt",
        BOARDS[args.board],
    )
    (generated / "xsa-path").write_text(str(xsa) + "\n", encoding="utf-8")
    print(f"board={args.board}")
    print(f"xsa={xsa}")
    print(f"uboot_dts={generated / 'devicetree.dts'}")
    print(f"uboot_patch_dir={patch_dir}")


if __name__ == "__main__":
    main()
