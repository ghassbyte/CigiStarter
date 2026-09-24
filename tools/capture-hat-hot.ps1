<#
.SYNOPSIS
    Watch the HAT/HOT exchange on a CIGI link and print every request and answer.

.DESCRIPTION
    CigiStarter's HAT/HOT answers ride on the same datagram as its Start-of-Frame
    reply (SOF followed by the response, as the reference Master IG framed them)
    and neither program prints the exchange anywhere. The only way to see it is
    to look at the wire, so this script captures the traffic between a CIGI HOST
    and CigiStarter with Wireshark's dumpcap, decodes it with tshark's built-in
    CIGI dissector, and prints the HAT/HOT Request messages (HOST to IG) and the
    answers (IG to HOST) with their terrain values.

    It is meant for a bench link where HOST and IG are both on this machine --
    the CIGI Host Emulator pointed at 127.0.0.1 with CigiStarter started as
    "CigiStarter.exe --host-ip 127.0.0.1 --ig-ip 127.0.0.1". It needs Npcap,
    which CigiStarter's fan-out needs anyway, and Wireshark's dumpcap/tshark. A
    default Npcap install does not ask for elevation.

.PARAMETER Seconds
    How long to capture for. Send the HAT/HOT requests from the HOST inside that
    window.

.PARAMETER Capture
    Decode a capture that already exists instead of taking a new one.

.PARAMETER Out
    Capture file to write. Defaults to %TEMP%\cigi-hat-hot.pcapng.

.PARAMETER IgPort
    CigiStarter's receive port: the HOST's flight data, HAT/HOT requests
    included, arrives here. Default 8000.

.PARAMETER HostPort
    The HOST's receive port: CigiStarter's SOF and HAT/HOT answers arrive here.
    Default 8001.

.PARAMETER Device
    Capture device. Defaults to Npcap's loopback adapter, where a same-machine
    bench link lives; "tshark -D" lists the others.

.PARAMETER PassThru
    Also return the decoded exchange as an object -- { Capture, Requests,
    Answers } -- for a script to assert on instead of a person reading the
    tables. Used by tools\test-hathot-emulator.ps1; the tables are printed
    either way.

.EXAMPLE
    # Watch the bench link for 30 seconds:
    .\capture-hat-hot.ps1 -Seconds 30

.EXAMPLE
    # Decode a capture taken earlier:
    .\capture-hat-hot.ps1 -Capture .\cap.pcapng

.EXAMPLE
    # On the rig, capturing on the IG PC's network adapter:
    .\capture-hat-hot.ps1 -Device '\Device\NPF_{...}' -Seconds 30
#>
[CmdletBinding()]
param(
    [int]    $Seconds  = 15,
    [string] $Capture  = '',
    [string] $Out      = "$env:TEMP\cigi-hat-hot.pcapng",
    [int]    $IgPort   = 8000,
    [int]    $HostPort = 8001,
    [string] $Device   = '\Device\NPF_Loopback',
    [switch] $PassThru
)

$ErrorActionPreference = 'Stop'

function Find-Tool([string]$Name) {
    foreach ($dir in @("$env:ProgramFiles\Wireshark", "${env:ProgramFiles(x86)}\Wireshark")) {
        $path = Join-Path $dir $Name
        if (Test-Path $path) { return $path }
    }
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    throw "$Name not found - install Wireshark, or put its folder on PATH."
}

$dumpcap = Find-Tool 'dumpcap.exe'
$tshark  = Find-Tool 'tshark.exe'

