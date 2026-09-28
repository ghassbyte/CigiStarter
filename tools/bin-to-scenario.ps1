<#
.SYNOPSIS
    Turn recorded CIGI captures (.bin) into CIGI Host Emulator scripts (.scp).

.DESCRIPTION
    Builds BinToScenario (tools\BinToScenario) and runs it over one or more
    captures, writing a .scp beside each input (or into -Out).

    The .bin files are recordings of the real HOST->IG stream -- made from a
    Wireshark capture by pcap_to_bin.py, in the project's TAP folder.  The CIGI
    Host Emulator cannot read one: it runs definition tables (.def) and a script.
    So this rewrites a capture as the script that drives the emulator through the
    same scenario -- the same entities in the same places, the same time of day,
    the same warm-up environment -- with no third program in the loop.

    What is translated, and what is deliberately not, is in
    tools\BinToScenario\README.md; every script's own header repeats it for that
    particular capture.

    Run this, then start the emulator and load the .scp from its script dialog.
    Scene commands land where the capture had them: a feed that sets the time of
    day mid-run changes it mid-scenario.  Move the line to the top of the script
    if you want it from the start.

.PARAMETER Bin
    One or more .bin captures to convert.

.PARAMETER Out
    Directory to write the .scp files into.  Beside each input by default.

.PARAMETER EntityType
    Emulator entity type for the ADD_ENTITY lines.  Default 118, the f18c in the
    emulator's own Default\Entities.def.

.PARAMETER NoCompress
    One FRAME per datagram, instead of folding runs of unchanged frames into
    FRAME_WAIT.

.PARAMETER Configuration
    Build configuration of the tool.  Release by default.

.PARAMETER NoBuild
    Use the executable already built.

.PARAMETER MsBuild
    Path to MSBuild.exe.  Found automatically when not given.

.EXAMPLE
    .\tools\bin-to-scenario.ps1 -Bin 'E:\03_project\Super Puma\Visual\Local\hlm.bin'

.EXAMPLE
    .\tools\bin-to-scenario.ps1 -Bin (Get-ChildItem 'E:\03_project\Super Puma\Visual\Local' -Filter *.bin).FullName
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)] [string[]] $Bin,
    [string] $Out = '',
    [int] $EntityType = 118,
    [switch] $NoCompress,
    [ValidateSet('Debug', 'Release')] [string] $Configuration = 'Release',
    [switch] $NoBuild,
    [string] $MsBuild = ''
)

$ErrorActionPreference = 'Stop'

$repo    = Split-Path -Parent $PSScriptRoot          # this script lives in tools\
$project = Join-Path $repo 'tools\BinToScenario\BinToScenario.vcxproj'
$exe     = Join-Path $repo ('x64\{0}\BinToScenario.exe' -f $Configuration)

function Find-MsBuild {
    if ($MsBuild) {
        if (-not (Test-Path $MsBuild)) { throw "MSBuild not found at '$MsBuild'." }
        return $MsBuild
    }

    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    # The Build Tools installation is not on PATH by default; vswhere is the
    # supported way to find it.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild `
                            -find 'MSBuild\**\Bin\MSBuild.exe' 2>$null |
                 Select-Object -First 1
        if ($found -and (Test-Path $found)) { return $found }
    }

    throw 'MSBuild not found.  Pass -MsBuild <path>, or put it on PATH.'
}

if (-not $NoBuild) {
    $msbuild = Find-MsBuild
    & $msbuild $project "/p:Configuration=$Configuration" "/p:Platform=x64" /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw 'BinToScenario did not build.' }
} elseif (-not (Test-Path $exe)) {
    throw ("no executable at {0}; run without -NoBuild to build it." -f $exe)
}

if ($Out -and -not (Test-Path $Out)) {
    New-Item -ItemType Directory -Force -Path $Out | Out-Null
}

foreach ($one in $Bin) {
    if (-not (Test-Path $one)) { throw "no such capture: $one" }

    $toolArgs = @($one)
    if ($Out) {
        $toolArgs += Join-Path $Out ([IO.Path]::GetFileNameWithoutExtension($one) + '.scp')
    }
    $toolArgs += @('--entity-type', $EntityType)
    if ($NoCompress) { $toolArgs += '--no-compress' }

    & $exe @toolArgs
    if ($LASTEXITCODE -ne 0) { throw "BinToScenario failed on $one" }
}
