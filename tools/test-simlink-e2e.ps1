<#
.SYNOPSIS
    Prove the entity writer works end to end on this PC: replay a recorded HOST
    stream into CigiStarter, and read PdkSimLink's own report out of the running
    Prepar3D to show the sim created the HOST's entities, is holding them on the
    poses the HOST commanded, and has taken the HOST's own clock, air and sky
    into its own time and weather systems.

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
      still there a frame later. PdkSimLink reads each object's actual pose back
      (IBaseObjectV400::GetPosition) and publishes it, and this test compares
      that against the pose that was written and reports the drift at its worst
      over the run.

      Which object is being read back decides what the number means, and the
      add-on's status word says which it is (`PhysicsFree`, set from the object's
      own category GUID rather than from the configured title):

        * the sim's own object (a stock title, e.g. the AS332).  It keeps its own
          simulation between writes and refuses a pose far below the ground under
          it.  On the bench it tracked the command to a few centimetres and tenths
          of a degree in the air, and a pose published well below the ground was
          held above it (a property of the place, not the terrain surface: 176.6 m
          over 174.3 m terrain, +26.7 m over water).  gnd-fly-bank.bin never
          commands below the ground, so its drift is ~0.2 m, not a refusal.  The
          number is reported, not asserted: a large value here is a finding about
          the sim's own objects, not a failure of the link.

        * CigiStarter.Entity, the physics-free object (CigiEntity.h).  It has no
          simulation and no ground contact, so the pose written is the pose that
          stays and a command below the sim's terrain is honoured.  Here the
          drift is asserted to be zero: if the object yields any of the pose it
          is given, the assertion fails.

      The status word also says whether any object is paused or in slew
      (`ModePauseOrSlew`); a physics-free object needs neither, and item 4's
      rotor cannot turn with either.

    The world
      The same region also carries the HOST's clock, air and cloud and the
      add-on's answer to them (SimLinkIpc.h, kVersion 4, with the sim's own sky
      read-back at 5), so this test reads the world the IG resolved, the
      generation the add-on applied, and the sky the sim's own weather system
      holds -- the GLOB station's cloud layers and the observation the sim
      parsed. The last is the only witness that a sky became cloud rather than
      merely being requested, and it is what makes the three `cloud pat*.bin`
      captures checkable at all: they describe their sky with a Component
      Control (a cloud pattern) and nothing else. The sky is sampled while the
      pattern is up, because a capture takes it away again before the run ends
      and a reading taken afterwards is a clear sky that is perfectly correct and
      proves nothing.

    Prepar3D must be running with the current PdkSimLink.dll loaded, and the
    DLL must be the build in this tree: the add-on is loaded once, at sim
    startup, and a stale copy would test the wrong code. Both are checked before
    anything is started, and the run says what to do about it.

    The objects the test creates use whatever container title PdkSimLink.ini
    names -- normally CigiStarter.Entity, the physics-free object, and a stock
    title only for a like-for-like comparison -- so a few objects will appear in
    the sim while it runs. With `camera` set, the entity the camera rides is
    created from `own_ship_title` instead (normally CigiStarter.Entity.OwnShip,
    the model-less container, so the eye is not inside a drawn model); the test
    checks that container is installed too. CigiStarter is stopped at the end,
    which retires every row and takes the objects back out.

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

# SimLinkIpc.h EnvFlags, by the number the region carries, so the checks below
# read as the contract rather than as masks.
$EnvApplyTime     = 0x001
$EnvApplyAtmos    = 0x002
$EnvApplyWeather  = 0x004
$EnvTimeManual    = 0x008
$EnvWeatherManual = 0x010
$EnvDateValid     = 0x020
$EnvLightning     = 0x040
$EnvTimeValid     = 0x080
$EnvAtmosValid    = 0x100
$EnvWeatherValid  = 0x200
$EnvWeatherPattern = 0x400

# SimLinkIpc.h kVersion, and the version the e2e reads the region as.  The
# layout it walks is the one this number names, so a stale DLL (the add-on is
# loaded once, at sim startup) has to be caught here rather than read as a
# nonsense world.
$ExpectedVersion = 5

if (-not $Bin) {
    $Bin = Join-Path (Split-Path -Parent $repo) 'hlm.bin'
    if (-not (Test-Path $Bin)) {
        $Bin = Join-Path (Split-Path -Parent $repo) 'gnd-fly-bank.bin'
    }
}

