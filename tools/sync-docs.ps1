<#
.SYNOPSIS
    Regenerates the git-derived "generated" regions of README.md.

.DESCRIPTION
    Some facts in README.md are derived from git -- above all the current IG
    build. A hand-typed copy is stale the moment it is written, so those facts
    live between markers and are rewritten here:

        <!-- BEGIN GENERATED: current-build -->
        **Current IG build: `v1.0.0`** ...
        <!-- END GENERATED: current-build -->

    Everything outside the markers is never touched. The release step
    (tools/bump-version.ps1, run by .githooks/post-merge) calls this before it
    tags, so the tagged commit carries a correct README; .githooks/pre-push runs
    it with -Check so a stale region cannot be published.

    It reports only what git already knows, and decides nothing. Prose -- a
    feature's own section, AGENTS.md, TODO.md -- cannot be generated and stays a
    human duty; this only keeps the *derivable* facts honest.

.PARAMETER Version
    The version to record, e.g. "v1.1.0". Default: the newest tag.

.PARAMETER Check
    Write nothing. Exit 1 when the region is stale, so a hook can refuse.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File tools\sync-docs.ps1
    powershell -NoProfile -ExecutionPolicy Bypass -File tools\sync-docs.ps1 -Check
#>
[CmdletBinding()]
param(
    [string]$Version,
    [switch]$Check
)

# Continue, not Stop: this script shells out to git, and git writes warnings
# (line-ending notes, etc.) to stderr that must not become terminating errors.
$ErrorActionPreference = 'Continue'

$root   = Split-Path -Parent $PSScriptRoot        # tools\ -> the repository root
$readme = Join-Path $root 'README.md'

$kBegin = '<!-- BEGIN GENERATED: current-build -->'
$kEnd   = '<!-- END GENERATED: current-build -->'

# An em dash for the generated prose. Kept as a code point because this file is
# read by PowerShell as ANSI (no BOM), so a literal non-ASCII byte here would be
# mangled; the README it writes is UTF-8 and gets the real character.
$dash = [char]0x2014

function Get-LatestTag {
    $cands = @(& git -C $root tag -l 'v[0-9]*' 2>$null |
               Where-Object { $_ -match '^v\d+\.\d+\.\d+$' })
    if (-not $cands) { return $null }
    $cands | Sort-Object { [version]($_.TrimStart('v')) } | Select-Object -Last 1
}

if (-not $Version) { $Version = Get-LatestTag }

$raw = [System.IO.File]::ReadAllText($readme)
$nl  = if ($raw -match "`r`n") { "`r`n" } else { "`n" }

# What the region should say. Kept to the newest tag name alone, on purpose:
# it then depends only on which tag exists, not on when it was made or how far
# HEAD has moved, so the release step can sync it *before* the new tag exists
# and still produce exactly what a later -Check will expect.
if ($Version) {
    $body = @(
        "**Current IG build: ``$Version``** $dash the newest tag on ``main``. Every"
        "build names itself $dash see [Versioning](#versioning)."
    )
} else {
    $body = @(
        "**Current IG build: untagged** $dash no release tag exists yet. Tag ``main``"
        "by merging a branch (``.githooks/post-merge``) $dash see [Versioning](#versioning)."
    )
}

$lines = $raw -split "\r?\n"
$bi = [array]::IndexOf($lines, $kBegin)
$ei = [array]::IndexOf($lines, $kEnd)

if ($bi -lt 0 -or $ei -lt 0 -or $ei -lt $bi) {
    # Not this script's region to write: say so, and do not block anything.
    Write-Host "[docs] README.md has no current-build markers; nothing to do."
    exit 0
}

$newLines = @()
if ($bi -gt 0) { $newLines += $lines[0..($bi - 1)] }
$newLines += $kBegin
$newLines += $body
$newLines += $kEnd
if ($ei -lt $lines.Count - 1) { $newLines += $lines[($ei + 1)..($lines.Count - 1)] }

$new = $newLines -join $nl

if ($new -eq $raw) {
    if (-not $Check) { Write-Host "[docs] README.md current-build is up to date ($Version)." }
    exit 0
}

if ($Check) {
    Write-Host "[docs] README.md current-build is stale; run tools\sync-docs.ps1"
    exit 1
}

[System.IO.File]::WriteAllText($readme, $new, (New-Object System.Text.UTF8Encoding($false)))
Write-Host "[docs] README.md current-build -> $(if ($Version) { $Version } else { 'untagged' })"
exit 0
