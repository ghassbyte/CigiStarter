<#
.SYNOPSIS
    Generates the build-identity header (BuildInfo.h) from git describe.

.DESCRIPTION
    Run as a pre-build step by CigiStarter.vcxproj. Writes a small header the
    app compiles in, so the running exe can name its own build:

        CigiStarter v1.0.0-3-g31d8f4b (Release, built 2026-10-11 01:05)

    The version string is *derived*, never hand-typed. On a tagged commit
    "git describe --tags" yields "v1.0.0"; three commits later it yields
    "v1.0.0-3-g<short-hash>"; with no tags at all it yields the bare short
    hash; and a working tree with uncommitted edits appends "-dirty". So a
    feature-branch bench build self-identifies without anyone tagging the
    branch -- the only tag the project needs is on the deployed baseline.

    The header is generated into the build's intermediate directory, which is
    git-ignored: it is an artefact, not source. Never edit it by hand.

.PARAMETER OutFile
    Full path of the header to write (typically the project's IntDir).

.PARAMETER RepoDir
    The repository root that git should be queried in.

.PARAMETER Configuration
    Build configuration to record ("Debug" or "Release").
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$OutFile,
    [Parameter(Mandatory = $true)][string]$RepoDir,
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'

# A trailing slash/backslash confuses "git -C"; trim it.
$RepoDir = $RepoDir.TrimEnd('\', '/')

function Invoke-Git {
    param([string[]]$GitArgs)
    try {
        $out = & git -C $RepoDir @GitArgs 2>$null
        if ($LASTEXITCODE -ne 0) { return '' }
        return (($out | Out-String).Trim())
    } catch {
        return ''   # git not installed / not a repo: fall back to defaults
    }
}

$describe = Invoke-Git @('describe', '--tags', '--always', '--dirty')
if ([string]::IsNullOrWhiteSpace($describe)) { $describe = 'unknown' }

$hash = Invoke-Git @('rev-parse', '--short', 'HEAD')
if ([string]::IsNullOrWhiteSpace($hash)) { $hash = 'unknown' }

# The semver part of the describe string: "v1.0.0" or "v1.0.0-3-gabc1234"
# both start with the tag, so both yield "1.0.0". A bare hash (no tags yet)
# does not match and stays "0.0.0".
$semver = '0.0.0'
if ($describe -match '^v?(\d+\.\d+\.\d+)') { $semver = $Matches[1] }

$dirty = 0
if ($describe -match 'dirty$') { $dirty = 1 }

$buildDate = (Get-Date).ToString('yyyy-MM-dd HH:mm')
$config    = $Configuration

# Keep this comment free of backticks and stray dollar signs: it is inside a
# double-quoted here-string, so those would be interpreted.
$content = @"
// =====================================================================
//  BuildInfo.h - GENERATED FILE, DO NOT EDIT, DO NOT COMMIT.
// ---------------------------------------------------------------------
//  Written by tools\gen_buildinfo.ps1 as a pre-build step of
//  CigiStarter.vcxproj, from git describe. It is the build's own identity,
//  compiled in so the exe can name itself (see src\Version.h).
//  Delete it freely; the next build regenerates it.
// =====================================================================
#pragma once

#define CIGI_BUILD_SEMVER    "$semver"
#define CIGI_BUILD_DESCRIBE  "$describe"
#define CIGI_BUILD_GIT_HASH  "$hash"
#define CIGI_BUILD_GIT_DIRTY $dirty
#define CIGI_BUILD_DATE      "$buildDate"
#define CIGI_BUILD_CONFIG    "$config"
"@

$content = $content + "`r`n"

$dir = Split-Path -Parent $OutFile
if ($dir -and -not (Test-Path -LiteralPath $dir)) {
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
}

$ascii = New-Object System.Text.ASCIIEncoding

# Rewrite only when the bytes would differ. A build from an unchanged tree
# then does not touch the file's timestamp, so the objects that include it
# are not needlessly recompiled.
if ([System.IO.File]::Exists($OutFile) -and
    [System.IO.File]::ReadAllText($OutFile) -eq $content) {
    Write-Verbose "BuildInfo.h unchanged ($describe)"
    exit 0
}

[System.IO.File]::WriteAllText($OutFile, $content, $ascii)
Write-Host "BuildInfo: $describe [$config] built $buildDate -> $OutFile"