#-----------------------------------------------------------------------------
# The shared region, re-derived from SimLinkIpc.h
#
#   header   magic 0, version 5, capacity 8, simPid 12, alive 16, lastError 20,
#            ticks 24, frameTicks 28, objectsCreated 32, objectsRemoved 36,
#            createFails 40, setFails 44, applied 48, diag 52,
#            positionReads 56, getFails 60, driftSamples 64, heldValid 68,
#            driftMaxMeters 72 (f64), driftMaxDeg 80 (f64)
#   entities[64] at 88, each row 72 bytes: seq 0, entityId 4, enabled 8,
#            flags 12, state 16, lat 24, lon 32, alt 40, roll 48, pitch 56, yaw 64
#   held[64] at 4696, each row 80 bytes: seq 0, entityId 4, tick 8, valid 12,
#            lat 16, lon 24, alt 32, roll 40, pitch 48, yaw 56,
#            driftMeters 64, driftDeg 72
#   view     at 9816, 4 u32 (16) + 6 doubles (48) = 64: the HOST's own eye
#   environment       at 9880, 12 u32 (48) + 10 doubles (80) = 128: the
#            resolved world (generation, flags, clock, cloud, air)
#   environmentStatus at 10008, 10 u32 (40) + 2 doubles (16) + 200 chars = 256:
#            what the add-on applied, the time of day the sim reported back, and
#            the sky the sim itself holds (its cloud layers and the station
#            observation it parsed)
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
  // A NUL-terminated narrow string, for the station observation the add-on
  // copies out of the sim's own weather system.
  public static string Ansi(IntPtr v, int o, int max) {
    var sb = new System.Text.StringBuilder();
    for (int i = 0; i < max; i++) {
      byte b = Marshal.ReadByte(IntPtr.Add(v, o + i));
      if (b == 0) break;
      sb.Append((char)b);
    }
    return sb.ToString();
  }
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
        PhysicsFree     = (($d -shr 29) -band 1) -eq 1
        ModePauseOrSlew = (($d -shr 30) -band 1) -eq 1
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

# The HOST's own eye, as the IG publishes it for the camera: one row, appended
# after held[] (SimLinkIpc.h, kVersion 3), read under the same seqlock.  The
# offsets are the row's 4 u32 header then its 6 doubles.
function Get-ViewRow {
    $o = 9816
    for ($try = 0; $try -lt 200; $try++) {
        $before = Region-U32 $o
        if ($before -band 1) { Start-Sleep -Milliseconds 1; continue }
        $row = [pscustomobject]@{
            Seq      = $before
            Valid    = Region-U32 ($o + 4)
            EntityId = Region-U32 ($o + 8)
            ViewId   = Region-U32 ($o + 12)
            X        = Region-F64 ($o + 16)
            Y        = Region-F64 ($o + 24)
            Z        = Region-F64 ($o + 32)
            Roll     = Region-F64 ($o + 40)
            Pitch    = Region-F64 ($o + 48)
            Yaw      = Region-F64 ($o + 56)
        }
        if ((Region-U32 $o) -eq $before) { return $row }
    }
    return $null
}

# The resolved world the IG publishes (SimLinkIpc.h, kVersion 4): one row at
# 9880, appended after the view.  It carries what the IG wants applied -- the
# clock, the air and the cloud -- already resolved with any operator override,
# so this is exactly the sim side's input.  Read under its seqlock like the rest.
function Get-EnvironmentRow {
    $o = 9880
    for ($try = 0; $try -lt 200; $try++) {
        $before = Region-U32 $o
        if ($before -band 1) { Start-Sleep -Milliseconds 1; continue }
        $row = [pscustomobject]@{
            Seq              = $before
            Generation       = Region-U32 ($o + 4)
            Flags            = Region-U32 ($o + 8)
            Hour             = Region-U32 ($o + 12)
            Minute           = Region-U32 ($o + 16)
            Second           = Region-U32 ($o + 20)
            Year             = Region-U32 ($o + 24)
            Month            = Region-U32 ($o + 28)
            Day              = Region-U32 ($o + 32)
            CloudType        = Region-U32 ($o + 36)
            Severity         = Region-U32 ($o + 40)
            PrecipType       = Region-U32 ($o + 44)
            AirTempC         = Region-F64 ($o + 48)
            VisibilityMeters = Region-F64 ($o + 56)
            Coverage         = Region-F64 ($o + 64)
            BaseElevMeters   = Region-F64 ($o + 72)
            ThicknessMeters  = Region-F64 ($o + 80)
            HorizWindMps     = Region-F64 ($o + 88)
            VertWindMps      = Region-F64 ($o + 96)
            WindDirDeg       = Region-F64 ($o + 104)
            BaroPressureMb   = Region-F64 ($o + 112)
            HumidityPct      = Region-F64 ($o + 120)
        }
        if ((Region-U32 $o) -eq $before) { return $row }
    }
    return $null
}

# The add-on's answer (at 10008): the generation it applied, how many it has
# applied, the time of day Prepar3D itself reports -- the one reading that says a
# commanded 12:00 became *day* rather than merely being accepted -- and the sky
# the sim says it holds, which is the same reading for the cloud: a METAR that
# names a cloud is a request, and this is the layer that exists.
function Get-EnvironmentStatus {
    $o = 10008
    for ($try = 0; $try -lt 200; $try++) {
        $before = Region-U32 $o
        if ($before -band 1) { Start-Sleep -Milliseconds 1; continue }
        $row = [pscustomobject]@{
            Seq               = $before
            AppliedGeneration = Region-U32 ($o + 4)
            Applies           = Region-U32 ($o + 8)
            Flags             = Region-U32 ($o + 12)
            TimeOfDay         = Region-I32 ($o + 16)
            LastError         = Region-I32 ($o + 20)
            CloudLayers       = Region-U32 ($o + 24)
            SimCloudType      = Region-U32 ($o + 28)
            SimCloudCover     = Region-U32 ($o + 32)
            SimPrecipType     = Region-I32 ($o + 36)
            SimCloudBaseM     = Region-F64 ($o + 40)
            SimCloudTopsM     = Region-F64 ($o + 48)
            SimMetar          = [SimLinkShm]::Ansi($script:view, $o + 56, 200)
        }
        if ((Region-U32 $o) -eq $before) { return $row }
    }
    return $null
}
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

