<#
.SYNOPSIS
    Play a recorded .bin HOST stream into CigiStarter, by hand.

.DESCRIPTION
    The .bin captures beside this repository are the real HOST's own downstream
    stream -- the datagrams it sent the reference IG -- so replaying one into a
    bench CigiStarter is the closest this PC gets to the rig with no HOST and no
    emulator in the loop: the entities move in Prepar3D, the HOST's own eye flies
    the picture, and the HAT/HOT answers come off the local sim's terrain, all
    from a file.

    This is the operator's version of what tools\test-simlink-e2e.ps1 does with
    assertions attached: it reads the file, sends each datagram at the pace it
    was recorded at, and prints what it is doing.  Nothing is checked and nothing
    is required to pass -- stop it whenever, with Ctrl+C.

    CigiStarter is started for you in its bench configuration (loopback, ports
    8010/8011) unless -NoStart is given, and stopped again on the way out unless
    -KeepRunning is.  To drive an IG that is already running on the deployment
    ports instead, pass -IgPort 8000 -NoStart.

    The recording is the HOST side only: CigiStarter still sends its SOFs, but
    nothing answers them here, so flow control holds the stream back after the
    first frame.  That affects nothing -- the entities and the eye come off the
    replay, which is what moves the picture.

.PARAMETER Bin
    The capture to play.  Default: the newest .bin beside this repository, which
    is where the recordings live (E:\03_project\Super Puma\Visual\Local\).

.PARAMETER Speed
    Playback speed.  1 is the pace it was recorded at; 4 is four times as fast,
    0.25 is slow enough to watch a single manoeuvre.

.PARAMETER IgPort
    The port CigiStarter listens on.  8010, the bench default.

.PARAMETER HostPort
    The port CigiStarter sends its SOFs to.  8011, the bench default.

.PARAMETER CigiStarterExe
    The IG to start.  Default: the Release build in this repository.

.PARAMETER NoStart
    Do not start CigiStarter -- drive one that is already running.

.PARAMETER KeepRunning
    Leave the CigiStarter this script started running when the replay ends.
    Useful for studying the final pose in the sim.

.PARAMETER Loop
    Play the capture over and over until Ctrl+C, instead of once.

.PARAMETER MaxSeconds
    Stop after this much wall-clock time.  Unset means the whole capture once,
    or -- with -Loop -- until Ctrl+C.

.EXAMPLE
    .\tools\play-bin.ps1
    .\tools\play-bin.ps1 'E:\03_project\Super Puma\Visual\Local\hlm.bin' -Speed 4
    .\tools\play-bin.ps1 -Loop -KeepRunning
    .\tools\play-bin.ps1 'hlm.bin' -IgPort 8000 -NoStart
#>
[CmdletBinding()]
param(
    [string] $Bin = '',
    [ValidateRange(0.01, 1000.0)] [double] $Speed = 1.0,
    [int]    $IgPort = 8010,
    [int]    $HostPort = 8011,
    [string] $CigiStarterExe = '',
    [switch] $NoStart,
    [switch] $KeepRunning,
    [switch] $Loop,
    [double] $MaxSeconds = 0
)

$ErrorActionPreference = 'Stop'

#-----------------------------------------------------------------------------
# The recording: repeating little-endian [uint32 deltaMicros][uint16 len][len
# payload bytes].  Re-derived here from cigi.h's own description of the format
# rather than shared with the code that reads it, for the same reason the e2e
# harness re-derives it: a harness that shares an offset with the code cannot
# catch a wrong offset.  A truncated tail simply ends the file.
#-----------------------------------------------------------------------------
function Read-BinFile([string]$path) {
    $b = [IO.File]::ReadAllBytes($path)
    $recs = New-Object System.Collections.Generic.List[object]
    $o = 0
    while ($o + 6 -le $b.Length) {
        $delta = [BitConverter]::ToUInt32($b, $o)
        $len   = [BitConverter]::ToUInt16($b, $o + 4)
        $o += 6
        if ($o + $len -gt $b.Length) { break }
        $payload = New-Object byte[] $len
        if ($len -gt 0) { [Array]::Copy($b, $o, $payload, 0, $len) }
        $o += $len
        $recs.Add([pscustomobject]@{ Delta = $delta; Payload = $payload })
    }
    return $recs
}

