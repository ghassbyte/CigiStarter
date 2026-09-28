<#
.SYNOPSIS
    Build and run every contract test that does not need Prepar3D.

.DESCRIPTION
    The repo has two add-ons that talk to their peer through a block of shared
    memory, and both have a console harness that compiles the code either side of
    the contract and drives it without the sim:

      PdkSimLink       the HOST's entities -> sim objects (SimLinkIpc.h)
      PdkTerrainProbe  the sim's terrain -> HAT/HOT answers (TerrainProbeIpc.h)

    and one tool whose input is a file, not a peer, so it can be driven the same
    way:

      BinToScenario    a recorded .bin CIGI stream -> a Host Emulator .scp

    This is the one command that runs them all: it builds each harness and runs
    it, prints every harness's own output, and exits 0 only when they all pass.
    Run it before a commit, or in a loop while editing.

    Close Prepar3D before running it: a harness plays the sim's half of the
    shared region, so it must be the only writer of it.

.PARAMETER Configuration
    Build configuration to test.  Release by default.

.PARAMETER Platform
    Build platform.  x64 by default.

.PARAMETER NoBuild
    Run the executables already built, without rebuilding them.

.PARAMETER Only
    Run only these suites, by name (e.g. -Only PdkSimLink).  All of them by
    default.

.PARAMETER MsBuild
    Path to MSBuild.exe.  Found automatically when not given.

.EXAMPLE
    .\tools\test-all.ps1
.EXAMPLE
    .\tools\test-all.ps1 -Only PdkSimLink -NoBuild
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')] [string] $Configuration = 'Release',
    [string] $Platform = 'x64',
    [switch] $NoBuild,
    [string[]] $Only = @(),
    [string] $MsBuild = ''
)

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot          # this script lives in tools\

# Every SDK-free contract test in the repo.  Add a row when an add-on grows one.
$suites = @(
    [pscustomobject]@{
        Name    = 'PdkSimLink'
        Project = 'tools\PdkSimLink\test\PdkSimLinkTest.vcxproj'
        Exe     = 'tools\PdkSimLink\test\{0}\{1}\PdkSimLinkTest.exe'
    },
    [pscustomobject]@{
        Name    = 'PdkTerrainProbe'
        Project = 'tools\PdkTerrainProbe\test\PdkTerrainProbeTest.vcxproj'
        Exe     = 'tools\PdkTerrainProbe\test\{0}\{1}\PdkTerrainProbeTest.exe'
    },
    [pscustomobject]@{
        Name    = 'BinToScenario'
        Project = 'tools\BinToScenario\test\BinToScenarioTest.vcxproj'
        Exe     = 'tools\BinToScenario\test\{0}\{1}\BinToScenarioTest.exe'
    }
)

if ($Only.Count -gt 0) {
    $known = @($suites | ForEach-Object { $_.Name })
    foreach ($name in $Only) {
        if ($known -notcontains $name) {
            throw ("no such suite '{0}'; known: {1}" -f $name, ($known -join ', '))
        }
    }
    $suites = @($suites | Where-Object { $Only -contains $_.Name })
}

function Write-Verdict([string]$Text, [string]$Colour) {
    $line = ('  RESULT: {0}' -f $Text)
    $rule = '=' * $line.Length
    Write-Host ''
    Write-Host $rule -ForegroundColor $Colour
    Write-Host $line -ForegroundColor $Colour
    Write-Host $rule -ForegroundColor $Colour
}

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

$msbuild = ''
if (-not $NoBuild) { $msbuild = Find-MsBuild }

$results = @()
foreach ($suite in $suites) {
    Write-Host ''
    Write-Host ('=== {0} ===' -f $suite.Name)

    $project = Join-Path $repo $suite.Project
    $exe     = Join-Path $repo ($suite.Exe -f $Platform, $Configuration)

    if (-not $NoBuild) {
        & $msbuild $project "/p:Configuration=$Configuration" "/p:Platform=$Platform" /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) {
            Write-Host ('{0}: the test project did not build' -f $suite.Name) -ForegroundColor Red
            $results += [pscustomobject]@{ Suite = $suite.Name; Result = 'FAIL'; Exit = 1 }
            continue
        }
    }

    if (-not (Test-Path $exe)) {
        Write-Host ('{0}: no executable at {1}' -f $suite.Name, $exe) -ForegroundColor Red
        $results += [pscustomobject]@{ Suite = $suite.Name; Result = 'FAIL'; Exit = 1 }
        continue
    }

    & $exe
    $code = $LASTEXITCODE
    $verdict = 'FAIL'
    if ($code -eq 0) { $verdict = 'PASS' }
    $results += [pscustomobject]@{ Suite = $suite.Name; Result = $verdict; Exit = $code }
}

$failed = @($results | Where-Object { $_.Result -ne 'PASS' }).Count
$passed = @($results | Where-Object { $_.Result -eq 'PASS' }).Count

Write-Host ''
Write-Host '=== summary ==='
$results | Format-Table -AutoSize | Out-String | Write-Host

if ($failed -eq 0) {
    Write-Verdict ('PASS   --   {0} suite(s) passed' -f $passed) 'Green'
    exit 0
}
Write-Verdict ('FAIL   --   {0} passed, {1} failed' -f $passed, $failed) 'Red'
exit 1
