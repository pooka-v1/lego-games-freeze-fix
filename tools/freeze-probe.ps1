# Samples what the game and the rest of the PC are doing, to find out what happens during a freeze.
# Usage: start it, then launch the game. Toggle Scroll Lock whenever a freeze starts (it is logged).
# Stop with Ctrl+C. Output: freeze-probe.csv next to this script.
param(
    [string]$GameName = "LEGOStarWarsSaga",
    [int]$IntervalMs = 250
)

Add-Type -AssemblyName System.Windows.Forms
# Decimal point, not comma: with a Spanish locale "12,5" would split one CSV column into two.
[System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::InvariantCulture
$outFile = Join-Path $PSScriptRoot "freeze-probe.csv"
$cores = [Environment]::ProcessorCount
"time,scrollLock,gameCpuPct,busiestThreadCpuPct,mainThreadState,mainThreadWait,gameWorkingSetMB," +
    "gameThreads,totalCpuPct,top1,top2,top3" | Set-Content $outFile -Encoding utf8

$prevProc = @{}
$prevThread = @{}
$prevStamp = Get-Date
Write-Host "Waiting for $GameName... (Ctrl+C to stop)"

while ($true) {
    Start-Sleep -Milliseconds $IntervalMs
    $now = Get-Date
    $elapsedMs = ($now - $prevStamp).TotalMilliseconds
    $prevStamp = $now

    # CPU delta per process since the previous sample, as % of the whole machine
    $usage = @()
    foreach ($p in Get-Process) {
        try { $cpu = $p.TotalProcessorTime.TotalMilliseconds } catch { continue }
        $key = $p.Id
        if ($prevProc.ContainsKey($key)) {
            $pct = 100 * ($cpu - $prevProc[$key]) / ($elapsedMs * $cores)
            if ($pct -gt 0) { $usage += [pscustomobject]@{ Name = $p.ProcessName; Pct = $pct } }
        }
        $prevProc[$key] = $cpu
    }
    $total = ($usage | Measure-Object Pct -Sum).Sum
    $others = $usage | Where-Object { $_.Name -ne $GameName -and $_.Name -ne "Idle" } |
        Sort-Object Pct -Descending | Select-Object -First 3
    $topText = @($others | ForEach-Object { "{0} {1:N1}" -f $_.Name, $_.Pct })
    while ($topText.Count -lt 3) { $topText += "" }

    $game = Get-Process -Name $GameName -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $game) { continue }

    # Per-thread view: the busiest thread tells whether the game spins or sleeps during a freeze;
    # the main (first) thread's wait reason tells what it is waiting for.
    $busiest = 0.0
    foreach ($t in $game.Threads) {
        try { $tcpu = $t.TotalProcessorTime.TotalMilliseconds } catch { continue }
        if ($prevThread.ContainsKey($t.Id)) {
            $tpct = 100 * ($tcpu - $prevThread[$t.Id]) / $elapsedMs
            if ($tpct -gt $busiest) { $busiest = $tpct }
        }
        $prevThread[$t.Id] = $tcpu
    }
    $main = $game.Threads | Sort-Object StartTime | Select-Object -First 1
    $mainState = $main.ThreadState
    $mainWait = if ($mainState -eq "Wait") { $main.WaitReason } else { "" }
    $gamePct = ($usage | Where-Object Name -eq $GameName | Measure-Object Pct -Sum).Sum
    $scroll = [System.Windows.Forms.Control]::IsKeyLocked([System.Windows.Forms.Keys]::Scroll)

    $line = "{0},{1},{2:N1},{3:N1},{4},{5},{6:N0},{7},{8:N1},{9},{10},{11}" -f
        $now.ToString("HH:mm:ss.fff"), [int]$scroll, $gamePct, $busiest, $mainState, $mainWait,
        ($game.WorkingSet64 / 1MB), $game.Threads.Count, $total, $topText[0], $topText[1], $topText[2]
    Add-Content $outFile $line
}
