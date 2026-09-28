<#
.SYNOPSIS
    Build and run the PdkSimLink contract test.

.DESCRIPTION
    PdkSimLink.dll can only be exercised for real inside Prepar3D, but the
    contract either side of it can be tested without the sim at all.  This
    builds tools\PdkSimLink\test\PdkSimLinkTest.vcxproj -- which compiles
    CigiStarter's half (SimLink.cpp) and the add-on's PDK-free half
    (SimLinkMap.h) directly -- and runs it, then reports the one word a run is
    read by and exits 0 or 1.

    The checks are the ones that would be flown by the HOST if they were wrong:

      * the HOST's own aircraft (entity 0) is rendered -- it is the one entity
        that must never be skipped for being id 0;
      * nothing else is: a disabled row, an all-zero pose, a standby or
        destroyed entity all mean "take the object back out";
      * the pose that arrives is the pose that was sent, in the PDK's units and
        order (degrees to radians, metres to feet, yaw in the heading slot);
      * a row caught mid-rewrite is never believed (the seqlock), read flat out
        with the IG rewriting underneath;
      * a publisher with no add-on loaded does nothing, and a region left behind
        by a sim that has stopped ticking is not believed.

    No Prepar3D, no PDK and no elevation are needed.

.PARAMETER Configuration
    Build configuration to test.  Release by default.

.PARAMETER Platform
    Build platform.  x64 by default.

.PARAMETER NoBuild
    Run the executable that is already built, without rebuilding it.  Useful in
    a tight edit-and-run loop.

.PARAMETER MsBuild
    Path to MSBuild.exe.  Found automatically when not given.

.EXAMPLE
    .\tools\test-simlink.ps1
    Build it and run it; exit 0 when every check passes.
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')] [string] $Configuration = 'Release',
    [string] $Platform = 'x64',
    [switch] $NoBuild,
    [string] $MsBuild = ''
)

$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot          # this script lives in tools\
$proj = Join-Path $repo 'tools\PdkSimLink\test\PdkSimLinkTest.vcxproj'
$exe  = Join-Path $repo ("tools\PdkSimLink\test\{0}\{1}\PdkSimLinkTest.exe" `
                         -f $Platform, $Configuration)

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

if (-not $NoBuild) {
    $msbuild = Find-MsBuild
    Write-Host ("building {0} ({1}|{2})" -f (Split-Path -Leaf $proj), $Configuration, $Platform)
    & $msbuild $proj "/p:Configuration=$Configuration" "/p:Platform=$Platform" /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) {
        Write-Verdict 'FAIL   --   the test project did not build' 'Red'
        exit 1
    }
}

if (-not (Test-Path $exe)) {
    Write-Host ("no test executable at {0}" -f $exe) -ForegroundColor Red
    Write-Host 'build it without -NoBuild, or check -Configuration / -Platform.'
    Write-Verdict 'FAIL   --   nothing to run' 'Red'
    exit 1
}

# Run it and let its own per-check output through: the harness prints what it
# checked, so a failure is readable without a debugger.
& $exe
$code = $LASTEXITCODE

if ($code -eq 0) {
    Write-Verdict 'PASS   --   the PdkSimLink contract holds' 'Green'
    exit 0
}
Write-Verdict 'FAIL   --   the PdkSimLink contract is broken' 'Red'
exit 1
