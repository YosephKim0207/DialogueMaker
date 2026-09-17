[CmdletBinding()]
param(
    [ValidateSet('Status', 'NewBranch')][string]$Action = 'Status',
    [string]$Name
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Git = 'C:\Program Files\Git\cmd\git.exe'
if (-not (Test-Path -LiteralPath $Git)) { throw "Windows Git not found: $Git" }
if ($Action -eq 'Status') {
    & $Git -C $ProjectRoot status --short --branch
    if ($LASTEXITCODE -ne 0) { throw 'git status failed.' }
    & $Git -C $ProjectRoot log -1 --format='%h %s'
    if ($LASTEXITCODE -ne 0) { throw 'git log failed.' }
    exit 0
}
if ($Name -notmatch '^(feature|fix|chore)/[a-z0-9][a-z0-9._/-]*$') {
    throw 'Use feature/name, fix/name or chore/name (lowercase ASCII).'
}
& $Git check-ref-format --branch $Name
if ($LASTEXITCODE -ne 0) { throw 'Invalid branch name.' }
$State = @(& $Git -C $ProjectRoot status --porcelain)
if ($LASTEXITCODE -ne 0) { throw 'Unable to inspect worktree.' }
if ($State.Count -gt 0) { throw 'Worktree is not clean. Preserve/review changes in Fork before starting a branch; do not auto-stash or auto-commit.' }
& $Git -C $ProjectRoot symbolic-ref --quiet HEAD
if ($LASTEXITCODE -ne 0) { throw 'Detached HEAD: select the intended base branch in Fork first.' }
# A local branch from the current HEAD; no fetch, push, reset or implicit staging.
& $Git -C $ProjectRoot switch -c $Name
if ($LASTEXITCODE -ne 0) { throw 'Branch creation failed.' }