# What one datagram carries, for the header line: the CIGI messages are runs of
# [id][size][body], so counting ids is enough to say whose sortie this is.
function Get-MessageHistogram([object[]]$records) {
    $h = @{}
    foreach ($r in $records) {
        $p = $r.Payload
        $i = 0
        while ($i + 2 -le $p.Length) {
            $id = $p[$i]; $size = $p[$i + 1]
            if ($size -lt 2 -or ($i + $size) -gt $p.Length) { $i++; continue }
            if (-not $h.ContainsKey($id)) { $h[$id] = 0 }
            $h[$id]++
            $i += $size
        }
    }
    return $h
}

# The port CigiStarter is bound to, so the script can tell "already running"
# from "start one".  Both the IG's receive port and the HOST's are checked: a
# deployment CigiStarter is found by either.
function Get-UdpOwner([int]$Port) {
    $line = & netstat -ano -p UDP 2>$null | Select-String -Pattern (":{0}\s" -f $Port) | Select-Object -First 1
    if (-not $line) { return 0 }
    $parts = ($line.Line -split '\s+') | Where-Object { $_ -ne '' }
    if ($parts.Count -lt 4) { return 0 }
    return [int]$parts[-1]
}

function Stop-CigiStarter {
    $procs = @(Get-Process CigiStarter -ErrorAction SilentlyContinue)
    if ($procs.Count -eq 0) { return }
    # Its EXIT button is the one way out: the title-bar X is swallowed by design,
    # and EXIT retires the entity rows so the add-on takes the objects back out.
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class PlayBinW {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
"@ -ErrorAction SilentlyContinue
    foreach ($p in $procs) { [void][PlayBinW]::PostMessage($p.MainWindowHandle, 0x0111, [IntPtr]1003, [IntPtr]::Zero) }
    $by = (Get-Date).AddSeconds(8)
    while ((Get-Date) -lt $by) {
        if (@(Get-Process CigiStarter -ErrorAction SilentlyContinue).Count -eq 0) { return }
        Start-Sleep -Milliseconds 200
    }
    foreach ($p in @(Get-Process CigiStarter -ErrorAction SilentlyContinue)) {
        Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue
    }
}

#-----------------------------------------------------------------------------
# Resolve the capture
#-----------------------------------------------------------------------------
if (-not $Bin) {
    $repo = Split-Path -Parent $PSScriptRoot
    $captures = Split-Path -Parent $repo
    $newest = Get-ChildItem -Path $captures -Filter *.bin -File -ErrorAction SilentlyContinue |
              Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $newest) {
        throw "no -Bin given and no .bin capture found in $captures"
    }
    $Bin = $newest.FullName
}
if (-not (Test-Path -LiteralPath $Bin)) {
    # A bare name means the capture folder beside the repository.
    $repo = Split-Path -Parent $PSScriptRoot
    $candidate = Join-Path (Split-Path -Parent $repo) $Bin
    if (Test-Path -LiteralPath $candidate) { $Bin = $candidate }
    else { throw "no such capture: $Bin" }
}
$Bin = (Resolve-Path -LiteralPath $Bin).Path

if (-not $CigiStarterExe) {
    $repo = Split-Path -Parent $PSScriptRoot
    $CigiStarterExe = Join-Path $repo 'x64\Release\CigiStarter.exe'
}

$records = Read-BinFile $Bin
if ($records.Count -eq 0) { throw "no records in $Bin" }

$span = 0.0
foreach ($r in $records) { $span += $r.Delta / 1000000.0 }
$hist = Get-MessageHistogram $records

