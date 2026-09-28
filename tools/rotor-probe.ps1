<#
.SYNOPSIS
    Build and run the SimConnect rotor/lights bench probe.

.DESCRIPTION
    tools\RotorProbe\rotor_probe.cpp is a standalone bench diagnostic, not part
    of any solution and not part of test-all.ps1: it links SimConnect instead of
    the PDK, takes no CIGI, and needs Prepar3D v6 *running* to do anything.

    It answers the questions the offline suite cannot, about the AS332 running
    as the sim's user aircraft:

      * does the stock rotor keep turning while the sim is paused, and while
        slew is engaged?
      * which of ROTOR RPM PCT:1 / ROTOR ROTATION ANGLE:1 / ENG ROTOR RPM:1
        actually move?
      * do the standard LIGHT NAV / BEACON / LANDING vars drive the model's
        exterior lights, and can they be written from outside?

    The script compiles the probe with cl.exe and then runs it with whatever
    arguments you passed through.

.PARAMETER BuildOnly
    Compile, do not run.

.PARAMETER SdkRoot
    The Prepar3D v6 SDK root.  Found automatically when not given: the installed
    SDK first, then the copy vendored in ..\CigiP3dSimConnect\external.

.PARAMETER ProbeArgs
    Everything else is handed to rotor_probe.exe, e.g. `test` or `lights on`.

.EXAMPLE
    .\tools\rotor-probe.ps1 report
.EXAMPLE
    .\tools\rotor-probe.ps1 watch 15
.EXAMPLE
    .\tools\rotor-probe.ps1 test
#>
[CmdletBinding()]
param(
    [switch] $BuildOnly,
    [string] $SdkRoot = '',
    [Parameter(ValueFromRemainingArguments = $true)] [string[]] $ProbeArgs = @()
)

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot          # this script lives in tools\
$tool = Join-Path $repo 'tools\RotorProbe'
$out  = Join-Path $tool 'x64\Release'
$exe  = Join-Path $out 'rotor_probe.exe'

# ---------------------------------------------------------------------------
#  The toolchain
# ---------------------------------------------------------------------------

function Find-VcVars {
    # Prefer the VS 2022 install the sibling SimConnect bridge builds with, then
    # let vswhere find whatever else is present.
    $known = @(
        (Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'),
        (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
    )
    foreach ($p in $known) { if (Test-Path $p) { return $p } }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
                            -find 'VC\Auxiliary\Build\vcvars64.bat' 2>$null |
                 Select-Object -First 1
        if ($found -and (Test-Path $found)) { return $found }
    }
    throw 'vcvars64.bat not found.  Pass no -Sdk and install the VS C++ toolchain, or edit this script.'
}

# ---------------------------------------------------------------------------
#  The SDK.  The installed layout is <root>\inc\SimConnect and
#  <root>\lib\SimConnect; the vendored one is <root>\inc and <root>\lib.
# ---------------------------------------------------------------------------

function Resolve-Sdk([string] $Root) {
    if (-not (Test-Path $Root)) { return $null }

    $header = @(
        (Join-Path $Root 'inc\SimConnect\SimConnect.h'),
        (Join-Path $Root 'inc\SimConnect.h')
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1

    $lib = @(
        (Join-Path $Root 'lib\SimConnect\SimConnect.lib'),
        (Join-Path $Root 'lib\SimConnect.lib')
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1

    if (-not $header -or -not $lib) { return $null }
    return [pscustomobject]@{
        Inc = Split-Path -Parent $header
        Lib = Split-Path -Parent $lib
        Root = $Root
    }
}

function Find-Sdk {
    if ($SdkRoot) {
        $r = Resolve-Sdk $SdkRoot
        if (-not $r) { throw "no SimConnect SDK at '$SdkRoot' (want inc\SimConnect\SimConnect.h and lib\SimConnect\SimConnect.lib)." }
        return $r
    }

    $candidates = @()
    $lm = Join-Path ${env:ProgramFiles} 'Lockheed Martin'
    if (Test-Path $lm) {
        $candidates += Get-ChildItem $lm -Directory -Filter 'Prepar3D v6 SDK*' -ErrorAction SilentlyContinue |
                       Sort-Object Name -Descending | ForEach-Object { $_.FullName }
    }
    $candidates += (Join-Path $repo '..\CigiP3dSimConnect\external\SimConnect-SDK-v6')

    foreach ($c in $candidates) {
        $r = Resolve-Sdk $c
        if ($r) { return $r }
    }
    throw 'No SimConnect SDK found.  Pass -Sdk <root>.'
}

$sdk = Find-Sdk
Write-Host ('SimConnect SDK: {0}' -f $sdk.Root) -ForegroundColor DarkGray
Write-Host ('  include: {0}' -f $sdk.Inc) -ForegroundColor DarkGray
Write-Host ('  library: {0}' -f $sdk.Lib) -ForegroundColor DarkGray

# ---------------------------------------------------------------------------
#  Build
# ---------------------------------------------------------------------------

$vcvars = Find-VcVars
New-Item -ItemType Directory -Force -Path $out | Out-Null

# The SimConnect SDK header is not /W4-clean; 4245 is its own signedness noise.
$cl = 'cl /nologo /EHsc /MD /W4 /wd4245 /I "{0}" "{1}" /Fe:"{2}" /Fo:"{3}\\" /link /LIBPATH:"{4}" SimConnect.lib user32.lib advapi32.lib ole32.lib shell32.lib ws2_32.lib' -f `
    $sdk.Inc, (Join-Path $tool 'rotor_probe.cpp'), $exe, $out, $sdk.Lib

Write-Host ('Building {0}' -f $exe) -ForegroundColor DarkGray
& cmd.exe /c ('call "{0}" >nul 2>&1 && {1}' -f $vcvars, $cl)
if ($LASTEXITCODE -ne 0) { throw 'the probe did not build.' }
if (-not (Test-Path $exe)) { throw "no executable at $exe" }

if ($BuildOnly) { Write-Host 'Built.' -ForegroundColor Green; exit 0 }

# ---------------------------------------------------------------------------
#  Run
# ---------------------------------------------------------------------------

& $exe @ProbeArgs
exit $LASTEXITCODE
