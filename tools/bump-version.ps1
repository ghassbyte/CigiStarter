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
    powershell -NoProfile -ExecutionPolicy Bypass -File tools\bump-version.ps1 -Branch feature/terrain-tile -DryRun
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

# 0-based indices, in the working-tree file, of the lines git sees as added
# there against $Against. `-U0` keeps the hunks tight, and counting the
# new-side line numbers gives the positions directly.
#
# Running git inside the process is what makes the caller's state irrelevant: a
# stash, an earlier partial commit or a half-finished edit elsewhere cannot hide
# the lines in question, because the comparison is against a committed ref and
# not against whatever the caller happens to have staged.
function Get-AddedLineIndices {
    param([string]$Path, [string]$Against = 'HEAD')

    $added = @()
    $raw = $null
    try { $raw = (Invoke-Git @('diff', '-U0', '--no-color', '--no-ext-diff', $Against, '--', $Path)) } catch { return $added }
    if ($null -eq $raw) { return $added }

    $nl = 0            # new-side line number the hunk header refers to
    foreach ($line in @($raw)) {
        if ($line -match '^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@') { $nl = [int]$Matches[1] - 1; continue }
        if ($line -like '+++*' -or $line -like '---*' -or $line -match '^diff --git' -or
            $line -match '^index '   -or $line -match '^@@') { continue }
        if ($line.StartsWith('+')) { $added += $nl; $nl++ ; continue }
        if ($line.StartsWith(' ')) { $nl++ }
        # '-' lines are old-side only: they do not advance the new-side count.
    }
    return $added
}

# Rename CHANGELOG's [Unreleased] heading to [X.Y.Z] - date and leave a fresh
# empty [Unreleased] above it, moving the section's entries with it.
#
# -MoveStrays: also move entry lines this merge added *outside* [Unreleased] --
# underneath the previous release's heading -- into the new section, keeping
# whatever subsection they sit under. That is not cosmetic. When two branches
# are in flight both add their entry under [Unreleased], and only the first to
# merge gets its heading moved; git resolves the second without a conflict but
# drops its entry under the release cut in between, leaving [Unreleased] empty.
# The hook then tags a commit whose changelog does not name its own release,
# which is the staleness this machinery exists to prevent. Hit for real on
# 2026-10-21: v1.1.0 and v1.2.0 landed off two branches and the second release's
# entry ended up under [1.1.0]. See ## Release workflow in AGENTS.md.
#
# "Added by this release" is read from `git diff -U0 <previous tag> --
# CHANGELOG.md`: only lines that appeared since the last release are eligible,
# so an entry that has always been in the previous release cannot be dragged
# forward. Diffing against the tag rather than HEAD is deliberate -- right
# after a merge the worktree equals HEAD, so a HEAD diff would be empty, and
# the tag is also the honest question ("what is new since the last release?").
function Finalize-Changelog {
    param([string]$Version, [string]$Date, [switch]$MoveStrays, [string]$Against)

    $chlog = Join-Path $root 'CHANGELOG.md'
    if (-not (Test-Path -LiteralPath $chlog)) { return }

    $raw = [System.IO.File]::ReadAllText($chlog)
    $nl  = if ($raw -match "`r`n") { "`r`n" } else { "`n" }
    $lines = $raw -split "\r?\n"

    # 0-based indices of the lines that appeared since the previous release.
    $added = @()
    if ($MoveStrays -and $Against) { $added = @(Get-AddedLineIndices -Path 'CHANGELOG.md' -Against $Against) }

    $i = -1
    for ($k = 0; $k -lt $lines.Count; $k++) {
        if ($lines[$k] -match '^##\s+\[Unreleased\]') { $i = $k; break }
    }
    if ($i -lt 0) { return }

    $j = $lines.Count
    for ($k = $i + 1; $k -lt $lines.Count; $k++) {
        if ($lines[$k] -match '^##\s+\[') { $j = $k; break }
    }

    # The previous release's section, straight after [Unreleased].
    $sectStart = -1
    $sectEnd   = $lines.Count
    if ($MoveStrays -and $j -lt $lines.Count -and $lines[$j] -match '^##\s+\[') {
        $sectStart = $j
        for ($k = $sectStart + 1; $k -lt $lines.Count; $k++) {
            if ($lines[$k] -match '^##\s+\[') { $sectEnd = $k; break }
        }
    }

    $body = if ($j -gt $i + 1) { @($lines[($i + 1)..($j - 1)]) } else { @() }
    $bodyIsEmpty = (($body -join '').Trim().Length -eq 0)

    # Entry lines this merge added inside the previous release, with the
    # subsection they sit under.
    $strays = @()
    $curSub = $null
    if ($sectStart -ge 0) {
        for ($k = $sectStart; $k -lt $sectEnd; $k++) {
            if ($lines[$k] -match '^#{3,3}\s') { $curSub = $lines[$k] }
            elseif ($added -contains $k) {
                $strays += [pscustomobject]@{ Subheading = $curSub; Text = $lines[$k] }
            }
        }
    }

    # Nothing under [Unreleased] and nothing stray: the ordinary no-op.
    if ($bodyIsEmpty -and $strays.Count -eq 0) { return }

    $newLines = @()
    if ($i -gt 0)              { $newLines += $lines[0..($i - 1)] }
    $newLines += '## [Unreleased]'
    $newLines += ''
    $newLines += "## [$Version] - $Date"

    if (-not $bodyIsEmpty) {
        $newLines += $body
    } else {
        # Move the strays up, in their own order and under their own headings.
        foreach ($stray in $strays) {
            $sub = $stray.Subheading
            if (-not $sub) { continue }
            $at = -1
            for ($k = 0; $k -lt $newLines.Count; $k++) { if ($newLines[$k] -eq $sub) { $at = $k; break } }
            if ($at -lt 0) {
                # A subsection this release did not have yet: open it, then add
                # the entry below it.
                $newLines += ''
                $newLines += $sub
                $newLines += $stray.Text
                continue
            }
            # Already open: add below its last entry, which is the line before
            # the blank that separates it from anything after.
            $ins = $at + 1
            while ($ins -lt $newLines.Count -and $newLines[$ins].Trim().Length -gt 0) { $ins++ }
            $before = @($newLines[0..($ins - 1)])
            $after  = if ($ins -lt $newLines.Count) { @($newLines[$ins..($newLines.Count - 1)]) } else { @() }
            $newLines = $before + @($stray.Text) + $after
        }
        if ($strays.Count -gt 0) { $newLines += '' }
    }

    # Everything from the previous release's heading on, with the strays taken
    # out -- this is a move, not a copy, so the lines cannot stay where they
    # were or the release notes would claim the entry twice.
    $tailEnd = if ($sectStart -ge 0) { $sectEnd } else { $lines.Count }
    $tail = @()
    for ($k = $j; $k -lt $tailEnd; $k++) {
        if ($strays.Count -gt 0 -and $added -contains $k) { continue }
        $tail += $lines[$k]
    }
    # Taking an entry out can leave two blank lines where one entry sat.
    $collapsed = @()
    foreach ($line in $tail) {
        if ($line.Trim().Length -eq 0 -and $collapsed.Count -gt 0 -and
            $collapsed[-1].Trim().Length -eq 0) { continue }
        $collapsed += $line
    }
    $newLines += $collapsed
    if ($tailEnd -lt $lines.Count) { $newLines += $lines[$tailEnd..($lines.Count - 1)] }

    [System.IO.File]::WriteAllText($chlog, ($newLines -join $nl),
                                   (New-Object System.Text.UTF8Encoding($false)))
}