# A View Control (id 16), the HOST's own eye, re-derived from cigi.h the same
# way the entity rows above are: 32 bytes, the entity it is attached to at 6 and
# six single-precision floats from 8.
function Get-ViewControls([byte[]]$p) {
    $out = New-Object System.Collections.Generic.List[object]
    $i = 0
    while ($i + 2 -le $p.Length) {
        $id = $p[$i]; $size = $p[$i + 1]
        if ($size -lt 2 -or ($i + $size) -gt $p.Length) { $i++; continue }
        if ($id -eq 16 -and $size -ge 32) {
            $out.Add([pscustomobject]@{
                ViewId   = [BitConverter]::ToUInt16($p, $i + 2)
                EntityId = [BitConverter]::ToUInt16($p, $i + 6)
                X        = [double]([BitConverter]::ToSingle($p, $i + 8))
                Y        = [double]([BitConverter]::ToSingle($p, $i + 12))
                Z        = [double]([BitConverter]::ToSingle($p, $i + 16))
                Roll     = [double]([BitConverter]::ToSingle($p, $i + 20))
                Pitch    = [double]([BitConverter]::ToSingle($p, $i + 24))
                Yaw      = [double]([BitConverter]::ToSingle($p, $i + 28))
            })
        }
        $i += $size
    }
    return ,$out
}

# A Component Control (id 4), which is how the HOST describes its cloud pattern:
# 32 bytes, the component id at 2, the entity it is attached to at 4, the class
# at 6, the state -- the pattern's index, 0 for "take it away" -- at 7 and six
# single-precision floats from 8.  The three `cloud pat*.bin` captures are the
# only ones in this deployment that carry one, and their component is 0x1001 on
# the entity 0x9001 with a footprint and an altitude in the first three data
# fields (metres: 8 x 1852 across, 7 x 1852 deep and 3000 ft up for the first).
function Get-CloudPatterns([byte[]]$p) {
    $out = New-Object System.Collections.Generic.List[object]
    $i = 0
    while ($i + 2 -le $p.Length) {
        $id = $p[$i]; $size = $p[$i + 1]
        if ($size -lt 2 -or ($i + $size) -gt $p.Length) { $i++; continue }
        if ($id -eq 4 -and $size -ge 32) {
            $out.Add([pscustomobject]@{
                ComponentId = [BitConverter]::ToUInt16($p, $i + 2)
                InstanceId  = [BitConverter]::ToUInt16($p, $i + 4)
                Class       = [int]$p[$i + 6]
                State       = [int]$p[$i + 7]
                Data1       = [double]([BitConverter]::ToSingle($p, $i + 8))
                Data2       = [double]([BitConverter]::ToSingle($p, $i + 12))
                Data3       = [double]([BitConverter]::ToSingle($p, $i + 16))
            })
        }
        $i += $size
    }
    return ,$out
}

