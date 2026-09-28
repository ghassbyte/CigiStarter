<#
.SYNOPSIS
    Prove the entity writer works end to end on this PC: replay a recorded HOST
    stream into CigiStarter, and read PdkSimLink's own report out of the running
    Prepar3D to show the sim created the HOST's entities and is holding them on
    the poses the HOST commanded.

.DESCRIPTION
    The contract test (tools\test-simlink.ps1) covers everything either side of
    PdkSimLink's shared region without a simulator, and the HAT/HOT bench test
    covers CigiStarter's answers on the wire. What neither can reach is the
    middle: does the pose CigiStarter publishes actually become a sim object in
    Prepar3D, on the pose that was published? That is what this is for, and it is
    the one step that would otherwise be tried for the first time on the rig.

    It needs no HOST and no emulator. The .bin captures beside this repository
    *are* the real HOST's own downstream stream -- the same datagrams the HOST
    sent the reference IG -- so replaying one at CigiStarter is as close to the
    rig as this PC can get with nothing else in the loop. The .bin framing and
    the Entity Control layout are re-derived here from cigi.h and SimLinkIpc.h
    rather than trusted, for the same reason the wire test re-derives
    TerrainProbeIpc.h: a harness that shares an offset with the code cannot
    catch a wrong offset.

    The add-on half is read out of the shared region the way an operator would
    read a black box, because it is the only witness: PdkSimLink creates its
    objects through the PDK, and nothing outside the sim process can enumerate
    them. Its status words say whether it has the object manager, whether the
    sim's frame event is being delivered, how many objects it created, how many
    it positioned on the last tick, and whether any create or SetPosition call
    failed. Every one of those is checked here.

    The read-back
      SetPosition succeeding says the sim accepted the pose, not that it is
      still there a frame later. The object this build creates is the sim's own
      and has its own simulation between writes, so it can be pulled off the
      commanded pose -- which is the whole reason the follow-up is a custom
      ISimObject with no flight model. PdkSimLink reads each object's actual pose
      back (IBaseObjectV400::GetPosition) and publishes it, and this test
      compares that against the pose that was written and reports the drift at
      its worst over the run.

      Measured on the bench against the AS332 (2026-09-28): in the air the
      object tracks the command to about 2 cm of position and 0.1 deg of
      attitude, on a static pose and on a moving one -- the native flight model
      does not fight the writer. The one large number this test can show is the
      sim's own ground: an object is clamped to the terrain under it, so a
      commanded altitude below the sim's terrain is held on the surface and
      reads as drift (up to ~277 m in this capture, whose command goes below
      the sim's ground). That is why the drift is reported rather than failed
      on, and why the commanded altitude is printed beside it.

    Prepar3D must be running with the current PdkSimLink.dll loaded, and the
    DLL must be the build in this tree: the add-on is loaded once, at sim
    startup, and a stale copy would test the wrong code. Both are checked before
    anything is started, and the run says what to do about it.

    The objects the test creates use whatever container title PdkSimLink.ini
    names (title=shipping_container_sm by default while bringing the link up),
    so a few containers will appear in the sim while it runs. CigiStarter is
    stopped at the end, which retires every row and takes the objects back out.

.PARAMETER Bin
    The recorded HOST stream to replay. Default hlm.bin beside the repository,
    an 11.6 s capture with two driven entities -- short enough to run at
    recorded speed.

.PARAMETER IgPort
    CigiStarter's receive port. Default 8010, because this machine's Prepar3D
    holds 8000 for its own CIGI.

.PARAMETER HostPort
    The port CigiStarter sends its SOF to. Default 8011.

.PARAMETER Speed
    Replay rate, as a multiple of the capture's own timing. 1.0 is the HOST's
    real cadence; raise it to get through a long capture quickly.

.PARAMETER MaxSeconds
    Stop replaying after this long regardless, so a large capture cannot make
    the test run for minutes. Default 30.

.PARAMETER CigiStarterExe
    CigiStarter to start. Defaults to x64\Release\CigiStarter.exe.

.PARAMETER KeepRunning
    Leave CigiStarter running when the checks are done, so the sim can be looked
    at. The default stops it, which also removes the test's objects.

.EXAMPLE
    # the whole thing, ending in a PASS or FAIL verdict:
    .\tools\test-simlink-e2e.ps1

.EXAMPLE
    # a longer capture, hurried along, and leave the IG up to look at the sim:
    .\tools\test-simlink-e2e.ps1 -Bin '..\gnd-fly-bank.bin' -Speed 20 -KeepRunning
#>
[CmdletBinding()]
param(
    [string] $Bin = '',
    [int]    $IgPort     = 8010,
    [int]    $HostPort   = 8011,
    [double] $Speed      = 1.0,
    [int]    $MaxSeconds = 30,
    [string] $CigiStarterExe = '',
    [switch] $KeepRunning
)

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
if (-not $CigiStarterExe) { $CigiStarterExe = Join-Path $repo 'x64\Release\CigiStarter.exe' }
$builtDll = Join-Path $repo 'tools\PdkSimLink\bin\Release-v6\PdkSimLink.dll'
$addonDir = Join-Path $env:USERPROFILE 'Documents\Prepar3D v6 Add-ons\PdkSimLink'
$deployedDll = Join-Path $addonDir 'PdkSimLink.dll'
$mappingName = 'Local\CigiStarter.SimLink.v1'

if (-not $Bin) {
    $Bin = Join-Path (Split-Path -Parent $repo) 'hlm.bin'
    if (-not (Test-Path $Bin)) {
        $Bin = Join-Path (Split-Path -Parent $repo) 'gnd-fly-bank.bin'
    }
}

#-----------------------------------------------------------------------------
# The shared region, re-derived from SimLinkIpc.h
#
#   header   magic 0, version 4, capacity 8, simPid 12, alive 16, lastError 20,
#            ticks 24, frameTicks 28, objectsCreated 32, objectsRemoved 36,
#            createFails 40, setFails 44, applied 48, diag 52,
#            positionReads 56, getFails 60, driftSamples 64, heldValid 68,
#            driftMaxMeters 72 (f64), driftMaxDeg 80 (f64)
#   entities[64] at 88, each row 72 bytes: seq 0, entityId 4, enabled 8,
#            flags 12, state 16, lat 24, lon 32, alt 40, roll 48, pitch 56, yaw 64
#   held[64] at 4696, each row 80 bytes: seq 0, entityId 4, tick 8, valid 12,
#            lat 16, lon 24, alt 32, roll 40, pitch 48, yaw 56,
#            driftMeters 64, driftDeg 72
#-----------------------------------------------------------------------------
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class SimLinkShm {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] public static extern IntPtr OpenFileMapping(uint a, bool i, string n);
  [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr MapViewOfFile(IntPtr h, uint a, uint hi, uint lo, UIntPtr b);
  [DllImport("kernel32.dll", SetLastError=true)] public static extern bool CloseHandle(IntPtr h);
  public static int    I32(IntPtr v, int o) { return Marshal.ReadInt32(IntPtr.Add(v, o)); }
  public static long   I64(IntPtr v, int o) { return Marshal.ReadInt64(IntPtr.Add(v, o)); }
  public static double F64(IntPtr v, int o) { return BitConverter.Int64BitsToDouble(Marshal.ReadInt64(IntPtr.Add(v, o))); }
  public static uint   U32(IntPtr v, int o) { return (uint)((long)Marshal.ReadInt32(IntPtr.Add(v, o)) & 0xFFFFFFFFL); }
}
"@

$script:view = [IntPtr]::Zero
$script:mapHandle = [IntPtr]::Zero

function Open-SimLink {
    if ($script:view -ne [IntPtr]::Zero) { return $script:view }
    $h = [SimLinkShm]::OpenFileMapping(0x0004, $false, $mappingName)
    if ($h -eq [IntPtr]::Zero) { return [IntPtr]::Zero }
    $v = [SimLinkShm]::MapViewOfFile($h, 0x0004, 0, 0, [UIntPtr]::Zero)
    if ($v -eq [IntPtr]::Zero) { [void][SimLinkShm]::CloseHandle($h); return [IntPtr]::Zero }
    $script:mapHandle = $h
    $script:view = $v
    return $v
}

function Region-U32([int]$off) { return [SimLinkShm]::U32($script:view, $off) }
function Region-I32([int]$off) { return [SimLinkShm]::I32($script:view, $off) }
function Region-F64([int]$off) { return [SimLinkShm]::F64($script:view, $off) }

# The add-on's status words, decoded. `diag` the same way SimLinkIpc.h packs it.
function Get-WriterHealth {
    $d = [int64](Region-U32 52)
    return [pscustomobject]@{
        Magic           = Region-U32 0
        Version         = Region-U32 4
        Capacity        = Region-U32 8
        SimPid          = Region-U32 12
        Alive           = Region-I32 16
        LastError       = Region-U32 20
        Ticks           = Region-U32 24
        FrameTicks      = Region-U32 28
        ObjectsCreated  = Region-U32 32
        ObjectsRemoved  = Region-U32 36
        CreateFails     = Region-U32 40
        SetFails        = Region-U32 44
        Applied         = Region-U32 48
        Diag            = $d
        PositionReads   = Region-U32 56
        GetFails        = Region-U32 60
        DriftSamples    = Region-U32 64
        HeldValid       = Region-U32 68
        DriftMaxMeters  = Region-F64 72
        DriftMaxDeg     = Region-F64 80
        Diagnosed       = (($d -shr 31) -band 1) -eq 1
        HasManager      = (($d -shr 24) -band 1) -eq 1
        HasTick         = (($d -shr 25) -band 1) -eq 1
        NoTitle         = (($d -shr 26) -band 1) -eq 1
        OwnThreadTick   = (($d -shr 27) -band 1) -eq 1
        HasReadback     = (($d -shr 28) -band 1) -eq 1
        DiagCreateFails = [int](($d -shr 16) -band 0xFF)
        DiagSetFails    = [int](($d -shr 8)  -band 0xFF)
    }
}

# One entity row, read under its seqlock: a row caught mid-rewrite is not
# believed, exactly as the add-on reads it.
function Get-EntityRow([int]$slot) {
    $o = 88 + 72 * $slot
    for ($try = 0; $try -lt 200; $try++) {
        $before = Region-U32 $o
        if ($before -band 1) { Start-Sleep -Milliseconds 1; continue }
        $row = [pscustomobject]@{
            Slot      = $slot
            Seq       = $before
            EntityId  = Region-U32 ($o + 4)
            Enabled   = Region-U32 ($o + 8)
            Flags     = Region-U32 ($o + 12)
            State     = Region-U32 ($o + 16)
            Lat       = Region-F64 ($o + 24)
            Lon       = Region-F64 ($o + 32)
            Alt       = Region-F64 ($o + 40)
            Roll      = Region-F64 ($o + 48)
            Pitch     = Region-F64 ($o + 56)
            Yaw       = Region-F64 ($o + 64)
            PoseValid = ((Region-U32 ($o + 12)) -band 1) -ne 0
        }
        if ((Region-U32 $o) -eq $before) { return $row }
    }
    return $null
}

function Get-LiveRows {
    $rows = @()
    for ($i = 0; $i -lt 64; $i++) {
        $r = Get-EntityRow $i
        if ($null -ne $r -and $r.Enabled -ne 0) { $rows += $r }
    }
    return ,$rows
}

# One read-back row, as the add-on publishes it (held[]): the pose the sim's
# object was actually found at, and how far that was from the pose it was last
# written.  Read under its own seqlock, the same way the add-on reads the IG's
# rows.
function Get-HeldRow([int]$slot) {
    $o = 4696 + 80 * $slot
    for ($try = 0; $try -lt 200; $try++) {
        $before = Region-U32 $o
        if ($before -band 1) { Start-Sleep -Milliseconds 1; continue }
        $row = [pscustomobject]@{
            Slot         = $slot
            Seq          = $before
            EntityId     = Region-U32 ($o + 4)
            Tick         = Region-U32 ($o + 8)
            Valid        = Region-U32 ($o + 12)
            Lat          = Region-F64 ($o + 16)
            Lon          = Region-F64 ($o + 24)
            Alt          = Region-F64 ($o + 32)
            Roll         = Region-F64 ($o + 40)
            Pitch        = Region-F64 ($o + 48)
            Yaw          = Region-F64 ($o + 56)
            DriftMeters  = Region-F64 ($o + 64)
            DriftDeg     = Region-F64 ($o + 72)
        }
        if ((Region-U32 $o) -eq $before) { return $row }
    }
    return $null
}

function Get-HeldRows {
    $rows = @()
    for ($i = 0; $i -lt 64; $i++) {
        $r = Get-HeldRow $i
        if ($null -ne $r -and $r.Valid -ne 0) { $rows += $r }
    }
    return ,$rows
}

# Where is the container title the add-on will create from?  A title that is not
# in the installation is the one misconfiguration that makes a run look half
# successful -- the add-on loads, ticks, reports the object manager, and creates
# nothing.  The sim's own tree and the per-user add-on folder are searched; a
# title that is nowhere in either is reported rather than failed, because an
# add-on can register SimObjects from anywhere.
function Find-SimTitle([string]$Title, [string]$InstallRoot) {
    $roots = @(
        (Join-Path $InstallRoot 'SimObjects'),
        (Join-Path $env:USERPROFILE 'Documents\Prepar3D v6 Add-ons')
    )
    # Aircraft added to the sim as packages (the AS332 the rig flies is one) live
    # at a path registered in add-ons.cfg rather than under the install's own
    # SimObjects tree, so follow those package paths too -- otherwise a title
    # that is perfectly valid is reported as missing.
    $cfgs = @(
        (Join-Path $env:PROGRAMDATA 'Lockheed Martin\Prepar3D v6\add-ons.cfg'),
        (Join-Path $env:APPDATA     'Lockheed Martin\Prepar3D v6\add-ons.cfg')
    )
    foreach ($c in $cfgs) {
        if (-not (Test-Path $c)) { continue }
        foreach ($line in @(Get-Content $c -ErrorAction SilentlyContinue | Select-String -Pattern '^\s*PATH\s*=')) {
            $p = ($line.ToString() -replace '^\s*PATH\s*=\s*', '').Trim()
            if (-not $p) { continue }
            if (-not [IO.Path]::IsPathRooted($p)) { $p = Join-Path $InstallRoot $p }
            if (Test-Path $p) { $roots += $p }
        }
    }
    $pattern = ('(?im)^\s*title\s*=\s*"?{0}"?\s*$' -f [regex]::Escape($Title))
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $hit = Get-ChildItem $root -Recurse -Filter *.cfg -ErrorAction SilentlyContinue |
               Select-String -Pattern $pattern -List -ErrorAction SilentlyContinue |
               Select-Object -First 1
        if ($hit) { return $hit.Path }
    }
    return ''
}

