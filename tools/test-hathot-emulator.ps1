<#
.SYNOPSIS
    Drive the CIGI Host Emulator through a set of HAT/HOT requests and check
    that CigiStarter answers every one of them correctly.

.DESCRIPTION
    CigiStarter's HAT/HOT answers ride on the upstream SOF datagram and neither
    program prints them, so the only honest way to test them is to put a real
    CIGI HOST on the link, ask it for terrain, and read the answers off the wire.
    This script is that: it drives the Host Emulator's "Send HAT/HOT Request"
    dialog (the emulator has no command line or scripting interface that does
    not need the GUI), captures the exchange with Wireshark's dumpcap, decodes
    it with tools\capture-hat-hot.ps1, and asserts on the answers.

    The checks are the ones that matter for safety and for the CIGI framing:

      * a request is answered at all, and answered once (or every n-th frame
        when it carries an update period);
      * the answer echoes the request's HAT/HOT id, and the request type picks
        the right response message (102 for HAT/HOT, 103 for Extended);
      * HAT = request altitude - HOT, i.e. the two doubles are where the ICD
        says they are (getting the 3-byte gap wrong is what flies the aircraft
        into the ground);
      * Valid is set only when a terrain source could answer;
      * an entity-relative request is answered at the aircraft's position;
      * when the sim's own terrain probe is loaded, the height on the wire is
        the probe's reading for the very point that was asked about.

    It attaches to a bench that is already up, and starts whatever is missing
    (CigiStarter, and the Host Emulator from external\HostEmulator_Win32_v3.3.3)
    unless -NoLaunch is given. Starting the emulator writes its link setup to
    HKCU\Software\Boeing TSS\CIGI Host Emulator 3 first, because that is where
    the emulator reads it.

    On this machine's bench Prepar3D (Professional Plus) holds UDP 8000 for its
    own CIGI, so the bench link runs on 8010/8011:

        .\tools\test-hathot-emulator.ps1 -IgPort 8010 -HostPort 8011

    The exit code is 0 when every check passed (or was skipped with a reason)
    and 1 when any check failed.

.PARAMETER IgPort
    CigiStarter's receive port: the HOST's flight data and HAT/HOT requests
    arrive here. Default 8000, the deployment's.

.PARAMETER HostPort
    The HOST's receive port: CigiStarter's SOF and HAT/HOT answers arrive here.
    Default 8001.

.PARAMETER Device
    Capture device. Defaults to Npcap's loopback adapter, where a same-machine
    bench link lives; "tshark -D" lists the others.

.PARAMETER Seconds
    How long each capture runs (the request is sent once the capture is live).
    Default 6.

.PARAMETER CigiStarterExe
    CigiStarter to start when none is listening. Defaults to
    x64\Release\CigiStarter.exe under the repository root.

.PARAMETER EmulatorExe
    Host Emulator to start when none is running. Defaults to the vendored copy
    in external\HostEmulator_Win32_v3.3.3.

.PARAMETER NoLaunch
    Only test a bench that is already up: never start CigiStarter or the Host
    Emulator. The script fails with what is missing.

.PARAMETER Out
    Folder for the captures and dumpcap's stderr. Defaults to
    %TEMP%\cigi-hathot-tests.

.EXAMPLE
    # the full bench, start to finish (P3D running with the terrain probe, so
    # the 'sim is the terrain source' check runs too):
    .\tools\test-hathot-emulator.ps1 -IgPort 8010 -HostPort 8011

.EXAMPLE
    # no P3D needed: CigiStarter answers geodetic requests from the SRTM
    # database instead, and the probe check is skipped with that reason:
    .\tools\test-hathot-emulator.ps1 -IgPort 8010 -HostPort 8011 -NoLaunch
#>
[CmdletBinding()]
param(
    [int]    $IgPort   = 8000,
    [int]    $HostPort = 8001,
    [string] $Device   = '\Device\NPF_Loopback',
    [int]    $Seconds  = 6,
    [string] $CigiStarterExe = '',
    [string] $EmulatorExe    = '',
    [switch] $NoLaunch,
    [string] $Out = "$env:TEMP\cigi-hathot-tests"
)

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
if (-not $CigiStarterExe) { $CigiStarterExe = Join-Path $repo 'x64\Release\CigiStarter.exe' }
if (-not $EmulatorExe)    { $EmulatorExe    = Join-Path $repo 'external\HostEmulator_Win32_v3.3.3\Hemu3.exe' }
$decodeTool = Join-Path $PSScriptRoot 'capture-hat-hot.ps1'
$regKey     = 'HKCU:\Software\Boeing TSS\CIGI Host Emulator 3'

