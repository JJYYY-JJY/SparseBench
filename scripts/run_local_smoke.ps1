Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build-nmake"
$Exe = Join-Path $BuildDir "sparsebench_spmv.exe"
$TinyData = Join-Path $RepoRoot "data\tiny"
$OutDir = Join-Path $RepoRoot "results"
$VsDevShell = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\Launch-VsDevShell.ps1"

Write-Host "SparseBench++ local smoke"
Write-Host "Repo root: $RepoRoot"
Write-Host "Build dir:  $BuildDir"
Write-Host "Binary:     $Exe"
Write-Host "Tiny data:  $TinyData"
Write-Host "Output dir: $OutDir"

if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    if (-not (Test-Path $VsDevShell)) {
        throw "MSVC developer shell was not found at $VsDevShell"
    }
    & $VsDevShell -Arch amd64 -HostArch amd64
}

cmake -S "$RepoRoot" -B "$BuildDir" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BuildDir"
ctest --test-dir "$BuildDir" --output-on-failure

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
& $Exe (Join-Path $TinyData "diag5.mtx") --threads 1 --repeat 10 --out (Join-Path $OutDir "local_diag5_t1.csv")
& $Exe (Join-Path $TinyData "diag5.mtx") --threads 2 --repeat 10 --out (Join-Path $OutDir "local_diag5_t2.csv")
