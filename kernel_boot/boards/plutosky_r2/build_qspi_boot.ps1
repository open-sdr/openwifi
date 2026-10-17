# SPDX-FileCopyrightText: 2026 Fu Wenbo
# SPDX-License-Identifier: AGPL-3.0-or-later

param(
    [Parameter(Mandatory = $true)] [string] $Fsbl,
    [Parameter(Mandatory = $true)] [string] $UBoot,
    [Parameter(Mandatory = $true)] [string] $OutputDirectory,
    [Parameter(Mandatory = $true)] [string] $Bootgen,
    [int64] $MaximumImageBytes = 1MB
)

$ErrorActionPreference = 'Stop'

function Convert-ToBootgenPath([string] $Path) {
    return (Resolve-Path -LiteralPath $Path).Path.Replace('\', '/')
}

$fsblPath = Convert-ToBootgenPath $Fsbl
$ubootPath = Convert-ToBootgenPath $UBoot
$bootgenPath = Convert-ToBootgenPath $Bootgen
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$bif = Join-Path $OutputDirectory 'plutosky_r2_qspi_boot.bif'
$image = Join-Path $OutputDirectory 'plutosky_r2_qspi_BOOT.BIN'
@"
the_ROM_image:
{
  [bootloader] $fsblPath
  $ubootPath
}
"@ | Set-Content -NoNewline -Encoding Ascii $bif

& $bootgenPath -arch zynq -image $bif -w -o i $image
if ($LASTEXITCODE -ne 0) {
    throw "Bootgen failed with exit code $LASTEXITCODE"
}

$imageInfo = Get-Item -LiteralPath $image
if ($imageInfo.Length -gt $MaximumImageBytes) {
    throw "QSPI boot image is $($imageInfo.Length) bytes; it exceeds the allowed $MaximumImageBytes-byte range."
}

Get-FileHash -Algorithm SHA256 $image
$imageInfo