# post-merge fires while git still considers the merge in progress: MERGE_HEAD is
# present, and git then refuses a partial commit (a plain `git commit -m` with no
# paths is fine, but `--only <paths>` is not) with
#     fatal: cannot do a partial commit during a merge.
# The merge is already committed by the time this hook runs, so clearing the
# marker only tells git what is already true -- and without it the generated
# documentation could never be committed from the hook, which is the whole point
# of the "the tagged commit is never stale" rule.
function Clear-MergeMarker {
    $mergeHead = ((Invoke-Git @('rev-parse', '--git-path', 'MERGE_HEAD')) | Out-String).Trim()
    if ($mergeHead -and (Test-Path -LiteralPath $mergeHead)) {
        Remove-Item -LiteralPath $mergeHead -Force -ErrorAction SilentlyContinue
    }
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
        $basedOn = if ($base) { "based on $base" } else { 'the first release; no earlier tag' }
        Write-Host "[tag] would cut $new  ($kind bump on '$Branch'; $basedOn)"
        exit 0
    }

    # 1. Bring the generated documentation up to date, then commit just it.
    if (-not $NoDocs) {
        $sync = Join-Path $PSScriptRoot 'sync-docs.ps1'
        if (Test-Path -LiteralPath $sync) { & $sync -Version $new | Out-Host }
        Finalize-Changelog -Version ($new -replace '^v', '') -Date (Get-Date).ToString('yyyy-MM-dd') -MoveStrays -Against $base
    }

    $changedDocs = @(Invoke-Git @('diff', '--name-only', '--', 'README.md', 'CHANGELOG.md'))
    if ($changedDocs.Count -gt 0) {
        Clear-MergeMarker   # else git refuses the partial commit -- see above
        $commitArgs = @('commit', '--only') + $changedDocs + @('-m', "docs: record the current IG build $new")
        Invoke-Git $commitArgs | Out-Host

        # Fail closed. A tag on a commit whose README still names the previous
        # version is exactly the staleness this machinery exists to prevent, and
        # pre-push cannot catch it (that guard reads the working tree, not the
        # tagged commit). Better an untagged merge, with a way to finish it, than
        # a published lie.
        if (@(Invoke-Git @('diff', '--name-only', '--', 'README.md', 'CHANGELOG.md')).Count -gt 0) {
            Write-Host "[tag] the generated docs could not be committed; $new NOT cut."
            Write-Host "[tag] finish by hand, then re-run: powershell -File tools\bump-version.ps1 -Branch $Branch"
            exit 0
        }
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
