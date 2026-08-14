#!/usr/bin/env python3
"""Set a generated Linux DTS console path without editing upstream DTS files."""

from __future__ import annotations

import pathlib
import re
import sys


if len(sys.argv) != 3:
    raise SystemExit(f"usage: {sys.argv[0]} <dts> <serial-path>")

dts = pathlib.Path(sys.argv[1])
serial_path = sys.argv[2]
text = dts.read_text(encoding="utf-8")
pattern = re.compile(
    r'^(?P<indent>[ \t]*)(?:linux,)?stdout-path\s*=\s*"[^"]*"\s*;',
    re.MULTILINE,
)
text, count = pattern.subn(
    rf'\g<indent>stdout-path = "{serial_path}:115200n8";', text, count=1
)
if count != 1:
    raise SystemExit(f"error: expected one stdout-path property in {dts}, found {count}")
dts.write_text(text, encoding="utf-8")