#-----------------------------------------------------------------------------
# The recorded HOST stream, re-derived from cigi.h and the .bin format:
# repeating little-endian [uint32 deltaMicros][uint16 len][len payload bytes],
# each payload a run of [id(1)][size(1)][body...] CIGI messages.
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

# Walk one datagram's CIGI messages, as cigi::forEachMessage does: a frame that
# does not fit is skipped a byte at a time rather than ending the scan.
function Get-EntityControls([byte[]]$p) {
    $out = New-Object System.Collections.Generic.List[object]
    $i = 0
    while ($i + 2 -le $p.Length) {
        $id = $p[$i]; $size = $p[$i + 1]
        if ($size -lt 2 -or ($i + $size) -gt $p.Length) { $i++; continue }
        if ($id -eq 2 -and $size -ge 48) {
            $out.Add([pscustomobject]@{
                EntityId = [BitConverter]::ToUInt16($p, $i + 2)
                State    = $p[$i + 4] -band 0x03
                Roll     = [double]([BitConverter]::ToSingle($p, $i + 12))
                Pitch    = [double]([BitConverter]::ToSingle($p, $i + 16))
                Yaw      = [double]([BitConverter]::ToSingle($p, $i + 20))
                Lat      = [BitConverter]::ToDouble($p, $i + 24)
                Lon      = [BitConverter]::ToDouble($p, $i + 32)
                Alt      = [BitConverter]::ToDouble($p, $i + 40)
            })
        }
        $i += $size
    }
    return ,$out
}