New-Item -ItemType Directory -Force -Path $Out | Out-Null

#-----------------------------------------------------------------------------
# The Host Emulator's dialog, over Win32 messages.
#
# The emulator's numeric fields ignore WM_SETTEXT (it validates on the keystroke
# that follows), so a field is set the way a person does: select all, backspace,
# then one WM_CHAR per character. The ids below are the emulator's own, read
# back from its dialog -- see the resource dump in the commit that added this.
#-----------------------------------------------------------------------------
Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
public static class Emu {
  public delegate bool EnumProc(IntPtr h, IntPtr p);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr p);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll", CharSet=CharSet.Unicode, EntryPoint="GetWindowTextW")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll", CharSet=CharSet.Unicode, EntryPoint="SendMessageW")] public static extern IntPtr SendMsg(IntPtr h, uint m, IntPtr w, StringBuilder l);
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  public static string Cls(IntPtr h) { var s = new StringBuilder(256); GetClassName(h, s, 256); return s.ToString(); }
  public static string Txt(IntPtr h) { var s = new StringBuilder(256); GetWindowText(h, s, 256); return s.ToString(); }
  public static string Val(IntPtr h) { var s = new StringBuilder(256); SendMsg(h, 0x000D, (IntPtr)256, s); return s.ToString(); }
  public static int Check(IntPtr h) { return (int)SendMessage(h, 0x00F0, IntPtr.Zero, IntPtr.Zero); }  // BM_GETCHECK
  public static List<IntPtr> Tops(uint pid) { var l = new List<IntPtr>(); EnumWindows((h,p)=>{ uint q; GetWindowThreadProcessId(h, out q); if (q==pid) l.Add(h); return true; }, IntPtr.Zero); return l; }
}

// The PdkTerrainProbe shared-memory table, to compare the wire answer against
// the reading the probe itself holds. The layout is the pinned one in
// TerrainProbeIpc.h: header 48 bytes, Query 32, Result 48, results[] at 8240.
public static class Shm {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)] public static extern IntPtr OpenFileMapping(uint a, bool i, string n);
  [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr MapViewOfFile(IntPtr h, uint a, uint hi, uint lo, UIntPtr b);
  [DllImport("kernel32.dll", SetLastError=true)] public static extern bool CloseHandle(IntPtr h);
}
"@

$script:emuDialog = [IntPtr]::Zero
$script:emuRadios = @{ 'HAT' = 1052; 'HOT' = 1053; 'Extended' = 1058; 'Geodetic' = 1059; 'Entity' = 1060 }
$script:emuFields = @{ Id = 1482; X = 1474; Y = 1475; Z = 1476; Period = 1526 }
$script:shmView   = $null

#-----------------------------------------------------------------------------
# Small helpers
#-----------------------------------------------------------------------------
function Find-Tool([string]$Name) {
    foreach ($dir in @("$env:ProgramFiles\Wireshark", "${env:ProgramFiles(x86)}\Wireshark")) {
        $path = Join-Path $dir $Name
        if (Test-Path $path) { return $path }
    }
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "$Name not found - install Wireshark, or put its folder on PATH."
}

# The pid that owns a UDP port, 0 when nothing does.
function Get-UdpOwner([int]$Port) {
    foreach ($line in @(netstat -ano -p udp | Select-String (":{0}\s" -f $Port))) {
        $parts = @($line.ToString().Trim() -split '\s+')
        if ($parts.Count -ge 4 -and $parts[1] -match (":{0}$" -f $Port)) {
            return [int]$parts[-1]
        }
    }
    return 0
}

$script:results     = @()
$script:currentTest = ''

function Assert([string]$Check, [bool]$Ok, [string]$Detail = '') {
    $verdict = 'FAIL'
    if ($Ok) { $verdict = 'PASS' }
    $suffix = ''
    if ($Detail) { $suffix = "  ($Detail)" }
    Write-Host ("    [{0}] {1}{2}" -f $verdict, $Check, $suffix)
    $script:results += [pscustomobject]@{
        Test = $script:currentTest; Result = $verdict; Check = $Check; Detail = $Detail
    }
}

function Skip([string]$Check, [string]$Why) {
    Write-Host ("    [SKIP] {0}  -- {1}" -f $Check, $Why)
    $script:results += [pscustomobject]@{
        Test = $script:currentTest; Result = 'SKIP'; Check = $Check; Detail = $Why
    }
}

function Run-Test([string]$Name, [scriptblock]$Body) {
    Write-Host ''
    Write-Host ("--- {0}" -f $Name)
    $script:currentTest = $Name
    try { & $Body }
    catch { Assert 'ran without throwing' $false $_.Exception.Message }
}

