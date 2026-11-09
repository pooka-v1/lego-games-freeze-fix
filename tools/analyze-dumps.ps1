# Prints where the game's main thread is stuck in each dump taken by freeze-dump.ps1.
# Needs WinDbg (winget install Microsoft.WinDbg). The first run downloads Microsoft symbols to symbols\.
# -AllThreads: full report (CPU time per thread + every thread's stack) for each dump, saved as .txt
# files in dumps\analysis\.
param(
    [string]$DumpDir = (Join-Path $PSScriptRoot "dumps"),
    [switch]$AllThreads
)

# The game is 32-bit, so the x86 debugger. WinDbg from the Store can only be launched through this
# alias, not through its real path under WindowsApps (access denied).
$cdb = Join-Path $env:LOCALAPPDATA "Microsoft\WindowsApps\cdbX86.exe"
if (-not (Test-Path $cdb)) { throw "cdbX86.exe not found. Install WinDbg: winget install Microsoft.WinDbg" }
$symbols = "srv*$(Join-Path $PSScriptRoot 'symbols')*https://msdl.microsoft.com/download/symbols"

$analysisDir = Join-Path $DumpDir "analysis"
if ($AllThreads) { New-Item -ItemType Directory -Force $analysisDir | Out-Null }

foreach ($dump in Get-ChildItem $DumpDir -Filter *.dmp) {
    if ($AllThreads) {
        $report = Join-Path $analysisDir ($dump.BaseName + ".txt")
        & $cdb -z $dump.FullName -y $symbols -c '!runaway 7; ~*k 25; q' 2>&1 | Out-File -Encoding utf8 $report
        Write-Host "$($dump.Name) -> $report"
        continue
    }
    # Main thread only: its first frames outside the generic Windows wait/COM plumbing.
    $frames = & $cdb -z $dump.FullName -y $symbols -c '~0k 20; q' 2>&1 |
        Select-String -Pattern '^[0-9a-f]{8} [0-9a-f]{8}\s+(\S+)' |
        ForEach-Object { $_.Matches[0].Groups[1].Value } |
        Where-Object { $_ -notmatch '^(win32u|user32|combase|rpcrt4|ntdll|KERNELBASE)!' } |
        Select-Object -First 3
    "{0}: {1}" -f $dump.Name, ($frames -join ' <- ')
}