# ---------------------------------------------------------------- capture ----
if (-not $Capture) {
    $Capture = $Out
    $filter  = "udp port $IgPort or udp port $HostPort"
    Write-Host ("capturing on {0} for {1}s" -f $Device, $Seconds)
    Write-Host ("  filter: {0}" -f $filter)

    $errFile = [IO.Path]::ChangeExtension($Out, '.dumpcap.err')
    Remove-Item $Out, $errFile -Force -ErrorAction SilentlyContinue
    $dumpcapArgs = "-i `"$Device`" -f `"$filter`" -w `"$Out`" -a duration:$Seconds"
    $proc = Start-Process -FilePath $dumpcap -ArgumentList $dumpcapArgs `
                          -PassThru -WindowStyle Hidden -RedirectStandardError $errFile

    # dumpcap needs a moment to open the device and the capture file, and a
    # request sent in that gap would be missed. Wait for the file before saying
    # the window is open, so the operator triggers the HOST against a live
    # capture rather than against the call to Start-Process.
    $openBy = (Get-Date).AddSeconds(10)
    while (-not (Test-Path $Out) -and -not $proc.HasExited -and (Get-Date) -lt $openBy) {
        Start-Sleep -Milliseconds 100
    }
    Write-Host "  capturing now - send the HAT/HOT requests from the HOST"

    try {
        $deadline = (Get-Date).AddSeconds($Seconds + 5)
        while (-not $proc.HasExited -and (Get-Date) -lt $deadline) { Start-Sleep -Milliseconds 200 }
    } finally {
        if (-not $proc.HasExited) { $proc.Kill() }
    }

    # dumpcap chats about the device and the packet counts on stderr even when
    # all is well, so it is only quoted when the capture turned out to be empty:
    # a bad device name is exactly the case where its reason matters.
    if (-not (Test-Path $Out) -or (Get-Item $Out).Length -eq 0) {
        $why   = ''
        $text  = ''
        if (Test-Path $errFile) {
            Start-Sleep -Milliseconds 300     # dumpcap's redirect may still be flushing
            $text = Get-Content $errFile -Raw -ErrorAction SilentlyContinue
        }
        if ($text) { $why = " -- " + $text.Trim() }
        throw ("the capture produced no packet data (is Npcap installed? is '$Device' " +
               "right? try 'tshark -D')$why")
    }
}

# ----------------------------------------------------------------- decode ----
$decode = @('-r', $Capture, '-d', "udp.port==$IgPort,cigi", '-d', "udp.port==$HostPort,cigi")

function Format-Metres([string]$Value) {
    if (-not $Value) { return '' }
    [string]::Format([Globalization.CultureInfo]::InvariantCulture, '{0:F2}', [double]$Value)
}

# The numeric value of one field occurrence, or $null when the message does not
# carry that datum (a HAT-only response has no height at all, and so on).
function To-Metres([string]$Value) {
    if (-not $Value) { return $null }
    [double]::Parse($Value, [Globalization.CultureInfo]::InvariantCulture)
}

$type  = @{ '0' = 'HAT'; '1' = 'HOT'; '2' = 'Extended' }
$coord = @{ 'False' = 'Geodetic'; 'True' = 'Entity' }

Write-Host ""
Write-Host "=== HOST -> IG : HAT/HOT requests ==="
$requestLines = & $tshark @decode -Y 'cigi.hat_hot_request' -T fields `
    -e frame.time_relative -e cigi.hat_hot_request.hat_hot_id `
    -e cigi.hat_hot_request.type -e cigi.hat_hot_request.coordinate_system `
    -e cigi.hat_hot_request.update_period -e cigi.hat_hot_request.entity_id `
    -e cigi.hat_hot_request.lat_xoff -e cigi.hat_hot_request.lon_yoff `
    -e cigi.hat_hot_request.alt_zoff 2>$null
$requestLines = @($requestLines | Where-Object { $_ })
$requests = @()
if ($requestLines.Count -eq 0) {
    Write-Host "  (none)"
} else {
    $requests = @($requestLines | ForEach-Object {
        $f = $_ -split "`t"
        [pscustomobject]@{
            'time (s)' = $f[0]
            id         = $f[1]
            type       = $type[$f[2]]
            system     = $coord[$f[3]]
            'update per.' = $f[4]
            entity     = $f[5]
            x          = Format-Metres $f[6]
            y          = Format-Metres $f[7]
            z          = Format-Metres $f[8]
        }
    })
    $requests | Format-Table -AutoSize | Out-String | Write-Host
}

Write-Host "=== IG -> HOST : HAT/HOT answers ==="
$answerLines = & $tshark @decode -Y 'cigi.hat_hot_response or cigi.hat_hot_ext_response' -T fields `
    -e frame.time_relative `
    -e cigi.hat_hot_response.hat_hot_id -e cigi.hat_hot_response.type `
    -e cigi.hat_hot_response.valid -e cigi.hat_hot_response.height `
    -e cigi.hat_hot_ext_response.hat_hot_id -e cigi.hat_hot_ext_response.valid `
    -e cigi.hat_hot_ext_response.hat -e cigi.hat_hot_ext_response.hot 2>$null
$answerLines = @($answerLines | Where-Object { $_ })

# One datagram carries one answer per HAT/HOT id whose update period fell on
# that frame, so with several ids live a row holds several answers: tshark
# joins the occurrences of a field with a comma, in message order. Split every
# field and pair them up by occurrence index.
$answers = @()
foreach ($line in $answerLines) {
    $f = $line -split "`t"
    $eo = $f[5] -split ','
    $ev = $f[6] -split ','
    $eh = $f[7] -split ','
    $et = $f[8] -split ','
    for ($k = 0; $k -lt $eo.Count; $k++) {
        if (-not $eo[$k]) { continue }
        # HAT/HOT Extended Response (103): both datums in one message.
        $answers += [pscustomobject]@{
            'time (s)' = $f[0]
            id         = $eo[$k]
            answer     = 'HAT + HOT'
            valid      = $ev[$k]
            hat        = To-Metres $eh[$k]
            hot        = To-Metres $et[$k]
            value      = "HAT " + (Format-Metres $eh[$k]) + " m / HOT " +
                                     (Format-Metres $et[$k]) + " m"
        }
    }
    $po = $f[1] -split ','
    $pt = $f[2] -split ','
    $pv = $f[3] -split ','
    $pd = $f[4] -split ','
    for ($k = 0; $k -lt $po.Count; $k++) {
        if (-not $po[$k]) { continue }
        # HAT/HOT Response (102): only the datum that was asked for.
        $what  = if ($pt[$k] -eq 'True') { 'HOT' } else { 'HAT' }
        $datum = To-Metres $pd[$k]
        $hat   = $null
        $hot   = $null
        if ($what -eq 'HOT') { $hot = $datum } else { $hat = $datum }
        $answers += [pscustomobject]@{
            'time (s)' = $f[0]
            id         = $po[$k]
            answer     = $what
            valid      = $pv[$k]
            hat        = $hat
            hot        = $hot
            value      = (Format-Metres $pd[$k]) + " m $what"
        }
    }
}
if ($answers.Count -eq 0) {
    Write-Host "  (none)"
} else {
    # The numbers are on the objects for a script to assert on; the table stays
    # as it always was.
    $answers | Select-Object 'time (s)', id, answer, valid, value |
        Format-Table -AutoSize | Out-String | Write-Host
}

Write-Host ("{0} request(s), {1} answer(s)  [{2}]" -f $requests.Count, $answers.Count, $Capture)
if ($requests.Count -eq 0 -and $answerLines.Count -eq 0) {
    Write-Host "nothing on the link: is the HOST pointed at port $IgPort and CigiStarter running?"
} elseif ($requests.Count -eq 0) {
    Write-Host "no request inside the window, but the answers are there: they come from a request sent before it."
} elseif ($answerLines.Count -eq 0) {
    Write-Host "requests but no answers: is CigiStarter running, and is its HAT/HOT answering switched on?"
}

if ($PassThru) {
    Write-Output ([pscustomobject]@{
        Capture  = $Capture
        Requests = $requests
        Answers  = $answers
    })
}