#-----------------------------------------------------------------------------
# The Host Emulator's Send HAT/HOT Request dialog
#-----------------------------------------------------------------------------
function Get-HatHotDialog {
    $p = Get-Process Hemu3 -ErrorAction SilentlyContinue
    if (-not $p) { return [IntPtr]::Zero }
    foreach ($h in [Emu]::Tops([uint32]$p.Id)) {
        if ([Emu]::Cls($h) -eq '#32770' -and [Emu]::Txt($h) -eq 'Send HAT/HOT Request Packet') { return $h }
    }
    return [IntPtr]::Zero
}

# Open it if it is not already up (Actions > Send Packet > HAT/HOT Request).
function Open-HatHotDialog {
    if ($script:emuDialog -ne [IntPtr]::Zero) { return $script:emuDialog }
    $dlg = Get-HatHotDialog
    if ($dlg -eq [IntPtr]::Zero) {
        $frame = (Get-Process Hemu3).MainWindowHandle
        if ($frame -eq [IntPtr]::Zero) { return [IntPtr]::Zero }
        [void][Emu]::PostMessage($frame, 0x0111, [IntPtr]0x8076, [IntPtr]::Zero)
        $by = (Get-Date).AddSeconds(5)
        while ((Get-Date) -lt $by) {
            Start-Sleep -Milliseconds 200
            $dlg = Get-HatHotDialog
            if ($dlg -ne [IntPtr]::Zero) { break }
        }
    }
    $script:emuDialog = $dlg
    return $dlg
}

function Set-EditValue([IntPtr]$Handle, [string]$Value) {
    [void][Emu]::SendMessage($Handle, 0x00B1, [IntPtr]::Zero, [IntPtr](-1))   # EM_SETSEL(0,-1)
    [void][Emu]::SendMessage($Handle, 0x0102, [IntPtr]8, [IntPtr]::Zero)      # WM_CHAR backspace
    foreach ($ch in $Value.ToCharArray()) {
        [void][Emu]::SendMessage($Handle, 0x0102, [IntPtr][int][char]$ch, [IntPtr]::Zero)
    }
}

function Compare-Number([string]$Got, [string]$Want) {
    $g = 0.0; $w = 0.0
    if (-not [double]::TryParse($Got, [ref]$g)) { return $false }
    if (-not [double]::TryParse($Want, [ref]$w)) { return $false }
    return [math]::Abs($g - $w) -lt 1e-6
}

# Fill the dialog in and confirm it took: a field the emulator refused (it
# validates keystrokes) would otherwise show up as a wrong request id much later.
function Set-HatHotFields {
    param(
        [string]$Type = 'Extended', [string]$System = 'Geodetic',
        [string]$Id, [string]$X, [string]$Y, [string]$Z, [string]$Period = '0'
    )
    $dlg = Open-HatHotDialog
    if ($dlg -eq [IntPtr]::Zero) { Write-Warning 'the HAT/HOT Request dialog is not open'; return $false }

    foreach ($name in @($Type, $System)) {
        $ctrl = $script:emuRadios[$name]
        if (-not $ctrl) { Write-Warning "unknown request '$name'"; return $false }
        [void][Emu]::SendMessage([Emu]::GetDlgItem($dlg, $ctrl), 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero)
        Start-Sleep -Milliseconds 150
    }
    $wanted = [ordered]@{ Id = $Id; X = $X; Y = $Y; Z = $Z; Period = $Period }
    foreach ($name in $wanted.Keys) {
        Set-EditValue ([Emu]::GetDlgItem($dlg, $script:emuFields[$name])) ([string]$wanted[$name])
    }
    Start-Sleep -Milliseconds 250

    $ok = $true
    foreach ($name in $wanted.Keys) {
        $got = [Emu]::Val([Emu]::GetDlgItem($dlg, $script:emuFields[$name]))
        if (-not (Compare-Number $got ([string]$wanted[$name]))) {
            Write-Warning ("${name}: wanted '$($wanted[$name])', the dialog holds '$got'")
            $ok = $false
        }
    }
    if ([Emu]::Check([Emu]::GetDlgItem($dlg, $script:emuRadios[$Type])) -ne 1)   { Write-Warning "$Type is not the selected request type"; $ok = $false }
    if ([Emu]::Check([Emu]::GetDlgItem($dlg, $script:emuRadios[$System])) -ne 1) { Write-Warning "$System is not the selected coordinate system"; $ok = $false }
    return $ok
}