Write-Host ''
Write-Host ("  capture     {0}" -f $Bin)
Write-Host ("  {0} datagrams, {1:N1} s of HOST time, {2}" -f `
    $records.Count, $span, ((Get-Item -LiteralPath $Bin).LastWriteTime.ToString('yyyy-MM-dd HH:mm')))
$ids = ($hist.Keys | Sort-Object | ForEach-Object { "{0}x{1}" -f $_, $hist[$_] }) -join ' '
Write-Host ("  messages    {0}" -f $ids)
Write-Host '              (1 IG Control, 2 Entity Control, 16 View Control = the HOST''s own eye)'

#-----------------------------------------------------------------------------
# The IG: start one on the bench ports if there is not one already
#-----------------------------------------------------------------------------
$startedHere = $false
if (-not $NoStart) {
    if ((Get-UdpOwner $IgPort) -ne 0) {
        Write-Host ("`n  CigiStarter is already listening on udp {0}; driving it" -f $IgPort)
    } else {
        if (-not (Test-Path -LiteralPath $CigiStarterExe)) {
            throw "no CigiStarter to start at $CigiStarterExe -- build it, or pass -NoStart"
        }
        Write-Host ("`n  starting    {0} (bench: loopback, udp {1}/{2})" -f $CigiStarterExe, $IgPort, $HostPort)
        Start-Process -FilePath $CigiStarterExe -WorkingDirectory (Split-Path -Parent $CigiStarterExe) `
            -ArgumentList @('--host-ip', '127.0.0.1', '--ig-ip', '127.0.0.1',
                            '--ig-port', "$IgPort", '--host-port', "$HostPort")
        $startedHere = $true
        $by = (Get-Date).AddSeconds(15)
        while ((Get-UdpOwner $IgPort) -eq 0 -and (Get-Date) -lt $by) { Start-Sleep -Milliseconds 200 }
        if ((Get-UdpOwner $IgPort) -eq 0) { throw "CigiStarter did not come up on udp $IgPort" }
    }
} else {
    if ((Get-UdpOwner $IgPort) -eq 0) {
        Write-Host ("`n  warning: nothing is listening on udp {0}; the replay will go nowhere" -f $IgPort)
    }
}

#-----------------------------------------------------------------------------
# Play it
#-----------------------------------------------------------------------------
$udp = New-Object System.Net.Sockets.UdpClient
$udp.Connect('127.0.0.1', $IgPort)

$rounds = 0
try {
    do {
        $rounds++
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $target = 0.0
        $sent = 0
        $lastReport = 0.0

        foreach ($rec in $records) {
            $target += ($rec.Delta / 1000000.0) / $Speed
            if ($rec.Payload.Length -gt 0) { [void]$udp.Send($rec.Payload, $rec.Payload.Length) }
            $sent++

            $elapsed = $sw.Elapsed.TotalSeconds
            if ($elapsed -lt $target) {
                $wait = $target - $elapsed
                if ($wait -gt 0.002) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
            }
            if (($sw.Elapsed.TotalSeconds - $lastReport) -ge 5.0) {
                $lastReport = $sw.Elapsed.TotalSeconds
                $pct = 100.0 * $sent / $records.Count
                Write-Host ("  playing     {0,6:N1} s of {1:N1} s  ({2,3:N0}%, {3} datagrams)" -f `
                    ($target * $Speed), $span, $pct, $sent)
            }
            if ($MaxSeconds -gt 0 -and $sw.Elapsed.TotalSeconds -ge $MaxSeconds) { break }
        }

        Write-Host ("  played      {0} datagrams, {1:N1} s of HOST time, in {2:N1} s wall clock" -f `
            $sent, ($target * $Speed), $sw.Elapsed.TotalSeconds)
    } while ($Loop)
} finally {
    $udp.Close()

    if ($startedHere -and -not $KeepRunning) {
        Write-Host '  stopping    CigiStarter (EXIT), so it retires the entities'
        Stop-CigiStarter
    } elseif ($KeepRunning) {
        Write-Host '  CigiStarter left running (-KeepRunning); the sim holds the last pose'
    }

    Write-Host ''
    Write-Host '  In Prepar3D: the HOST''s entities are being drawn and the picture is flown'
    Write-Host '  from the HOST''s own eye if PdkSimLink.ini has camera=<entity id> set.'
    Write-Host '  CigiStarter''s window says which: "SimLink : N entities applied, N objects'
    Write-Host '  in the sim, eye from the HOST".  Nothing there means the add-on is not'
    Write-Host '  loaded, and CigiStarter''s window says why.'
    Write-Host ''
}