# The HOST's world, re-derived from cigi.h the same way: a Celestial Sphere
# Control (id 9, the clock and the packed date), an Atmosphere Control (id 10,
# the air) and a Weather Control (id 12, cloud and precipitation -- which no
# capture in this deployment carries).  One datagram can carry any of them, and
# the last of each kind in the run is what the accumulated world holds.
function Get-EnvironmentMessages([byte[]]$p) {
    $time = $null; $atmos = $null; $weather = $null
    $i = 0
    while ($i + 2 -le $p.Length) {
        $id = [int]$p[$i]; $size = [int]$p[$i + 1]
        if ($size -lt 2 -or ($i + $size) -gt $p.Length) { $i++; continue }
        if ($id -eq 9 -and $size -ge 16) {
            $time = [pscustomobject]@{
                Hour   = [int]$p[$i + 2]
                Minute = [int]$p[$i + 3]
                Flags  = [int]$p[$i + 4]
                Date   = [BitConverter]::ToUInt32($p, $i + 8)
            }
        } elseif ($id -eq 10 -and $size -ge 32) {
            $atmos = [pscustomobject]@{
                Humidity   = [int]$p[$i + 3]
                TempC      = [double]([BitConverter]::ToSingle($p, $i + 4))
                Visibility = [double]([BitConverter]::ToSingle($p, $i + 8))
                HorizWind  = [double]([BitConverter]::ToSingle($p, $i + 12))
                WindDir    = [double]([BitConverter]::ToSingle($p, $i + 20))
                Baro       = [double]([BitConverter]::ToSingle($p, $i + 24))
            }
        } elseif ($id -eq 12 -and $size -ge 56) {
            $flags  = [int]$p[$i + 6]
            $flags2 = [int]$p[$i + 7]
            $weather = [pscustomobject]@{
                LayerId    = [int]$p[$i + 4]
                Humidity   = [int]$p[$i + 5]
                CloudType  = ($flags -shr 4) -band 0x0F
                Lightning  = (($flags -shr 3) -band 1) -eq 1
                Severity   = ($flags2 -shr 2) -band 0x07
                TempC      = [double]([BitConverter]::ToSingle($p, $i + 8))
                Visibility = [double]([BitConverter]::ToSingle($p, $i + 12))
                Coverage   = [double]([BitConverter]::ToSingle($p, $i + 20))
                BaseElevM  = [double]([BitConverter]::ToSingle($p, $i + 24))
                ThicknessM = [double]([BitConverter]::ToSingle($p, $i + 28))
            }
        }
        $i += $size
    }
    return [pscustomobject]@{ Time = $time; Atmos = $atmos; Weather = $weather }
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
$ownShipTitle = ''
$cameraId = ''
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
        $ownShipTitle = ((Get-Content "$addonDir\PdkSimLink.ini" | Select-String '^\s*own_ship_title\s*=' | Select-Object -First 1) -replace '^\s*own_ship_title\s*=\s*', '').Trim()
        $cameraId = ((Get-Content "$addonDir\PdkSimLink.ini" | Select-String '^\s*camera\s*=\s*' | Select-Object -First 1) -replace '^\s*camera\s*=\s*', '').Trim()
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

    # The entity the sim's view rides is created from `own_ship_title` and not
    # from `title`, so with the camera on, that container has to be in the
    # installation as well.  A camera whose own ship cannot be created stops at
    # the picture: the pose arrives, the view is driven, and nothing is drawn
    # where the eye is, which reads as a link failure rather than an install one.
    if ($cameraId -and $ownShipTitle -and $title) {
        $ownAt = Find-SimTitle $ownShipTitle (Split-Path -Parent $p3d.Path)
        Assert 'the own-ship container exists in this installation' ([bool]$ownAt) `
            $(if ($ownAt) { $ownAt } else { "'$ownShipTitle' is not in the sim's SimObjects tree or the add-on folder - the entity the camera rides cannot be created" })
        if (-not $ownAt) {
            Write-Host '      (copy SimObjects\CigiEntity.OwnShip, or set own_ship_title empty to create the own ship from title=)'
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
        ($h.Magic -eq 0x4B4C5350 -and $h.Version -eq $ExpectedVersion -and $h.Capacity -eq 64) `
        ("magic 0x{0:X8}, version {1} (expected {2}), capacity {3}" -f `
            $h.Magic, $h.Version, $ExpectedVersion, $h.Capacity)

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
    $lastView = $null
    $lastTime = $null
    $lastAtmos = $null
    $lastWeather = $null
    $sawEnv = $false
    # The HOST's cloud pattern (a Component Control), when the capture describes
    # its sky that way: the pattern is turned off again near the end, so what it
    # asked for has to be remembered as the run goes by, and the sim's own answer
    # has to be sampled while it is still up.
    $sawPattern = $false
    $sawPatternOff = $false
    $lastPattern = $null
    $patternStates = @{}
    $simSky = $null
    $simSkyLayers = 0
    $patternRow = $null
    $simSkyEnd = $null
    $sampled = @()
    $sentPoses = @{}
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
            if ($e.EntityId -eq 0) {
                # What the capture itself commanded for the aircraft, so the
                # movement check below can tell a frozen capture (the four
                # *_replay.bin time captures) from a link that stopped moving.
                $sentPoses[('{0:F6}/{1:F6}/{2:F2}' -f $e.Lat, $e.Lon, $e.Alt)] = $true
            }
            $slot = Get-SlotOf $e.EntityId
            if ((-not $lastBySlot.ContainsKey($slot)) -or ($lastBySlot[$slot].EntityId -eq $e.EntityId)) {
                $lastBySlot[$slot] = [pscustomobject]@{
                    EntityId = $e.EntityId; State = $e.State
                    Lat = $e.Lat; Lon = $e.Lon; Alt = $e.Alt
                    Roll = $e.Roll; Pitch = $e.Pitch; Yaw = $e.Yaw
                }
            }
        }

        # The HOST's own eye comes on the same stream and is what the picture is
        # flown from, so the last one seen is kept to check against the row.
        foreach ($v in (Get-ViewControls $rec.Payload)) { $lastView = $v }

        # The world beyond the entities, likewise: the last of each kind in the
        # run is what the accumulated HostEnvironment holds and the IG resolves.
        $envMsg = Get-EnvironmentMessages $rec.Payload
        if ($null -ne $envMsg.Time)    { $lastTime    = $envMsg.Time;    $sawEnv = $true }
        if ($null -ne $envMsg.Atmos)   { $lastAtmos   = $envMsg.Atmos;   $sawEnv = $true }
        if ($null -ne $envMsg.Weather) { $lastWeather = $envMsg.Weather; $sawEnv = $true }

        # The HOST's cloud pattern, when the capture has one: the component id is
        # the vendor's (0x1001) and the state is the pattern's index, 0 being the
        # HOST taking the cloud away again.
        foreach ($cc in (Get-CloudPatterns $rec.Payload)) {
            if ($cc.ComponentId -ne 0x1001) { continue }
            $sawEnv = $true
            $sawPattern = $true
            if ($cc.State -gt 0) {
                $lastPattern = $cc
                $patternStates[$cc.State] = $true
            } else {
                $sawPatternOff = $true
            }
        }

        if (($sent % $every) -eq 0) {
            $row = Get-EntityRow (Get-SlotOf 0)
            if ($null -ne $row -and $row.Enabled -ne 0) {
                $sampled += [pscustomobject]@{ Lat = $row.Lat; Lon = $row.Lon; Alt = $row.Alt }
            }

            # The sky the *sim* holds, which is the whole point of a cloud
            # pattern: the region carrying it only proves the IG decoded it.
            # Sampled while the pattern is up, because the HOST takes it away
            # again before the run ends and a reading taken afterwards would be a
            # clear sky that is perfectly correct and proves nothing.  The region
            # row is sampled here for the same reason -- it is the world as the IG
            # resolved it, and by the end of the run the HOST has taken the
            # pattern away and the row is a clear sky.
            if ($sawPattern -and -not $sawPatternOff) {
                if ($null -eq $patternRow) {
                    $now = Get-EnvironmentRow
                    if ($null -ne $now -and ($now.Flags -band $EnvWeatherPattern) -ne 0 -and
                        $now.CloudType -ne 0 -and $now.Coverage -gt 0) {
                        $patternRow = $now
                    }
                }
                $st = Get-EnvironmentStatus
                if ($null -ne $st -and $st.CloudLayers -gt $simSkyLayers) {
                    $simSky = $st
                    $simSkyLayers = $st.CloudLayers
                }
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

    # The sky the sim holds *now*, after the HOST has taken its pattern away
    # again: the last thing the capture asked for is a clear sky, so this is what
    # a pattern turned off has to leave behind.
    if ($sawPatternOff) { $simSkyEnd = Get-EnvironmentStatus }

    $wanted = @($lastBySlot.Values | Where-Object { $_.State -eq 1 })
    $after = Get-WriterHealth

    Assert 'the HOST stream carried entities with a usable pose' ($lastBySlot.Count -gt 0) `
        ("{0} row(s) expected" -f $lastBySlot.Count)
    if ($lastBySlot.Count -eq 0) { return }

    # An object per driven entity -- and one that is actually being driven.  The
    # created counter is cumulative for the sim's lifetime and the sim is not
    # restarted between runs, so a second run against the same Prepar3D creates
    # nothing new: a CigiStarter that was force-stopped (its EXIT button is the
    # one path that retires the rows) leaves its objects in the sim, and the
    # add-on finds the slots already filled.  What has to hold either way is that
    # an object exists for every driven entity and is being positioned.
    $created = $after.ObjectsCreated - $before.ObjectsCreated
    Assert 'the sim holds an object for every driven entity' `
        (($created -ge $wanted.Count) -or ($after.Applied -eq $wanted.Count)) `
        ("created {0} this run, {1} applied of {2} driven (state Active)" -f $created, $after.Applied, $wanted.Count)
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

    # The pose is owned without pausing or slewing the sim (item 2).  The default
    # path -- a HOST-driven object moved between the sim's own frames -- never
    # asks either object to stop, and the frame-event check above already proves
    # the sim as a whole is running.  This is the per-object half of it.
    Assert 'no object is paused or in slew' (-not $after.ModePauseOrSlew) `
        ("objects reporting pause or slew: {0}" -f $(if ($after.ModePauseOrSlew) { 'yes' } else { 'none' }))

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
    if ($after.PhysicsFree) {
        Write-Host '    object: CigiStarter.Entity -- the physics-free ISimObject'
    } else {
        Write-Host '    object: the sim''s own, from a stock container title'
    }
    if ($after.DriftMaxMeters -lt 0.5 -and $after.DriftMaxDeg -lt 0.5) {
        Write-Host '    => the object stays on the pose it is written, to a few centimetres'
    } elseif ($after.PhysicsFree) {
        Write-Host '    => the physics-free object left the pose it was written; it must not'
        Write-Host '       (asserted below) -- it has no ground contact, so it must not be held'
        Write-Host '       above a below-ground command the way a stock object is.'
    } else {
        Write-Host '    => the object left the pose it was written.  Check the commanded altitude above:'
        Write-Host '       a stock object refuses a pose far below the ground and is held above it, so a'
        Write-Host '       command below the sim''s terrain reads as drift without the object moving.'
    }

    # Item 2's acceptance.  A physics-free object exists to hold the pose exactly
    # and to go where it is sent, so -- and only when one is in use -- the drift
    # is failed on rather than reported.  The stock object's refusal of a
    # below-ground pose is the proof: a refusal here did not get lifted, and on
    # any capture the physics-free drift has nowhere to hide.
    if ($after.PhysicsFree) {
        Assert 'the physics-free object holds the pose it is written' `
            ($after.DriftMaxMeters -lt 5.0 -and $after.DriftMaxDeg -lt 5.0) `
            ("worst drift {0:F2} m / {1:F2} deg over {2} comparisons" -f `
                $after.DriftMaxMeters, $after.DriftMaxDeg, $after.DriftSamples)
    } else {
        Write-Host ''
        Write-Host '  (the objects are the sim''s own, so the drift above is reported and not asserted;'
        Write-Host '   point PdkSimLink.ini at CigiStarter.Entity to check the physics-free object)'
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

    # The HOST's own eye (a View Control). The sim's view is a camera and not an
    # object, so the camera can only be where the HOST put it if the view crossed
    # the same link the poses did -- a wrong offset in the region shows up here as
    # a camera in the wrong place, which no pose check can see.
    $view = Get-ViewRow
    Assert 'the HOST''s own eye crossed the link' `
        ($lastView -ne $null -and $null -ne $view -and $view.Valid -ne 0) `
        ("the capture carried {0} view(s); the region's row valid={1}" -f `
            $(if ($null -eq $lastView) { 0 } else { 1 }), $(if ($null -eq $view) { 'unreadable' } else { $view.Valid }))
    if ($null -ne $lastView -and $null -ne $view -and $view.Valid -ne 0) {
        Write-Host ("    the HOST's eye: entity {0}  xyz {1:F2}/{2:F2}/{3:F2} m  rpy {4:F1}/{5:F1}/{6:F1}" -f `
            $view.EntityId, $view.X, $view.Y, $view.Z, $view.Roll, $view.Pitch, $view.Yaw)
        Assert 'the eye in the region is the eye the HOST sent' `
            ($view.EntityId -eq $lastView.EntityId -and
             (Compare-Number $view.X     $lastView.X     1e-6) -and
             (Compare-Number $view.Y     $lastView.Y     1e-6) -and
             (Compare-Number $view.Z     $lastView.Z     1e-6) -and
             (Compare-Number $view.Roll  $lastView.Roll  1e-6) -and
             (Compare-Number $view.Pitch $lastView.Pitch 1e-6) -and
             (Compare-Number $view.Yaw   $lastView.Yaw   1e-6)) `
            ("sent entity {0} xyz {1:F2}/{2:F2}/{3:F2} rpy {4:F1}/{5:F1}/{6:F1}" -f `
                $lastView.EntityId, $lastView.X, $lastView.Y, $lastView.Z,
                $lastView.Roll, $lastView.Pitch, $lastView.Yaw)
        Assert 'the eye is attached to the entity the camera rides' `
            ($view.EntityId -eq 0) `
            ("attached to entity {0}; the camera is on entity 0" -f $view.EntityId)
    }

    # The HOST's world (SimLinkIpc.h, kVersion 4): the clock, the air and the
    # cloud, which the IG resolves and publishes for the add-on to apply on the
    # sim's own clock and weather systems.  No sortie capture carries a Celestial
    # Sphere or a Weather Control -- those are the four *_replay.bin time captures
    # and the operator presets -- so each group is checked against what the
    # capture actually carried, and the absence of a clock is checked as a case of
    # its own: a capture that never sent one must not have moved the sim's clock.
    # The three `cloud pat*.bin` captures describe their sky a third way, with a
    # Component Control on the cloud entity, and are checked below.
    if ($sawEnv) {
        $envRow  = Get-EnvironmentRow
        $envStat = Get-EnvironmentStatus
        Assert 'the HOST''s world crossed the link' `
            ($null -ne $envRow -and $envRow.Generation -ne 0) `
            ("the region's world generation is {0}" -f `
                $(if ($null -eq $envRow) { 'unreadable' } else { $envRow.Generation }))
        if ($null -ne $envRow -and $envRow.Generation -ne 0) {
            Write-Host ("    the world: gen {0} flags 0x{1:X3}  {2:D2}:{3:D2}:{4:D2}  cloud {5} cov {6:F0} sev {7} precip {8}  {9:F1}C vis {10:F0}m {11:F0}mb" -f `
                $envRow.Generation, $envRow.Flags, $envRow.Hour, $envRow.Minute, $envRow.Second,
                $envRow.CloudType, $envRow.Coverage, $envRow.Severity, $envRow.PrecipType,
                $envRow.AirTempC, $envRow.VisibilityMeters, $envRow.BaroPressureMb)

            Assert 'the add-on applied the world the IG published' `
                ($null -ne $envStat -and $envStat.AppliedGeneration -eq $envRow.Generation -and $envStat.Applies -gt 0) `
                ("applied gen {0}, {1} application(s), lastError 0x{2:X8}" -f `
                    $(if ($null -eq $envStat) { '?' } else { $envStat.AppliedGeneration }),
                    $(if ($null -eq $envStat) { 0 } else { $envStat.Applies }),
                    $(if ($null -eq $envStat) { 0 } else { $envStat.LastError }))

            # The air the capture carried: the region's exact values are the
            # HOST's, converted to the units the METAR rounds around.
            if ($null -ne $lastAtmos) {
                Assert 'the air in the region is the air the HOST sent' `
                    ((Compare-Number $envRow.AirTempC $lastAtmos.TempC 1e-4) -and
                     (Compare-Number $envRow.VisibilityMeters $lastAtmos.Visibility 0.5) -and
                     (Compare-Number $envRow.BaroPressureMb $lastAtmos.Baro 0.01) -and
                     (Compare-Number $envRow.HorizWindMps $lastAtmos.HorizWind 1e-4) -and
                     ($envRow.Flags -band $EnvAtmosValid) -ne 0) `
                    ("sent {0:F4}C vis {1:F1}m baro {2:F2}mb wind {3:F3}m/s" -f `
                        $lastAtmos.TempC, $lastAtmos.Visibility, $lastAtmos.Baro, $lastAtmos.HorizWind)
            }

            # The clock, only where the capture carried one (the *_replay.bin
            # captures).  The region's clock is the HOST's, and the sim's own
            # answer -- GetGeneralTimeOfDay -- is the evidence that a commanded
            # 12:00 actually became day.
            if ($null -ne $lastTime) {
                Assert 'the HOST''s clock is the clock in the region' `
                    ($envRow.Hour -eq $lastTime.Hour -and $envRow.Minute -eq $lastTime.Minute -and
                     ($envRow.Flags -band $EnvTimeValid) -ne 0) `
                    ("sent {0:D2}:{1:D2}, region {2:D2}:{3:D2}" -f `
                        $lastTime.Hour, $lastTime.Minute, $envRow.Hour, $envRow.Minute)
                Assert 'the sim reports a time of day after the clock was set' `
                    ($null -ne $envStat -and $envStat.TimeOfDay -ge 0 -and $envStat.TimeOfDay -le 3) `
                    ("GetGeneralTimeOfDay -> {0} (0 dawn, 1 day, 2 dusk, 3 night; -1 never applied)" -f `
                        $(if ($null -eq $envStat) { '?' } else { $envStat.TimeOfDay }))
            } else {
                # The real HOST's case (AGENTS.md): an Atmosphere Control and no
                # clock at all, so the presence gate must keep the 00:00 default
                # from becoming a command.
                Assert 'a capture with no clock leaves the sim''s clock alone' `
                    (($envRow.Flags -band $EnvTimeValid) -eq 0) `
                    ("flags 0x{0:X3}: no kEnvTimeValid, so the clock was not applied" -f $envRow.Flags)
            }

            # No capture carries a Weather Control, so nothing should have been
            # applied from the weather group either -- no operator is at the
            # window during a replay.  kEnvApplyWeather is the bit the add-on
            # keys its METAR on, and a METAR always names a sky, so this is what
            # keeps a capture with an Atmosphere Control and no weather from
            # clearing the sim's cloud.  The three `cloud pat*.bin` captures are
            # the exception and are checked below: they describe their sky with a
            # Component Control instead of a Weather Control.
            if ($null -eq $lastWeather -and -not $sawPattern) {
                Assert 'a capture with no weather leaves the sim''s sky alone' `
                    ((($envRow.Flags -band $EnvWeatherValid) -eq 0) -and
                     (($envRow.Flags -band $EnvApplyWeather) -eq 0)) `
                    ("flags 0x{0:X3}: neither kEnvWeatherValid nor kEnvApplyWeather" -f $envRow.Flags)
            }

            # The HOST's cloud pattern: a Component Control on the cloud entity,
            # which is the only description of the sky in the three `cloud
            # pat*.bin` captures (no Weather Control, no clock, no atmosphere).
            # The region is where it has to land and the add-on has to be told to
            # apply it; the sim's own weather system is where it has to become
            # cloud.  Both are read *while the pattern is up* and not at the end:
            # the HOST takes its pattern away again a thousand datagrams later,
            # and the last thing the capture asks for is a clear sky -- correct,
            # and no evidence that the cloud was ever there.
            if ($sawPattern) {
                Assert 'the HOST''s cloud pattern is the sky in the region' `
                    ($null -ne $patternRow) `
                    $(if ($null -eq $patternRow) { 'no sample taken while the pattern was up had a sky: the pattern never reached the region' }
                      else { "flags 0x{0:X3}: cloud {1} cov {2:F0}% base {3:F1}m for pattern {4}" -f `
                          $patternRow.Flags, $patternRow.CloudType, $patternRow.Coverage,
                          $patternRow.BaseElevMeters, $lastPattern.State })
                if ($null -ne $patternRow) {
                    Assert 'it is marked as the HOST''s pattern and not an operator''s sky' `
                        ((($patternRow.Flags -band $EnvWeatherValid) -ne 0) -and
                         (($patternRow.Flags -band $EnvWeatherPattern) -ne 0) -and
                         (($patternRow.Flags -band $EnvWeatherManual) -eq 0) -and
                         (($patternRow.Flags -band $EnvApplyWeather) -ne 0) -and
                         (($patternRow.Flags -band $EnvAtmosValid) -eq 0) -and
                         (($patternRow.Flags -band $EnvTimeValid) -eq 0)) `
                        ("flags 0x{0:X3}: kEnvWeatherValid|kEnvWeatherPattern|kEnvApplyWeather, and no air or clock" -f $patternRow.Flags)

                    # The mapping is this IG's, but it is not arbitrary: the base
                    # is the pattern's own altitude and the coverage its footprint,
                    # so the numbers in the region have to be the numbers on the
                    # wire (metres, converted), not a default that happened to look
                    # like cloud.
                    $wantCoverage = [math]::Min(100.0, [math]::Max(30.0,
                        ($lastPattern.Data1 / 1852.0) * ($lastPattern.Data2 / 1852.0)))
                    Assert 'the cloud is that pattern''s footprint and altitude' `
                        ((Compare-Number $patternRow.Coverage $wantCoverage 0.5) -and
                         (Compare-Number $patternRow.BaseElevMeters $lastPattern.Data3 0.1)) `
                        ("sent {0:F0} x {1:F0} m at {2:F1} m -> coverage {3:F0}% base {4:F1} m" -f `
                            $lastPattern.Data1, $lastPattern.Data2, $lastPattern.Data3,
                            $patternRow.Coverage, $patternRow.BaseElevMeters)
                }

                # One pattern per capture (1, 2 or 3): a state read from the wrong
                # byte would vary from message to message, and a pattern index
                # mistaken for something else would arrive as several.
                Assert 'the capture''s pattern reached the link as one pattern' `
                    ($patternStates.Count -eq 1 -and $lastPattern.State -ge 1 -and $lastPattern.State -le 3) `
                    ("pattern states seen: {0}" -f (($patternStates.Keys | Sort-Object) -join ', '))

                # The sim's own weather system, which is the only witness that
                # the cloud was built rather than merely written down.
                Assert 'the HOST''s pattern became cloud in the sim' `
                    ($null -ne $simSky -and $simSky.CloudLayers -gt 0) `
                    ("the sim reported {0} cloud layer(s) during the replay{1}" -f `
                        $(if ($null -eq $simSky) { '?' } else { $simSky.CloudLayers }), `
                        $(if ($null -eq $simSky -or -not $simSky.SimMetar) { '' } else { " (station observation '$($simSky.SimMetar)')" }))
                if ($null -ne $simSky) {
                    Write-Host ("    the sim's own sky: {0} layer(s) type {1} cover {2} base {3:F0}m tops {4:F0}m precip {5}" -f `
                        $simSky.CloudLayers, $simSky.SimCloudType, $simSky.SimCloudCover, `
                        $simSky.SimCloudBaseM, $simSky.SimCloudTopsM, $simSky.SimPrecipType)
                    Write-Host ("    the sim's station observation: {0}" -f $simSky.SimMetar)

                    # The sim's layer is the pattern's, in the sim's own units --
                    # metres, so the altitude on the wire has to come back as it
                    # went out (a base converted one way too many would read as a
                    # tenth of it).
                    Assert 'the sim''s cloud carries the pattern''s base' `
                        (Compare-Number $simSky.SimCloudBaseM $lastPattern.Data3 6.0) `
                        ("the pattern's altitude {0:F1} m, the sim's layer {1:F1} m" -f `
                            $lastPattern.Data3, $simSky.SimCloudBaseM)
                    Assert 'the sim''s cloud is the pattern''s coverage bucket' `
                        ($simSky.SimCloudCover -gt 0 -and $simSky.SimCloudCover -le 8) `
                        ("the sim's cover value {0} (0-8 eighths)" -f $simSky.SimCloudCover)
                }

                # And the HOST taking the pattern away is a clear sky, not a
                # leftover: the state-0 message carries the same footprint, so a
                # mapping that keyed off the data rather than the state would
                # leave the cloud standing for the rest of the session.
                if ($sawPatternOff) {
                    Assert 'the HOST taking its pattern away clears the cloud in the sim' `
                        ($null -ne $simSkyEnd -and $simSkyEnd.CloudLayers -eq 0) `
                        ("after the state-0 message the sim reports {0} layer(s){1}" -f `
                            $(if ($null -eq $simSkyEnd) { '?' } else { $simSkyEnd.CloudLayers }), `
                            $(if ($null -eq $simSkyEnd -or -not $simSkyEnd.SimMetar) { '' } else { " (station observation '$($simSkyEnd.SimMetar)')" }))
                    Assert 'and the region carries the clear sky the HOST asked for' `
                        (($envRow.Flags -band $EnvWeatherValid) -ne 0 -and
                         ($envRow.Flags -band $EnvWeatherPattern) -ne 0 -and
                         $envRow.CloudType -eq 0) `
                        ("flags 0x{0:X3}: cloud {1} cov {2:F0}% -- present and empty, not absent" -f `
                            $envRow.Flags, $envRow.CloudType, $envRow.Coverage)
                }
            }
        }
    } else {
        Write-Host '  -this capture carries no environment message; the world path is not exercised'
    }

    # Movement: entity 0's published pose has to change over the run, or the
    # table is being filled once and left.  Whether that is askable depends on
    # the capture: the four *_replay.bin time captures are a single frozen pose,
    # so a link that showed one pose is exactly right for them.  The check is on
    # the capture's own motion -- if the capture moved, the region has to have
    # moved with it.
    $distinct = @($sampled | ForEach-Object { '{0:F6}/{1:F6}/{2:F2}' -f $_.Lat, $_.Lon, $_.Alt } | Select-Object -Unique)
    if ($sentPoses.Count -gt 1) {
        Assert 'the aircraft pose is moving through the link, not frozen' ($distinct.Count -gt 1) `
            ("{0} distinct poses sampled during the replay, of {1} the capture sent" -f $distinct.Count, $sentPoses.Count)
    } else {
        Write-Host '  -the capture is a single frozen pose; movement is not exercised'
    }
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

        # The eye goes with them, or a stopped IG would leave the camera holding
        # the last viewpoint of a run that is over.
        $view = Get-ViewRow
        Assert 'the HOST''s eye is taken back too' `
            ($null -ne $view -and $view.Valid -eq 0) `
            ("the region's view row valid={0}" -f $(if ($null -eq $view) { 'unreadable' } else { $view.Valid }))

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