function Send-HatHot {
    $dlg = Open-HatHotDialog
    if ($dlg -eq [IntPtr]::Zero) { throw 'the HAT/HOT Request dialog is not open' }
    [void][Emu]::SendMessage([Emu]::GetDlgItem($dlg, 1), 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero)   # the Send button
}

#-----------------------------------------------------------------------------
# Capture + decode
#-----------------------------------------------------------------------------
function Start-Capture([int]$Length) {
    $cap = Join-Path $Out ("{0}.pcapng" -f (Get-Date -Format 'HHmmssfff'))
    Remove-Item $cap, "$cap.err" -Force -ErrorAction SilentlyContinue
    $filter   = "udp port $IgPort or udp port $HostPort"
    $dumpArgs = "-i `"$Device`" -f `"$filter`" -w `"$cap`" -a duration:$Length"
    $proc = Start-Process -FilePath $script:dumpcap -ArgumentList $dumpArgs -PassThru `
                          -WindowStyle Hidden -RedirectStandardError "$cap.err"

    # dumpcap opens the device and the capture file a moment after it starts,
    # and a request sent in that gap is missed. Wait for the file (it is created
    # before capture begins) and give the capture a moment to be live.
    $by = (Get-Date).AddSeconds(8)
    while (-not (Test-Path $cap) -and -not $proc.HasExited -and (Get-Date) -lt $by) {
        Start-Sleep -Milliseconds 100
    }
    Start-Sleep -Milliseconds 1200
    return [pscustomobject]@{ Proc = $proc; Path = $cap; Length = $Length }
}

function Complete-Capture($Cap) {
    if (-not $Cap.Proc.HasExited) {
        if (-not $Cap.Proc.WaitForExit(($Cap.Length + 10) * 1000)) { $Cap.Proc.Kill() }
    }
    Start-Sleep -Milliseconds 300
    return $Cap.Path
}

function Receive-Exchange([string]$Capture) {
    # Write-Host inside the tool goes to the information stream, so 6>$null
    # keeps the suite's own output readable; -PassThru returns the decode.
    return (& $decodeTool -Capture $Capture -IgPort $IgPort -HostPort $HostPort -PassThru 6>$null)
}

# Both always hand back a real array. `return @(...)` alone would unwrap a
# single result to a scalar, and a lone PSCustomObject has no .Count at all
# (it is $null), which silently turns every "exactly once" check false.
function Get-Answers($Exchange, [int]$Id)  { return ,@($Exchange.Answers  | Where-Object { [int]$_.id -eq $Id }) }
function Get-Requests($Exchange, [int]$Id) { return ,@($Exchange.Requests | Where-Object { [int]$_.id -eq $Id }) }

#-----------------------------------------------------------------------------
# The PdkTerrainProbe shared-memory table (terrain::probe in TerrainProbeIpc.h)
#-----------------------------------------------------------------------------
function Open-ProbeShm {
    if ($null -ne $script:shmView) { return $script:shmView }
    $h = [Shm]::OpenFileMapping(0x0004, $false, 'Local\CigiStarter.TerrainProbe.v1')
    if ($h -eq [IntPtr]::Zero) { return $null }
    $view = [Shm]::MapViewOfFile($h, 0x0004, 0, 0, [UIntPtr]::Zero)
    if ($view -eq [IntPtr]::Zero) { [void][Shm]::CloseHandle($h); return $null }
    $script:shmView = $view
    return $view
}

function Probe-I32([int]$Offset) {
    return [Runtime.InteropServices.Marshal]::ReadInt32([IntPtr]::Add((Open-ProbeShm), $Offset))
}
function Probe-F64([int]$Offset) {
    return [BitConverter]::Int64BitsToDouble([Runtime.InteropServices.Marshal]::ReadInt64([IntPtr]::Add((Open-ProbeShm), $Offset)))
}

function Get-ProbeHeader {
    if ($null -eq (Open-ProbeShm)) { return $null }
    if ((Probe-I32 0) -ne 0x50425450) { return $null }   # 'PTBP'
    return [pscustomobject]@{
        Version    = Probe-I32 4
        Capacity   = Probe-I32 8
        TickRateHz = Probe-I32 12
        Alive      = Probe-I32 16
        Ticks      = Probe-I32 28
        QueryFails = Probe-I32 40
    }
}

# The row the probe holds for one HAT/HOT id (the id is the key it echoed back),
# read under the seqlock so a row caught mid-rewrite is not believed.
function Get-ProbeRow([int]$Key) {
    if ($null -eq (Open-ProbeShm)) { return $null }
    for ($try = 0; $try -lt 200; $try++) {
        $before = Probe-I32 36
        if ($before -band 1) { Start-Sleep -Milliseconds 2; continue }
        $row = $null
        for ($i = 0; $i -lt 256; $i++) {
            $o = 8240 + 48 * $i
            if ((Probe-I32 ($o + 24)) -eq $Key) {
                $row = [pscustomobject]@{
                    lat    = Probe-F64 $o
                    lon    = Probe-F64 ($o + 8)
                    hot    = Probe-F64 ($o + 16)
                    key    = $Key
                    status = Probe-I32 ($o + 28)
                }
                break
            }
        }
        if ($null -ne $row -and (Probe-I32 36) -eq $before) { return $row }
    }
    return $null
}

#-----------------------------------------------------------------------------
# Bench bring-up
#-----------------------------------------------------------------------------
function Set-EmulatorLink {
    if (-not (Test-Path $regKey)) { New-Item -Path $regKey -Force | Out-Null }
    Set-ItemProperty -Path $regKey -Name 'Remote IP Addr'     -Value '127.0.0.1'
    Set-ItemProperty -Path $regKey -Name 'Remote Port'        -Value "$IgPort"
    Set-ItemProperty -Path $regKey -Name 'Local Port'         -Value "$HostPort"
    Set-ItemProperty -Path $regKey -Name 'CIGI Minor Version' -Value '3'
    Set-ItemProperty -Path $regKey -Name 'Big Endian'         -Value '0'
}

function Initialize-Bench {
    Write-Host ("link: IG {0} (CigiStarter)  <->  HOST {1} (Host Emulator)   device {2}" -f $IgPort, $HostPort, $Device)

    $owner = Get-UdpOwner $IgPort
    if ($owner -ne 0) {
        $proc = Get-Process -Id $owner -ErrorAction SilentlyContinue
        $name = 'another program'
        if ($proc) { $name = $proc.ProcessName }
        if (-not $proc -or $name -ne 'CigiStarter') {
            throw ("UDP port {0} is held by {1} (pid {2}), not CigiStarter - stop it, or pass a different -IgPort" -f $IgPort, $name, $owner)
        }
        Write-Host ("  CigiStarter is listening on {0} (pid {1})" -f $IgPort, $owner)
    } elseif ($NoLaunch) {
        throw "-NoLaunch: nothing is listening on UDP port $IgPort"
    } else {
        if (-not (Test-Path $CigiStarterExe)) { throw "CigiStarter.exe not found at $CigiStarterExe - build it first (see README.md)" }
        Write-Host ("  starting {0}" -f $CigiStarterExe)
        Start-Process -FilePath $CigiStarterExe -WorkingDirectory (Split-Path -Parent $CigiStarterExe) `
            -ArgumentList @('--host-ip','127.0.0.1','--ig-ip','127.0.0.1','--ig-port',"$IgPort",'--host-port',"$HostPort")
        $by = (Get-Date).AddSeconds(15)
        while ((Get-UdpOwner $IgPort) -eq 0 -and (Get-Date) -lt $by) { Start-Sleep -Milliseconds 250 }
        if ((Get-UdpOwner $IgPort) -eq 0) { throw "CigiStarter did not come up on UDP port $IgPort" }
        Write-Host ("  CigiStarter is listening on {0} (pid {1})" -f $IgPort, (Get-UdpOwner $IgPort))
    }

    $hemu = Get-Process Hemu3 -ErrorAction SilentlyContinue
    if ($hemu) {
        Write-Host ("  Host Emulator is running (pid {0})" -f $hemu.Id)
        $saved = (Get-ItemProperty -Path $regKey -Name 'Remote Port' -ErrorAction SilentlyContinue).'Remote Port'
        if ($saved -and $saved -ne "$IgPort") {
            Write-Warning ("the emulator's saved Remote Port is $saved but CigiStarter is on $IgPort - if the running emulator was not re-pointed, no request will reach the IG")
        }
    } elseif ($NoLaunch) {
        throw "-NoLaunch: the Host Emulator (Hemu3) is not running"
    } else {
        if (-not (Test-Path $EmulatorExe)) { throw "Host Emulator not found at $EmulatorExe" }
        Write-Host ("  starting {0}" -f $EmulatorExe)
        Set-EmulatorLink
        Start-Process -FilePath $EmulatorExe -WorkingDirectory (Split-Path -Parent $EmulatorExe)
        $by = (Get-Date).AddSeconds(20)
        $hemu = $null
        while ((Get-Date) -lt $by) {
            $hemu = Get-Process Hemu3 -ErrorAction SilentlyContinue
            if ($hemu -and $hemu.MainWindowHandle -ne 0) { break }
            Start-Sleep -Milliseconds 300
        }
        if (-not $hemu) { throw "the Host Emulator did not start" }
        Write-Host ("  Host Emulator is up (pid {0})" -f $hemu.Id)
    }

    if ((Open-HatHotDialog) -eq [IntPtr]::Zero) {
        throw "could not open the Host Emulator's 'Send HAT/HOT Request' dialog"
    }
    Write-Host '  the Send HAT/HOT Request dialog is open'
}

