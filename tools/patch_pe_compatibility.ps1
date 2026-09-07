param(
    [Parameter(Mandatory=$true)]
    [string]$Path
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Path)) {
    throw "PE file not found: $Path"
}

[byte[]]$bytes = [System.IO.File]::ReadAllBytes($Path)
if ($bytes.Length -lt 0x100) {
    throw "File too small to be a PE image: $Path"
}
if ($bytes[0] -ne 0x4D -or $bytes[1] -ne 0x5A) {
    throw "Missing MZ signature: $Path"
}

$peOffset = [System.BitConverter]::ToInt32($bytes, 0x3C)
if ($peOffset -lt 0 -or ($peOffset + 0x60) -ge $bytes.Length) {
    throw "Invalid PE header offset: $Path"
}
if ($bytes[$peOffset] -ne 0x50 -or $bytes[$peOffset + 1] -ne 0x45 -or
    $bytes[$peOffset + 2] -ne 0 -or $bytes[$peOffset + 3] -ne 0) {
    throw "Missing PE signature: $Path"
}

$coff = $peOffset + 4
$machine = [System.BitConverter]::ToUInt16($bytes, $coff)
if ($machine -ne 0x014C) {
    throw ("Expected IMAGE_FILE_MACHINE_I386 (0x014C), got 0x{0:X4}" -f $machine)
}

$optional = $coff + 20
$magic = [System.BitConverter]::ToUInt16($bytes, $optional)
if ($magic -ne 0x010B) {
    throw ("Expected PE32 optional header (0x010B), got 0x{0:X4}" -f $magic)
}

$subsystem = [System.BitConverter]::ToUInt16($bytes, $optional + 0x44)
if ($subsystem -ne 2) {
    throw ("Expected IMAGE_SUBSYSTEM_WINDOWS_GUI (2), got {0}" -f $subsystem)
}

function Set-U16([byte[]]$Data, [int]$Offset, [UInt16]$Value) {
    $Data[$Offset] = [byte]($Value -band 0xFF)
    $Data[$Offset + 1] = [byte](($Value -shr 8) -band 0xFF)
}

# Retail MapEdit.exe was linked by VC6 with both PE OS and subsystem versions 4.0.
# MSVC v143 refuses WINDOWS|x86 subsystem versions below 5.01, so exact legacy
# loader metadata is restored after link without changing any C++ runtime logic.
Set-U16 $bytes ($optional + 0x28) 4  # MajorOperatingSystemVersion
Set-U16 $bytes ($optional + 0x2A) 0  # MinorOperatingSystemVersion
Set-U16 $bytes ($optional + 0x30) 4  # MajorSubsystemVersion
Set-U16 $bytes ($optional + 0x32) 0  # MinorSubsystemVersion

[System.IO.File]::WriteAllBytes($Path, $bytes)

Write-Host ("MapEdit PE compatibility header patched: OS=4.0 SubsystemVersion=4.0 -> {0}" -f $Path)
