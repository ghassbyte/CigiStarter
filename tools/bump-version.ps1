<#
.SYNOPSIS
    Cuts the next SemVer tag for a merge into main, and keeps the generated
    documentation in step with it.

.DESCRIPTION
    Called by .githooks/post-merge the moment a branch is merged into main.
    Landing on main is this project's definition of "proven work", so that is
    the moment a version is cut -- and the hook means the number is never chosen
    by hand.

    The bump comes from the branch name, the only statement of intent a hook can
    read:

        feature/*   MINOR    a new message mapping, terrain source or feature
        fix/*       PATCH    a fix that changes no mapping
        major/*     MAJOR    changes what the IG puts on the wire or answers
        breaking/*  MAJOR    an alias for major/
        anything else        PATCH, with a note

    Before tagging, two generated pieces of documentation are brought up to date
    and committed, so the *tagged commit* is never stale:

        * README.md's current-build line (tools/sync-docs.ps1);
        * CHANGELOG.md's [Unreleased] section, renamed to the new version (Keep
          a Changelog's own convention), leaving a fresh empty [Unreleased].

    The commit touches only those two paths (`git commit --only`), so unrelated
    work in the tree is not swept up. If there is nothing to change, no commit
    is made and the tag lands on the merge itself.

    The tag is annotated and lists the commits since the previous tag, so
    `git show vX.Y.Z` explains itself. The script never pushes and never deletes
    anything: review the tag, then publish it.

.PARAMETER Branch
    The branch that was merged; its name selects the bump.

.PARAMETER DryRun
    Work out the version and say so, but create nothing.

.PARAMETER NoDocs
    Tag without touching README.md or CHANGELOG.md.

.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File tools\bump-version.ps1 -Branch feature/searchlight -DryRun
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Branch,
    [switch]$DryRun,
    [switch]$NoDocs
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot        # tools\ -> the repository root

# Run git in the repo root without letting its stderr (line-ending notes,
# "already up to date" style chatter) become a terminating error. Deliberately
# NOT named `Git`: PowerShell resolves command names case-insensitively, so a
# function called Git would shadow git.exe and call itself forever.
function Invoke-Git {
    param([string[]]$Argv)
    $eap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { & git -C $root @Argv } finally { $ErrorActionPreference = $eap }
}

# Rename CHANGELOG's [Unreleased] heading to [X.Y.Z] - date and leave a fresh
# empty [Unreleased] above it. A no-op when there is no section or it is empty.
function Finalize-Changelog {
    param([string]$Version, [string]$Date)

    $chlog = Join-Path $root 'CHANGELOG.md'
    if (-not (Test-Path -LiteralPath $chlog)) { return }

    $raw = [System.IO.File]::ReadAllText($chlog)
    $nl  = if ($raw -match "`r`n") { "`r`n" } else { "`n" }
    $lines = $raw -split "\r?\n"

    $i = -1
    for ($k = 0; $k -lt $lines.Count; $k++) {
        if ($lines[$k] -match '^##\s+\[Unreleased\]') { $i = $k; break }
    }
    if ($i -lt 0) { return }

    $j = $lines.Count
    for ($k = $i + 1; $k -lt $lines.Count; $k++) {
        if ($lines[$k] -match '^##\s+\[') { $j = $k; break }
    }

    $body = if ($j -gt $i + 1) { @($lines[($i + 1)..($j - 1)]) } else { @() }
    if (($body -join '').Trim().Length -eq 0) { return }   # nothing to release

    $newLines = @()
    if ($i -gt 0)              { $newLines += $lines[0..($i - 1)] }
    $newLines += '## [Unreleased]'
    $newLines += ''
    $newLines += "## [$Version] - $Date"
    $newLines += $body
    if ($j -lt $lines.Count)   { $newLines += $lines[$j..($lines.Count - 1)] }

    [System.IO.File]::WriteAllText($chlog, ($newLines -join $nl),
                                   (New-Object System.Text.UTF8Encoding($false)))
}

# Any failure is a no-op, never a broken merge: post-merge cannot change the
# merge's outcome, but a wall of red text after a clean merge is alarming.
try {
    if ((((Invoke-Git @('rev-parse', '--abbrev-ref', 'HEAD')) | Out-String).Trim()) -ne 'main') {
        Write-Host '[tag] not on main; no tag.'
        exit 0
    }

    # Idempotent: a commit that already carries a release tag is left alone, so
    # a second merge or a re-run cannot double-tag.
    $onHead = @(Invoke-Git @('tag', '--points-at', 'HEAD') | Where-Object { $_ -match '^v\d+\.\d+\.\d+$' })
    if ($onHead.Count -gt 0) {
        Write-Host "[tag] HEAD is already tagged $($onHead[0]); no tag."
        exit 0
    }

    # The newest release tag anywhere in the repo, by version order (not by date
    # or by which branch it is on).
    $latest = @(Invoke-Git @('tag', '-l', 'v[0-9]*') | Where-Object { $_ -match '^v\d+\.\d+\.\d+$' }) |
              Sort-Object { [version]($_.TrimStart('v')) } |
              Select-Object -Last 1

    if ($latest) {
        $base  = $latest
        $parts = $latest.TrimStart('v').Split('.')
        $major = [int]$parts[0]; $minor = [int]$parts[1]; $patch = [int]$parts[2]
    } else {
        $base = $null; $major = 0; $minor = 0; $patch = 0
    }

    $prefix = ($Branch -split '/')[0].ToLowerInvariant()
    $kind = switch ($prefix) {
        'feature'  { 'minor' }
        'fix'      { 'patch' }
        'major'    { 'major' }
        'breaking' { 'major' }
        default    { 'patch' }
    }

    if     ($kind -eq 'major') { $major++; $minor = 0; $patch = 0 }
    elseif ($kind -eq 'minor') { $minor++; $patch = 0 }
    else                       { $patch++ }

    # Never collide with a tag that already exists under this name.
    $new = "v$major.$minor.$patch"
    while (@(Invoke-Git @('tag', '-l', $new)).Count -gt 0) {
        $patch++; $new = "v$major.$minor.$patch"
    }

    if ($prefix -notin @('feature', 'fix', 'major', 'breaking')) {
        Write-Host "[tag] '$Branch' is not feature/*, fix/* or major/*; bumping a patch."
    }

    if ($DryRun) {
        Write-Host "[tag] would cut $new  ($kind bump on '$Branch'; based on $base)"
        exit 0
    }

    # 1. Bring the generated documentation up to date, then commit just it.
    if (-not $NoDocs) {
        $sync = Join-Path $PSScriptRoot 'sync-docs.ps1'
        if (Test-Path -LiteralPath $sync) { & $sync -Version $new | Out-Host }
        Finalize-Changelog -Version ($new -replace '^v', '') -Date (Get-Date).ToString('yyyy-MM-dd')
    }

    $changedDocs = @(Invoke-Git @('diff', '--name-only', '--', 'README.md', 'CHANGELOG.md'))
    if ($changedDocs.Count -gt 0) {
        $commitArgs = @('commit', '--only') + $changedDocs + @('-m', "docs: record the current IG build $new")
        Invoke-Git $commitArgs | Out-Host
    }

    # 2. Tag the (now up-to-date) commit, with a message listing what it covers.
    $range   = if ($base) { "$base..HEAD" } else { 'HEAD' }
    $subject = @(Invoke-Git @('log', '--oneline', $range))

    $body = @(
        "Merge of $Branch into main, auto-tagged by .githooks/post-merge."
        ""
        if ($base) { "Changes since ${base}:" } else { "The first tagged build." }
        ""
    )
    $body += ($subject | ForEach-Object { "  $_" })
    $message = $body -join "`r`n"

    $msgFile = [System.IO.Path]::GetTempFileName()
    try {
        [System.IO.File]::WriteAllText($msgFile, $message)
        Invoke-Git @('tag', '-a', $new, '-F', $msgFile) | Out-Host
        if (@(Invoke-Git @('tag', '-l', $new)).Count -eq 0) {
            Write-Host "[tag] could not create $new."
            exit 0
        }
    } finally {
        Remove-Item -LiteralPath $msgFile -Force -ErrorAction SilentlyContinue
    }

    Write-Host "[tag] merged '$Branch' -> $new"
    Write-Host "[tag] publish it with:  git push origin main $new"
    exit 0
}
catch {
    Write-Host "[tag] skipped: $($_.Exception.Message)"
    exit 0
}