#-----------------------------------------------------------------------------
# Preflight
#-----------------------------------------------------------------------------
$script:dumpcap = Find-Tool 'dumpcap.exe'
$null = Find-Tool 'tshark.exe'
if (-not (Test-Path $decodeTool)) { throw "the decode tool is missing: $decodeTool" }
$script:hasP3d = [bool](Get-Process Prepar3D -ErrorAction SilentlyContinue)

Write-Host 'CigiStarter HAT/HOT bench tests'
Initialize-Bench
$probe = Get-ProbeHeader
if ($null -ne $probe -and $probe.Alive -eq 1) {
    Write-Host ("  terrain probe: loaded, ticking at {0} Hz, {1} query failure(s)" -f $probe.TickRateHz, $probe.QueryFails)
} else {
    Write-Host '  terrain probe: not loaded (answers come from the terrain database / FSUIPC)'
}

# The points below: Atang Senjaya, Halim Perdanakusuma and Soekarno-Hatta. Any
# two distinct places will do; these are the ones the bench tiles and the sim
# both cover, so the checks that need a terrain source always have one.
$Atang    = @{ X = '-6.5428'; Y = '106.7566' }
$Halim    = @{ X = '-6.2664'; Y = '106.8913' }
$Soekarno = @{ X = '-6.1341'; Y = '106.6552' }
$Alt      = '3500'

