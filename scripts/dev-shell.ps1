# scripts/dev-shell.ps1
#
# Enter the Visual Studio developer shell (loads cl.exe, the Windows SDK
# linker paths, cmake, ninja, etc.) in the current directory, then drop into
# an interactive prompt where `cmake --preset debug-windows` will Just Work.
#
# Usage (from any PowerShell, in the repo root or anywhere):
#   .\scripts\dev-shell.ps1
#
# Optional: pass a command to execute and exit:
#   .\scripts\dev-shell.ps1 -Run "cmake --preset debug-windows; cmake --build --preset debug-windows"

[CmdletBinding()]
param(
    [string]$Run,
    [ValidateSet('amd64','x86','arm64')]
    [string]$Arch = 'amd64'
)

$ErrorActionPreference = 'Stop'

# --- Locate vswhere ---------------------------------------------------------
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    $vswhere = Join-Path $env:ProgramFiles 'Microsoft Visual Studio\Installer\vswhere.exe'
}
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Is Visual Studio installed? (looked under both Program Files and Program Files (x86))"
}

# --- Find the newest VS install that has the C++ workload -------------------
$vsInstall = & $vswhere -latest -prerelease `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsInstall) {
    throw "No Visual Studio install with the 'Desktop development with C++' workload was found. Install it via the VS Installer."
}

$launchScript = Join-Path $vsInstall 'Common7\Tools\Launch-VsDevShell.ps1'
if (-not (Test-Path $launchScript)) {
    throw "Launch-VsDevShell.ps1 not found at $launchScript. The VS install may be incomplete."
}

# --- Enter the dev shell (mutates current PS session env) -------------------
Write-Host ">> Entering VS Dev Shell for $vsInstall ($Arch)" -ForegroundColor Cyan
& $launchScript -Arch $Arch -SkipAutomaticLocation | Out-Null

# Bring the user back to the repo root regardless of where they invoked from.
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot
Write-Host ">> CWD: $repoRoot" -ForegroundColor Cyan

if ($Run) {
    Write-Host ">> Running: $Run" -ForegroundColor Cyan
    Invoke-Expression $Run
    exit $LASTEXITCODE
}

Write-Host ">> Dev shell ready. Try:  cmake --preset debug-windows" -ForegroundColor Green
