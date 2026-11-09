# Takes a minidump of the game every time Scroll Lock is pressed, without stealing focus from it.
# Each press writes 2 dumps, 3 s apart (shows whether the threads stay stuck in the same place).
# Usage: start it, launch the game, press Scroll Lock once while playing normally (baseline) and
# once at the start of each freeze. A short beep confirms each capture. Stop with Ctrl+C.
# Output: dumps\ next to this script.
param(
    [string]$GameName = "LEGOStarWarsSaga"
)

Add-Type -AssemblyName System.Windows.Forms
$procdump = Join-Path $PSScriptRoot "procdump\procdump.exe"
$dumpDir = Join-Path $PSScriptRoot "dumps"
New-Item -ItemType Directory -Force $dumpDir | Out-Null

function Get-ScrollLock { [System.Windows.Forms.Control]::IsKeyLocked([System.Windows.Forms.Keys]::Scroll) }

# Default minidump: thread stacks + module list, enough to see what each thread waits on.
# Started async so a slow write never misses the next key press. No -n/-s: with them procdump
# waits for a trigger instead of dumping right away.
function Write-GameDump([int]$processId) {
    Start-Process -FilePath $procdump -WindowStyle Hidden -ArgumentList "-accepteula $processId `"$dumpDir`""
}

Write-Host "Waiting for $GameName... (Ctrl+C to stop)"
$lastState = Get-ScrollLock
$captures = 0
$followUpAt = $null
$followUpPid = 0

while ($true) {
    Start-Sleep -Milliseconds 100

    if ($followUpAt -and (Get-Date) -ge $followUpAt) {
        Write-GameDump $followUpPid
        $followUpAt = $null
    }

    $state = Get-ScrollLock
    if ($state -eq $lastState) { continue }
    $lastState = $state

    $game = Get-Process -Name $GameName -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $game) { Write-Host "Scroll Lock pressed, but $GameName is not running"; continue }

    $captures++
    Write-Host ("[{0}] capture #{1}" -f (Get-Date).ToString("HH:mm:ss"), $captures)
    Write-GameDump $game.Id
    $followUpAt = (Get-Date).AddSeconds(3)
    $followUpPid = $game.Id
    [Console]::Beep(1000, 150)
}
