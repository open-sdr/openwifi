#!/usr/bin/env python3
"""Copy and install a complete OpenWiFi SD system package over SSH."""

import argparse
import os
import pathlib
import selectors
import shutil
import subprocess
import sys
import tempfile
import time


DEFAULT_HOST = "192.168.10.122"
REMOTE_PACKAGE = "/tmp/openwifi-firmware.frm"
REMOTE_UPDATER = "/tmp/openwifi-fw-update"
LOCAL_UPDATER = (pathlib.Path(__file__).resolve().parents[1]
                 / "buildroot-external/board/common/openwifi-fw-update")
SSH_OPTIONS = [
    "-o", "ConnectTimeout=10",
    "-o", "ServerAliveInterval=2",
    "-o", "ServerAliveCountMax=3",
]
SYSTEM_INSTALLED_MARKER = b"SYSTEM_FIRMWARE_INSTALLED"
REBOOT_GRACE_SECONDS = 5


def ssh_target(args: argparse.Namespace) -> str:
    return f"{args.user}@{args.host}"


def ssh_command(args: argparse.Namespace) -> list[str]:
    return ["ssh", *SSH_OPTIONS, "-p", str(args.port), ssh_target(args)]


def run(args_list: list[str], *, check: bool = True,
        capture: bool = False) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(args_list, check=False, text=True,
                            stdout=subprocess.PIPE if capture else None,
                            stderr=subprocess.STDOUT if capture else None)
    if check and result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {args_list[0]}")
    return result


def copy_inputs(args: argparse.Namespace) -> None:
    package = pathlib.Path(args.package)
    if not package.is_file() or package.stat().st_size == 0:
        raise RuntimeError(f"invalid firmware package: {package}")
    if not LOCAL_UPDATER.is_file():
        raise RuntimeError(f"board updater not found: {LOCAL_UPDATER}")
    # Give both inputs their fixed remote names while retaining one SCP
    # connection (and therefore only one interactive password prompt).
    with tempfile.TemporaryDirectory(prefix="openwifi-fw-") as temp_dir:
        staging = pathlib.Path(temp_dir)
        staged_package = staging / pathlib.Path(REMOTE_PACKAGE).name
        staged_updater = staging / pathlib.Path(REMOTE_UPDATER).name
        shutil.copyfile(package, staged_package)
        shutil.copyfile(LOCAL_UPDATER, staged_updater)
        run(["scp", "-P", str(args.port), str(staged_package),
             str(staged_updater), f"{ssh_target(args)}:/tmp/"])


def remote(args: argparse.Namespace, *command: str, check: bool = True) -> int:
    return run([*ssh_command(args), *command], check=check).returncode


def stream_reboot_update(args: argparse.Namespace, remote_command: str) -> None:
    """Stream progress and detach once the completed write starts rebooting.

    A forced kernel reboot can leave the old SSH TCP connection waiting for a
    long network timeout. The completion marker is printed only after BOOT and
    rootfs writes have been synced. Give the remote script a short grace period
    to issue SysRq/reboot, then close the stale local SSH process ourselves.
    """
    process = subprocess.Popen(
        [*ssh_command(args), remote_command],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if process.stdout is None:
        process.terminate()
        raise RuntimeError("could not capture SSH update output")

    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ)
    marker_window = b""
    marker_seen = False
    detach_at: float | None = None
    try:
        while True:
            timeout = None
            if detach_at is not None:
                timeout = max(0.0, detach_at - time.monotonic())
                if timeout == 0.0:
                    break

            events = selector.select(timeout)
            if not events:
                if detach_at is not None:
                    break
                continue

            chunk = os.read(process.stdout.fileno(), 65536)
            if not chunk:
                returncode = process.wait()
                if marker_seen:
                    return
                if returncode:
                    raise RuntimeError(
                        f"remote system update failed ({returncode})")
                raise RuntimeError(
                    "remote update ended without the completion marker")

            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            marker_input = marker_window + chunk
            if (not marker_seen and
                    SYSTEM_INSTALLED_MARKER in marker_input):
                marker_seen = True
                detach_at = time.monotonic() + REBOOT_GRACE_SECONDS
            marker_window = marker_input[-len(SYSTEM_INSTALLED_MARKER):]

        # The image is fully written. Only the obsolete SSH transport is
        # stopped here; the five-second grace period lets the board execute
        # its kernel-level reboot first.
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        print("Firmware write completed; detached from the rebooting board.")
    finally:
        selector.close()
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--user", default="root")
    parser.add_argument("--port", type=int, default=22)
    subparsers = parser.add_subparsers(dest="operation", required=True)
    subparsers.add_parser("status", help="show the device update status")
    check_parser = subparsers.add_parser("check", help="validate a package on the device")
    check_parser.add_argument("package")
    update_parser = subparsers.add_parser(
        "update", help="install a complete BOOT and ext4 rootfs package")
    update_parser.add_argument("package")
    update_parser.add_argument("--reboot", action="store_true")
    args = parser.parse_args()

    try:
        if args.operation == "status":
            return remote(args, "openwifi-fw-update", "status")

        copy_inputs(args)
        command = "check" if args.operation == "check" else "install"
        if args.operation == "update" and args.reboot:
            command += " --reboot"
        remote_command = (
            f"sh {REMOTE_UPDATER} {command} {REMOTE_PACKAGE}; "
            "rc=$?; "
            f"rm -f {REMOTE_PACKAGE} {REMOTE_UPDATER}; "
            "[ $rc -eq 0 ] || exit $rc"
        )
        if args.operation == "update" and args.reboot:
            print("Installing complete system firmware; the board will reboot "
                  "immediately after the rootfs write...", flush=True)
            stream_reboot_update(args, remote_command)
            return 0
        return remote(args, remote_command)
    except (OSError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