# cigi::poseUsable: finite, on the globe, and not the all-zero "no position"
# placeholder. main.cpp publishes only usable poses, so a row can only ever
# carry one of these.
function Test-PoseUsable($e) {
    if ([double]::IsNaN($e.Lat) -or [double]::IsInfinity($e.Lat) -or
        [double]::IsNaN($e.Lon) -or [double]::IsInfinity($e.Lon) -or
        [double]::IsNaN($e.Alt) -or [double]::IsInfinity($e.Alt)) { return $false }
    if ($e.Lat -lt -90 -or $e.Lat -gt 90) { return $false }
    if ($e.Lon -lt -180 -or $e.Lon -gt 180) { return $false }
    return -not ($e.Lat -eq 0 -and $e.Lon -eq 0 -and $e.Alt -eq 0)
}

function Get-SlotOf([int]$id) { return $id -band 63 }

function Compare-Number([double]$got, [double]$want, [double]$tol) {
    return [math]::Abs($got - $want) -le $tol
}

#-----------------------------------------------------------------------------
# Reporting, in the same shape as the other bench scripts
#-----------------------------------------------------------------------------
$script:results = @()
$script:currentTest = ''

function Assert([string]$Check, [bool]$Ok, [string]$Detail = '') {
    $verdict = 'FAIL'
    if ($Ok) { $verdict = 'PASS' }
    $suffix = ''
    if ($Detail) { $suffix = "  ($Detail)" }
    Write-Host ("    [{0}] {1}{2}" -f $verdict, $Check, $suffix)
    $script:results += [pscustomobject]@{ Test = $script:currentTest; Result = $verdict; Check = $Check; Detail = $Detail }
}

