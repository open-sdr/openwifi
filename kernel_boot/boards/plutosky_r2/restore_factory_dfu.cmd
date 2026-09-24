@echo off
:: SPDX-FileCopyrightText: 2026 Fu Wenbo
:: SPDX-License-Identifier: AGPL-3.0-or-later
setlocal
cd /d "%~dp0factory_recovery"
dfu-util.exe -d 0456:b673,0456:b674 -D pluto.dfu -a firmware.dfu
endlocal
pause
