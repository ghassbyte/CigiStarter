# `.githooks/`

Committed git hooks for this repository. They are **not active until you opt
in**, because git only reads `.git/hooks` by default:

```
git config core.hooksPath .githooks      # once per clone
```

Undo with `git config --unset core.hooksPath`.

## `post-merge` — tag a proven build on `main`

The project's workflow (see `AGENTS.md` → *Release workflow*) is: branch from
`main`, prove the work there, merge it into `main`. Landing on `main` *is* the
definition of proven, so the hook cuts the version tag at that moment. The bump
is read from the branch name, so the number is never chosen by hand:

| Branch | Bump | For |
| --- | --- | --- |
| `feature/*` | MINOR (`1.2.0` → `1.3.0`) | a new message mapping, terrain source or feature |
| `fix/*` | PATCH (`1.2.0` → `1.2.1`) | a fix that changes no mapping |
| `major/*`, `breaking/*` | MAJOR (`1.2.0` → `2.0.0`) | changes what the IG puts on the wire or answers |
| anything else | PATCH, with a note | — |

```
$ git checkout main && git merge --ff-only feature/pdk-simlink
[tag] merged 'feature/pdk-simlink' -> v1.1.0
[tag] publish it with:  git push origin main v1.1.0
```

It is a **local** hook and it **never pushes**: review the tag, then publish it.
A bad tag is caught before it leaves the machine.

It deliberately does nothing when the merge is not new work landing on `main`:
a non-`main` branch, a `git pull` (reflog `pull …`), a merge of a
remote-tracking ref, or a `--squash` merge (which commits nothing to stand a tag
on). It is also idempotent — a `HEAD` that already carries a `vX.Y.Z` tag is
left alone, so a re-run or a second merge cannot double-tag.

The version rule itself lives in `tools/bump-version.ps1`; the hook is only a
dispatcher. The same script also refreshes the *generated* documentation before
it tags — the README's current-build line (`tools/sync-docs.ps1`) and the
changelog's `[Unreleased]` heading — and commits just those two files, so the
tagged commit is never stale. See `AGENTS.md` → *Release workflow*.

To see what a merge *would* cut, without creating anything:

```
powershell -NoProfile -ExecutionPolicy Bypass -File tools\bump-version.ps1 -Branch feature/x -DryRun
```

## `pre-push` — refuse a stale generated region

The README's current-build line is generated. If a tag is ever cut or changed by
hand, that line can contradict the tags; this stops the push rather than
publishing it:

```
pre-push: generated docs are stale -- run: powershell -File tools\sync-docs.ps1
```

Fix it with the command it prints, or bypass with `git push --no-verify` when the
mismatch is understood.

### Why a shell script rather than a `.ps1` hook

Git executes hooks through `sh` on **every** platform, Windows included, so the
hook *file* has to be a shell script — a `.ps1` named `post-merge` would simply
not run. The `sh` half is therefore a bare dispatcher, and everything that
decides anything is in `tools\bump-version.ps1`, which is real PowerShell
(branch parsing, version ordering, the tag message). The hook calls
`powershell`, which is present on Windows; on another platform it needs `pwsh`
reachable under that name, so treat "runs anywhere" as for the *hook*, not for
this script — the build and the IG PC are Windows-only anyway.

One trap the split creates: the shebang must be LF-terminated or git's bundled
`sh` cannot find its interpreter, so `.gitattributes` pins `.githooks/*` to
`eol=lf`. Do not add a hook without that rule.