#-----------------------------------------------------------------------------
# 1. Extended response: both datums, in the ICD's places, once only.
#-----------------------------------------------------------------------------
Run-Test 'extended / geodetic / one-shot' {
    if (-not (Set-HatHotFields -Type Extended -System Geodetic -Id 21 -X $Atang.X -Y $Atang.Y -Z $Alt -Period 0)) {
        Assert 'the emulator took the requested fields' $false 'the dialog read back different values'
        return
    }
    # Start-Capture waits for the capture to be live before it returns, because
    # a request sent in dumpcap's startup gap is missed.
    $cap = Start-Capture $Seconds
    Send-HatHot
    $ex = Receive-Exchange (Complete-Capture $cap)

    $req = Get-Requests $ex 21
    Assert 'the emulator sent the request (id 21)' ($req.Count -eq 1) ("saw {0}" -f $req.Count)
    if ($req.Count -eq 1) {
        Assert 'the request is Extended'  ($req[0].type   -eq 'Extended') ($req[0].type)
        Assert 'the request is Geodetic'  ($req[0].system -eq 'Geodetic') ($req[0].system)
    }

    $ans = Get-Answers $ex 21
    Assert 'answered exactly once' ($ans.Count -eq 1) ("{0} answer(s)" -f $ans.Count)
    if ($ans.Count -ne 1) { return }
    $a = $ans[0]
    Assert 'the answer is a HAT/HOT Extended Response (103)' ($a.answer -eq 'HAT + HOT') ($a.answer)
    Assert 'Valid is set' ($a.valid -eq 'True') ($a.valid)
    Assert 'HOT is a real height' ($null -ne $a.hot -and $a.hot -gt -500.0 -and $a.hot -lt 5000.0) ("HOT {0:N2} m" -f $a.hot)
    Assert 'HAT = request altitude - HOT' ($null -ne $a.hat -and [math]::Abs($a.hat - ([double]$Alt - $a.hot)) -le 0.05) `
           ("HAT {0:N2} m, HOT {1:N2} m" -f $a.hat, $a.hot)

    # A one-shot request is answered once. What is on the link afterwards comes
    # from the period-6 request the update-period test leaves standing, so this
    # capture must be taken with no further send.
    $again = Get-Answers (Receive-Exchange (Complete-Capture (Start-Capture 3))) 21
    Assert 'not answered again (updatePeriod 0)' ($again.Count -eq 0) ("{0} further answer(s)" -f $again.Count)
}

#-----------------------------------------------------------------------------
# 2. HOT-only: the 16-byte response, carrying only the datum asked for.
#-----------------------------------------------------------------------------
Run-Test 'hot / geodetic' {
    if (-not (Set-HatHotFields -Type HOT -System Geodetic -Id 22 -X $Halim.X -Y $Halim.Y -Z $Alt -Period 0)) {
        Assert 'the emulator took the requested fields' $false 'the dialog read back different values'
        return
    }
    $cap = Start-Capture $Seconds
    Send-HatHot
    $ex = Receive-Exchange (Complete-Capture $cap)

    $req = Get-Requests $ex 22
    Assert 'the request is HOT' ($req.Count -ge 1 -and $req[0].type -eq 'HOT') ("{0} request(s)" -f $req.Count)

    $ans = Get-Answers $ex 22
    Assert 'answered exactly once' ($ans.Count -eq 1) ("{0} answer(s)" -f $ans.Count)
    if ($ans.Count -ne 1) { return }
    $a = $ans[0]
    Assert 'the answer is a plain HAT/HOT Response (102)' ($a.answer -eq 'HOT') ($a.answer)
    Assert 'Valid is set' ($a.valid -eq 'True') ($a.valid)
    Assert 'HOT is a real height' ($null -ne $a.hot -and $a.hot -gt -500.0 -and $a.hot -lt 5000.0) ("HOT {0:N2} m" -f $a.hot)
    Assert 'a HOT-only answer carries no HAT' ($null -eq $a.hat)
}

#-----------------------------------------------------------------------------
# 3. HAT-only: the same response, with the HAT datum instead.
#-----------------------------------------------------------------------------
Run-Test 'hat / geodetic' {
    if (-not (Set-HatHotFields -Type HAT -System Geodetic -Id 23 -X $Soekarno.X -Y $Soekarno.Y -Z $Alt -Period 0)) {
        Assert 'the emulator took the requested fields' $false 'the dialog read back different values'
        return
    }
    $cap = Start-Capture $Seconds
    Send-HatHot
    $ex = Receive-Exchange (Complete-Capture $cap)

    $req = Get-Requests $ex 23
    Assert 'the request is HAT' ($req.Count -ge 1 -and $req[0].type -eq 'HAT') ("{0} request(s)" -f $req.Count)

    $ans = Get-Answers $ex 23
    Assert 'answered exactly once' ($ans.Count -eq 1) ("{0} answer(s)" -f $ans.Count)
    if ($ans.Count -ne 1) { return }
    $a = $ans[0]
    Assert 'the answer is a plain HAT/HOT Response (102)' ($a.answer -eq 'HAT') ($a.answer)
    Assert 'Valid is set' ($a.valid -eq 'True') ($a.valid)
    Assert 'HAT is a real height' ($null -ne $a.hat -and $a.hat -gt 0.0 -and $a.hat -lt 5000.0) ("HAT {0:N2} m" -f $a.hat)
    Assert 'a HAT-only answer carries no HOT' ($null -eq $a.hot)
}

#-----------------------------------------------------------------------------
# 4. The update period: one request, re-answered by the IG every n-th frame.
#-----------------------------------------------------------------------------
Run-Test 'update period is honoured' {
    if (-not (Set-HatHotFields -Type Extended -System Geodetic -Id 24 -X $Atang.X -Y $Atang.Y -Z $Alt -Period 6)) {
        Assert 'the emulator took the requested fields' $false 'the dialog read back different values'
        return
    }
    $cap = Start-Capture $Seconds
    Send-HatHot
    $ex = Receive-Exchange (Complete-Capture $cap)

    $ans = Get-Answers $ex 24
    Assert 're-answered on its period from one request' ($ans.Count -ge 5) ("{0} answers" -f $ans.Count)
    Assert 'every answer is Valid' (@($ans | Where-Object { $_.valid -ne 'True' }).Count -eq 0)
    $terrain = @($ans | Select-Object -ExpandProperty hot -Unique)
    Assert 'every answer reports the same terrain' ($terrain.Count -le 1) ("{0} distinct height(s)" -f $terrain.Count)

    # Leave the bench as it was found: a fresh request for the same id with a
    # zero period replaces the standing one and is answered once, then dropped.
    if (Set-HatHotFields -Type Extended -System Geodetic -Id 24 -X $Atang.X -Y $Atang.Y -Z $Alt -Period 0) {
        Send-HatHot
        Start-Sleep -Seconds 1
    }
}

#-----------------------------------------------------------------------------
# 5. Several requests in one capture: the answers share datagrams.
#-----------------------------------------------------------------------------
Run-Test 'several requests at once' {
    $points = @(
        @{ Id = 25; P = $Halim },
        @{ Id = 26; P = $Atang },
        @{ Id = 27; P = $Soekarno }
    )
    $cap = Start-Capture $Seconds
    $set = $true
    foreach ($r in $points) {
        if (-not (Set-HatHotFields -Type Extended -System Geodetic -Id $r.Id -X $r.P.X -Y $r.P.Y -Z $Alt -Period 0)) { $set = $false; break }
        Send-HatHot
        Start-Sleep -Milliseconds 400
    }
    $ex = Receive-Exchange (Complete-Capture $cap)
    Assert 'the emulator took all three requests' $set
    foreach ($r in $points) {
        $a = Get-Answers $ex $r.Id
        $valid = '-'
        if ($a.Count -gt 0) { $valid = $a[0].valid }
        Assert ("id {0} is answered once and Valid" -f $r.Id) ($a.Count -eq 1 -and $valid -eq 'True') ("{0} answer(s), valid={1}" -f $a.Count, $valid)
    }
}

#-----------------------------------------------------------------------------
# 6. Entity-relative: answered at the aircraft's position, so it needs the sim.
#-----------------------------------------------------------------------------
Run-Test 'entity-relative request' {
    if (-not $script:hasP3d) {
        Skip 'answered at the aircraft position' 'Prepar3D is not running, so there is no ownship position'
        return
    }
    if (-not (Set-HatHotFields -Type Extended -System Entity -Id 28 -X 0 -Y 0 -Z 0 -Period 0)) {
        Assert 'the emulator took the requested fields' $false 'the dialog read back different values'
        return
    }
    $cap = Start-Capture $Seconds
    Send-HatHot
    $ex = Receive-Exchange (Complete-Capture $cap)

    $ans = Get-Answers $ex 28
    Assert 'answered exactly once' ($ans.Count -eq 1) ("{0} answer(s)" -f $ans.Count)
    if ($ans.Count -ne 1) { return }
    $a = $ans[0]
    Assert 'Valid is set (the aircraft position is known)' ($a.valid -eq 'True') ($a.valid)
    Assert 'HOT is the terrain under the aircraft' ($null -ne $a.hot -and $a.hot -gt -500.0 -and $a.hot -lt 5000.0) ("HOT {0:N2} m" -f $a.hot)
}

#-----------------------------------------------------------------------------
# 7. The sim is the terrain source: the wire height is the probe's own reading.
#-----------------------------------------------------------------------------
Run-Test 'the sim is the terrain source' {
    $hdr = Get-ProbeHeader
    if ($null -eq $hdr -or $hdr.Alive -ne 1) {
        Skip "CigiStarter's HOT equals the probe's reading" 'the PdkTerrainProbe add-on is not loaded in Prepar3D'
        return
    }
    if (-not (Set-HatHotFields -Type Extended -System Geodetic -Id 29 -X $Halim.X -Y $Halim.Y -Z $Alt -Period 0)) {
        Assert 'the emulator took the requested fields' $false 'the dialog read back different values'
        return
    }
    $cap = Start-Capture $Seconds
    Send-HatHot
    $ex = Receive-Exchange (Complete-Capture $cap)

    $ans = Get-Answers $ex 29
    Assert 'answered exactly once' ($ans.Count -eq 1) ("{0} answer(s)" -f $ans.Count)
    if ($ans.Count -ne 1) { return }
    $a = $ans[0]
    $row = Get-ProbeRow 29
    Assert 'the probe holds a reading for the request id' ($null -ne $row -and $row.status -eq 1) ("status {0}" -f $(if ($row) { $row.status } else { 'no row' }))
    if ($null -eq $row) { return }
    Assert "CigiStarter's HOT equals the probe's own reading" ([math]::Abs($a.hot - $row.hot) -le 0.05) ("wire {0:N2} m, probe {1:N2} m" -f $a.hot, $row.hot)
    Assert 'the probe row is the point that was asked about' `
        ([math]::Abs($row.lat - [double]$Halim.X) -lt 1e-4 -and [math]::Abs($row.lon - [double]$Halim.Y) -lt 1e-4) `
        ("probe lat {0:N5} lon {1:N5}" -f $row.lat, $row.lon)
}

#-----------------------------------------------------------------------------
# Summary
#-----------------------------------------------------------------------------
$passed  = @($script:results | Where-Object { $_.Result -eq 'PASS' }).Count
$failed  = @($script:results | Where-Object { $_.Result -eq 'FAIL' }).Count
$skipped = @($script:results | Where-Object { $_.Result -eq 'SKIP' }).Count

Write-Host ''
Write-Host '=== checks ==='
if ($failed -gt 0) {
    Write-Host 'failures:'
    $script:results | Where-Object { $_.Result -eq 'FAIL' } | Format-Table -AutoSize | Out-String | Write-Host
}
$script:results | Format-Table -AutoSize | Out-String | Write-Host
Write-Host ''
Write-Host ("{0} passed, {1} failed, {2} skipped   (captures in {3})" -f $passed, $failed, $skipped, $Out)

if ($failed -gt 0) { exit 1 }
exit 0