function Run-Test([string]$Name, [scriptblock]$Body) {
    Write-Host ''
    Write-Host ("--- {0}" -f $Name)
    $script:currentTest = $Name
    try { & $Body }
    catch { Assert 'ran without throwing' $false $_.Exception.Message }
}

function Write-Verdict([string]$Text, [string]$Colour) {
    $line = ('  RESULT: {0}' -f $Text)
    $rule = '=' * $line.Length
    Write-Host ''
    Write-Host $rule -ForegroundColor $Colour
    Write-Host $line -ForegroundColor $Colour
    Write-Host $rule -ForegroundColor $Colour
}

#-----------------------------------------------------------------------------
# Bench pieces: CigiStarter, stopped the one way it provides (its EXIT button;
# its title-bar X is swallowed by design).
#-----------------------------------------------------------------------------
$script:startedCigi = $false

function Get-UdpOwner([int]$Port) {
    foreach ($line in @(netstat -ano -p udp | Select-String (":{0}\s" -f $Port))) {
        $parts = @($line.ToString().Trim() -split '\s+')
        if ($parts.Count -ge 4 -and $parts[1] -match (":{0}$" -f $Port)) { return [int]$parts[-1] }
    }
    return 0
}

function Stop-CigiStarter {
    $procs = @(Get-Process CigiStarter -ErrorAction SilentlyContinue)
    if ($procs.Count -eq 0) { return }
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class W {
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
"@ -ErrorAction SilentlyContinue
    foreach ($p in $procs) { [void][W]::PostMessage($p.MainWindowHandle, 0x0111, [IntPtr]1003, [IntPtr]::Zero) }
    $by = (Get-Date).AddSeconds(8)
    while (@(Get-Process CigiStarter -ErrorAction SilentlyContinue).Count -gt 0 -and (Get-Date) -lt $by) {
        Start-Sleep -Milliseconds 200
    }
    @(Get-Process CigiStarter -ErrorAction SilentlyContinue) | Stop-Process -Force -ErrorAction SilentlyContinue
}

#-----------------------------------------------------------------------------
# Preflight: everything about the sim half, reported before anything is started
#-----------------------------------------------------------------------------
Write-Host ''
Write-Host 'CigiStarter entity writer - local end-to-end test'
Write-Host ("  capture     {0}" -f $Bin)
Write-Host ("  link        IG {0} (CigiStarter)  <-  replay of the recorded HOST" -f $IgPort)
Write-Host ("  replay      {0}x real time, at most {1}s" -f $Speed, $MaxSeconds)

$hasP3d = $false
$dllLoaded = $false
$deployMatches = $false
$title = ''
try {
    if (-not (Test-Path $Bin)) {
        throw ("no capture to replay at '{0}'.  The .bin files beside this repository are the recorded HOST stream; pass one with -Bin." -f $Bin)
    }
    if (-not (Test-Path $CigiStarterExe)) {
        throw ("CigiStarter.exe not found at {0} - build it first (README.md)." -f $CigiStarterExe)
    }

    $pending = Get-UdpOwner $IgPort
    if ($pending -ne 0) {
        $proc = Get-Process -Id $pending -ErrorAction SilentlyContinue
        $name = 'another program'
        if ($proc) { $name = $proc.ProcessName }
        if (-not $proc -or $name -ne 'CigiStarter') {
            throw ("UDP port {0} is held by {1} (pid {2}), not CigiStarter." -f $IgPort, $name, $pending)
        }
        Write-Host ("  note        a CigiStarter is already listening on {0} (pid {1}) - this run will stop it first" -f $IgPort, $pending)
        if ($KeepRunning) { throw "-KeepRunning: stop the CigiStarter that is already up first" }
        Stop-CigiStarter
    }

    $p3d = Get-Process Prepar3D -ErrorAction SilentlyContinue
    $dllLoaded = $false
    $addonWait = 0
    if ($p3d) {
        # A sim started a moment ago has not loaded its add-ons yet, so give the
        # module a bounded moment to appear rather than calling it "not installed".
        while (-not $dllLoaded -and $addonWait -lt 60000) {
            $dllLoaded = [bool]($p3d.Modules | Where-Object { $_.ModuleName -eq 'PdkSimLink.dll' })
            if ($dllLoaded) { break }
            Start-Sleep -Milliseconds 1000
            $addonWait += 1000
            $p3d = Get-Process Prepar3D -ErrorAction SilentlyContinue
            if (-not $p3d) { break }
        }
        if ($p3d) {
            $dllLoaded = [bool]($p3d.Modules | Where-Object { $_.ModuleName -eq 'PdkSimLink.dll' })
            Write-Host ("  sim         Prepar3D pid {0}{1}" -f $p3d.Id,
                $(if ($addonWait -gt 0) { "   (add-on appeared after {0:N1}s)" -f ($addonWait / 1000.0) } else { '' }))
        }
    }
    $hasP3d = [bool]$p3d
    if (-not $hasP3d) {
        Write-Host '  sim         Prepar3D is NOT running' -ForegroundColor Yellow
    }

    if (Test-Path "$addonDir\PdkSimLink.ini") {
        $title = ((Get-Content "$addonDir\PdkSimLink.ini" | Select-String '^\s*title\s*=' | Select-Object -First 1) -replace '^\s*title\s*=\s*', '').Trim()
    }
    Write-Host ("  add-on      title='{0}'" -f $(if ($title) { $title } else { '(none)' }))

    if ((Test-Path $builtDll) -and (Test-Path $deployedDll)) {
        $deployMatches = (Get-FileHash $builtDll).Hash -eq (Get-FileHash $deployedDll).Hash
    }

    Write-Host ''
    Write-Host '--- the sim half of the writer'
    $script:currentTest = 'the sim half of the writer'

    Assert 'Prepar3D is running (the writer renders inside it)' $hasP3d `
        $(if ($hasP3d) { 'pid ' + $p3d.Id } else { 'start Prepar3D with the entity writer installed first' })
    if (-not $hasP3d) { throw 'the sim has to be up before this test means anything' }

    Assert 'PdkSimLink.dll is loaded in Prepar3D' $dllLoaded `
        $(if ($dllLoaded) { 'add-on.xml loaded it' } else { 'it is not in the process: check ' + $addonDir })

    Assert 'the loaded add-on is the build in this tree' $deployMatches `
        $(if ($deployMatches) { 'hashes match' } else { 'STALE: copy ' + $builtDll + ' over ' + $deployedDll + ' and restart Prepar3D' })

    Assert 'a container title is configured' ([bool]$title) `
        $(if ($title) { $title } else { 'PdkSimLink.ini has no title=, so nothing can be created' })

    if ($title) {
        $installRoot = Split-Path -Parent $p3d.Path
        $titleAt = Find-SimTitle $title $installRoot
        Assert 'the container title exists in this installation' ([bool]$titleAt) `
            $(if ($titleAt) { $titleAt } else { "'$title' is not in the sim's SimObjects tree or the add-on folder - the add-on will create nothing" })
        if (-not $titleAt) {
            Write-Host '      (an add-on may register SimObjects from elsewhere; the create-failure count below is the last word)'
        }
    }

    # The module being loaded and the add-on having finished Load() are two
    # different moments, so retry the region rather than reading a race as a
    # broken install.
    $view = Open-SimLink
    $regionWait = 0
    while ($view -eq [IntPtr]::Zero -and $regionWait -lt 30000) {
        Start-Sleep -Milliseconds 500
        $regionWait += 500
        $view = Open-SimLink
    }
    Assert 'the shared region exists (the add-on created it)' ($view -ne [IntPtr]::Zero) $mappingName
    if ($view -eq [IntPtr]::Zero) { throw 'no shared region: the add-on did not start, or is an older build' }

    $h = Get-WriterHealth
    Assert 'the region speaks the contract this build expects' `
        ($h.Magic -eq 0x4B4C5350 -and $h.Version -eq 2 -and $h.Capacity -eq 64) `
        ("magic 0x{0:X8}, version {1}, capacity {2}" -f $h.Magic, $h.Version, $h.Capacity)

    Assert 'the add-on obtained the sim object manager' $h.HasManager `
        $(if ($h.HasManager) { '' } else { 'QueryService(SID_SimObjectManager) failed' })
    Assert 'the sim frame event is being delivered' ($h.HasTick -and $h.Alive -eq 1) `
        ("hasTick={0}, alive={1}, diag 0x{2:X8}" -f $h.HasTick, $h.Alive, $h.Diag)

    # The frame event is the tick of a *running* sim (the add-on has no fallback
    # thread by design), so a paused Prepar3D is visible right here.  A sim that
    # is still loading its scenario looks exactly the same -- frame ticks frozen
    # -- so wait for the tick to start rather than sampling once.  How long it
    # took is worth printing: that wait is the scenario load.
    $ft0 = $h.FrameTicks
    $waited = 0
    $moved = $false
    while (-not $moved -and $waited -lt 90000) {
        Start-Sleep -Milliseconds 500
        $waited += 500
        if ((Get-WriterHealth).FrameTicks -ne $ft0) { $moved = $true }
    }
    Assert 'the sim is running, not paused' $moved `
        ("frame ticks {0} -> {1} after {2:N1}s" -f $ft0, (Get-WriterHealth).FrameTicks, ($waited / 1000.0))
    if (-not $moved) {
        Write-Host '      (a Prepar3D still loading its scenario looks like this too, so check the sim before blaming a pause)'
    }
} catch {
    Write-Host ''
    Write-Host ("the bench did not come up: {0}" -f $_.Exception.Message) -ForegroundColor Red
    Write-Verdict 'FAIL   --   the bench did not come up' 'Red'
    exit 1
}

#-----------------------------------------------------------------------------
# The run: replay the HOST into CigiStarter, watching the region fill
#-----------------------------------------------------------------------------
$records = Read-BinFile $Bin
$span = 0.0
foreach ($r in $records) { $span += $r.Delta / 1000000.0 }
Write-Host ''
Write-Host ("  replayed    {0} datagrams, {1:N1}s of HOST time" -f $records.Count, $span)

$before = Get-WriterHealth
$udp = New-Object System.Net.Sockets.UdpClient
$udp.Connect('127.0.0.1', $IgPort)

Run-Test 'the HOST entities reach the sim' {
    Write-Host ("  starting {0}" -f $CigiStarterExe)
    Start-Process -FilePath $CigiStarterExe -WorkingDirectory (Split-Path -Parent $CigiStarterExe) `
        -ArgumentList @('--host-ip','127.0.0.1','--ig-ip','127.0.0.1','--ig-port',"$IgPort",'--host-port',"$HostPort")
    $script:startedCigi = $true
    $by = (Get-Date).AddSeconds(15)
    while ((Get-UdpOwner $IgPort) -eq 0 -and (Get-Date) -lt $by) { Start-Sleep -Milliseconds 200 }
    Assert 'CigiStarter is listening on the IG port' ((Get-UdpOwner $IgPort) -ne 0) ("udp $IgPort")
    if ((Get-UdpOwner $IgPort) -eq 0) { return }

    # Replay, remembering what the HOST asked for. The row the IG keeps is the
    # last usable pose published for an entity id, and a slot already held by a
    # different id is not overwritten (SimLink.cpp) -- so the expectation is
    # built the same way the publisher builds the table.
    $lastBySlot = @{}
    $sampled = @()
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $target = 0.0
    $sent = 0
    $every = [math]::Max(1, [int]($records.Count / 8))

    foreach ($rec in $records) {
        $target += ($rec.Delta / 1000000.0) / $Speed
        if ($rec.Payload.Length -gt 0) { [void]$udp.Send($rec.Payload, $rec.Payload.Length) }
        $sent++

        foreach ($e in (Get-EntityControls $rec.Payload)) {
            if (-not (Test-PoseUsable $e)) { continue }
            $slot = Get-SlotOf $e.EntityId
            if ((-not $lastBySlot.ContainsKey($slot)) -or ($lastBySlot[$slot].EntityId -eq $e.EntityId)) {
                $lastBySlot[$slot] = [pscustomobject]@{
                    EntityId = $e.EntityId; State = $e.State
                    Lat = $e.Lat; Lon = $e.Lon; Alt = $e.Alt
                    Roll = $e.Roll; Pitch = $e.Pitch; Yaw = $e.Yaw
                }
            }
        }

        if (($sent % $every) -eq 0) {
            $row = Get-EntityRow (Get-SlotOf 0)
            if ($null -ne $row -and $row.Enabled -ne 0) {
                $sampled += [pscustomobject]@{ Lat = $row.Lat; Lon = $row.Lon; Alt = $row.Alt }
            }
        }

        if ($sw.Elapsed.TotalSeconds -lt $target) {
            $wait = $target - $sw.Elapsed.TotalSeconds
            if ($wait -gt 0.002) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
        }
        if ($sw.Elapsed.TotalSeconds -ge $MaxSeconds) { break }
    }
    $udp.Close()
    Write-Host ("  replayed    {0} datagrams in {1:N1}s" -f $sent, $sw.Elapsed.TotalSeconds)

    # Let the add-on take a few ticks on the last frames before reading it.
    Start-Sleep -Milliseconds 1200

    $wanted = @($lastBySlot.Values | Where-Object { $_.State -eq 1 })
    $after = Get-WriterHealth

    Assert 'the HOST stream carried entities with a usable pose' ($lastBySlot.Count -gt 0) `
        ("{0} row(s) expected" -f $lastBySlot.Count)
    if ($lastBySlot.Count -eq 0) { return }

    Assert 'the sim created an object per driven entity' `
        ($after.ObjectsCreated - $before.ObjectsCreated -ge $wanted.Count) `
        ("created {0}, expected at least {1} (state Active)" -f ($after.ObjectsCreated - $before.ObjectsCreated), $wanted.Count)
    Assert 'every created object is being positioned each tick' `
        ($after.Applied -eq $wanted.Count) `
        ("applied {0} of {1} on the last tick" -f $after.Applied, $wanted.Count)
    Assert 'no object failed to be created' ($after.CreateFails -eq $before.CreateFails) `
        ("create failures {0} -> {1}" -f $before.CreateFails, $after.CreateFails)
    Assert 'no pose was refused by the sim' ($after.SetFails -eq $before.SetFails) `
        ("SetPosition failures {0} -> {1}, last HRESULT 0x{2:X8}" -f $before.SetFails, $after.SetFails, $after.LastError)

    # The read-back.  SetPosition succeeding says the sim accepted the pose, not
    # that it kept it: the object is the sim's own and has its own simulation
    # between writes.  PdkSimLink reads each object's actual pose back with
    # GetPosition and publishes it, so these checks are the difference between
    # "the sim took the pose" and "the sim is holding it".
    Assert 'the add-on is reading the objects back from the sim' `
        ($after.PositionReads -gt $before.PositionReads) `
        ("GetPosition {0} -> {1}, failures {2}" -f $before.PositionReads, $after.PositionReads, $after.GetFails)
    Assert 'no read-back was refused by the sim' ($after.GetFails -eq $before.GetFails) `
        ("GetPosition failures {0} -> {1}, last HRESULT 0x{2:X8}" -f $before.GetFails, $after.GetFails, $after.LastError)

    $held = Get-HeldRows
    Assert 'a read-back row exists for every object being driven' `
        ($held.Count -ge $wanted.Count) ("{0} held row(s), {1} driven" -f $held.Count, $wanted.Count)

    $heldAgrees = ($held.Count -gt 0)
    foreach ($h in $held) {
        $cmd = Get-EntityRow $h.Slot
        if ($null -eq $cmd -or $cmd.Enabled -eq 0 -or $cmd.EntityId -ne $h.EntityId) { $heldAgrees = $false }
    }
    Assert 'each read-back row belongs to the entity in that slot' $heldAgrees `
        'held[] and entities[] agree on the slot and the id'

    # THE MEASUREMENT.  This is the number the whole test exists for: how far the
    # native object wandered from the pose it was last written, at its worst,
    # while the aircraft was moving.  It is reported rather than asserted -- a
    # large value is a finding about the sim's own objects, not a failure of the
    # link, and it is what decides whether a physics-free ISimObject is needed.
    Write-Host ''
    Write-Host '  the read-back (held vs last commanded)'
    foreach ($h in ($held | Sort-Object Slot)) {
        Write-Host ("    slot {0,2}  id {1,-6}  held      lat {2:F6} lon {3:F6} alt {4,8:F1}  rpy {5:F1}/{6:F1}/{7:F1}   drift {8:F2} m / {9:F2} deg" -f `
            $h.Slot, $h.EntityId, $h.Lat, $h.Lon, $h.Alt, $h.Roll, $h.Pitch, $h.Yaw, $h.DriftMeters, $h.DriftDeg)
        $cmd = Get-EntityRow $h.Slot
        if ($null -ne $cmd) {
            Write-Host ("    {0,24}commanded lat {1:F6} lon {2:F6} alt {3,8:F1}  rpy {4:F1}/{5:F1}/{6:F1}" -f `
                '', $cmd.Lat, $cmd.Lon, $cmd.Alt, $cmd.Roll, $cmd.Pitch, $cmd.Yaw)
        }
    }
    Write-Host ("    worst over the run: {0:F2} m, {1:F2} deg   ({2} comparisons)" -f `
        $after.DriftMaxMeters, $after.DriftMaxDeg, $after.DriftSamples)
    if ($after.DriftMaxMeters -lt 0.5 -and $after.DriftMaxDeg -lt 0.5) {
        Write-Host '    => the object stays on the pose it is written, to a few centimetres'
    } else {
        Write-Host '    => the object left the pose it was written.  Check the commanded altitude above:'
        Write-Host '       the sim clamps an object to its own ground, so a command below the sim''s'
        Write-Host '       terrain is held on the surface and reads as drift without being one.'
    }

    # The rows themselves: the pose the sim was told is the pose the HOST sent.
    $rows = Get-LiveRows
    Write-Host ''
    Write-Host '  the rows the IG is holding'
    foreach ($r in ($rows | Sort-Object Slot)) {
        Write-Host ("    slot {0,2}  id {1,-6} state {2}  lat {3:F6} lon {4:F6} alt {5,8:F1} m  rpy {6:F1}/{7:F1}/{8:F1}" -f `
            $r.Slot, $r.EntityId, $r.State, $r.Lat, $r.Lon, $r.Alt, $r.Roll, $r.Pitch, $r.Yaw)
    }

    $matched = 0
    foreach ($slot in ($lastBySlot.Keys | Sort-Object)) {
        $want = $lastBySlot[$slot]
        $got = Get-EntityRow $slot
        if ($null -eq $got -or $got.Enabled -eq 0) { continue }
        if ($got.EntityId -ne $want.EntityId) { continue }
        if (-not $got.PoseValid) { continue }
        if (-not (Compare-Number $got.Lat $want.Lat 1e-9)) { continue }
        if (-not (Compare-Number $got.Lon $want.Lon 1e-9)) { continue }
        if (-not (Compare-Number $got.Alt $want.Alt 1e-6)) { continue }
        if (-not (Compare-Number $got.Roll  $want.Roll  1e-6)) { continue }
        if (-not (Compare-Number $got.Pitch $want.Pitch 1e-6)) { continue }
        if (-not (Compare-Number $got.Yaw   $want.Yaw   1e-6)) { continue }
        $matched++
    }
    Assert 'every driven entity is in the table on the pose that was sent' `
        ($matched -eq $lastBySlot.Count) ("{0} of {1} rows match" -f $matched, $lastBySlot.Count)

    $aircraft = Get-SlotOf 0
    Assert "the HOST's own aircraft (entity 0) is among them" `
        (($lastBySlot.ContainsKey($aircraft)) -and ($lastBySlot[$aircraft].EntityId -eq 0)) `
        'entity 0 is the aircraft the whole picture is arranged around'

    # Movement: entity 0's published pose has to change over the run, or the
    # table is being filled once and left.
    $distinct = @($sampled | ForEach-Object { '{0:F6}/{1:F6}/{2:F2}' -f $_.Lat, $_.Lon, $_.Alt } | Select-Object -Unique)
    Assert 'the aircraft pose is moving through the link, not frozen' ($distinct.Count -gt 1) `
        ("{0} distinct poses sampled during the replay" -f $distinct.Count)
}

#-----------------------------------------------------------------------------
# Teardown: stopping the IG must retire the rows, so the add-on takes the
# objects back out. A stopped IG that left them parked is the failure that
# would litter the sim.
#-----------------------------------------------------------------------------
if ($script:startedCigi -and -not $KeepRunning) {
    Run-Test 'the link is torn down cleanly' {
        Write-Host '  stopping CigiStarter (EXIT)'
        Stop-CigiStarter
        Start-Sleep -Milliseconds 1500

        $rows = Get-LiveRows
        Assert 'no entity row is left enabled' ($rows.Count -eq 0) `
            ("{0} row(s) still live" -f $rows.Count)

        $end = Get-WriterHealth
        $made   = $end.ObjectsCreated - $before.ObjectsCreated
        $unmade = $end.ObjectsRemoved - $before.ObjectsRemoved
        Assert 'the add-on took this run''s objects back out' ($made -eq $unmade) `
            ("{0} created, {1} removed" -f $made, $unmade)
        Assert 'the add-on is still alive after the IG left' ($end.Alive -eq 1) 'the sim side is untouched'
    }
} elseif ($KeepRunning) {
    Write-Host ''
    Write-Host '  -KeepRunning: CigiStarter is still up, so its objects are still in the sim.'
    Write-Host '  Stop it (EXIT) to retire them.'
}

#-----------------------------------------------------------------------------
# Summary
#-----------------------------------------------------------------------------
$passed  = @($script:results | Where-Object { $_.Result -eq 'PASS' }).Count
$failed  = @($script:results | Where-Object { $_.Result -eq 'FAIL' }).Count

Write-Host ''
Write-Host '=== checks ==='
if ($failed -gt 0) {
    Write-Host 'failures:'
    $script:results | Where-Object { $_.Result -eq 'FAIL' } | Format-Table -AutoSize | Out-String | Write-Host
}
$script:results | Format-Table -AutoSize | Out-String | Write-Host

$verdict = 'FAIL'
$colour  = 'Red'
if ($failed -eq 0) { $verdict = 'PASS'; $colour = 'Green' }
Write-Verdict ('{0}   --   {1} passed, {2} failed' -f $verdict, $passed, $failed) $colour

if ($failed -gt 0) { exit 1 }
exit 0
