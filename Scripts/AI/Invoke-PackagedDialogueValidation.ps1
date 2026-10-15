[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackageResult,
    [Parameter(Mandatory=$true)][ValidatePattern('^[a-fA-F0-9]{32}$')][string]$DialogueValidationId
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$PackagePath = (Resolve-Path -LiteralPath $PackageResult).Path
if (-not $PackagePath.StartsWith((Join-Path $ProjectRoot 'Saved/AI/Package_'), [StringComparison]::OrdinalIgnoreCase)) { throw 'Use this project isolated package result.' }
$Package = Get-Content -LiteralPath $PackagePath -Raw | ConvertFrom-Json
if ($Package.Outcome -ne 'PackagedNotLaunched' -or $Package.Configuration -ne 'Development') { throw 'A successful Development package is required.' }
$Executables = @($Package.Executables | Where-Object { $_ -match '[\\/]DialogueMaker[\\/]Binaries[\\/]Win64[\\/]DialogueMaker.exe$' })
if ($Executables.Count -ne 1) { throw 'Expected exactly one inner game executable, not the bootstrap launcher.' }
$Executable = (Resolve-Path -LiteralPath $Executables[0]).Path
$Archive = (Resolve-Path -LiteralPath $Package.Archive).Path
if (-not $Executable.StartsWith($Archive + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Executable outside archive.' }
$Mutex = New-Object System.Threading.Mutex($false, 'Local\UE544-AI-Verification')
$Acquired = $false
# 기존 프로젝트 슬롯의 변경을 감시하되 읽기만 한다.
function Get-SaveHashes([string]$Directory) {
    $Hashes = @{}
    if (Test-Path -LiteralPath $Directory) {
        Get-ChildItem -LiteralPath $Directory -Recurse -File | ForEach-Object { $Hashes[$_.FullName] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    }
    return $Hashes
}
function Assert-SameHashes($Before, $After) {
    if ($Before.Count -ne $After.Count) { throw 'Unexpected save file count change.' }
    foreach ($Key in $Before.Keys) { if (-not $After.ContainsKey($Key) -or $Before[$Key] -ne $After[$Key]) { throw "Unexpected save mutation: $Key" } }
}
try {
    try { $Acquired = $Mutex.WaitOne(0) } catch [System.Threading.AbandonedMutexException] { $Acquired = $true }
    if (-not $Acquired) { throw 'Another AI verification is running.' }
    $RunRoot = Join-Path $ProjectRoot ('Saved/AI/PackageRun_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '_' + [guid]::NewGuid().ToString('N').Substring(0,8))
    $UserDir = Join-Path $RunRoot 'RuntimeUserDir'
    New-Item -ItemType Directory -Path $UserDir | Out-Null
    $ExistingSlots = Join-Path $ProjectRoot 'Saved/SaveGames'
    $Before = Get-SaveHashes $ExistingSlots
    $Result = [ordered]@{ Outcome='Failed'; Executable=$Executable; PackageResult=$PackagePath; ValidationId=$DialogueValidationId; UserDir=$UserDir; Phases=@(); ExistingSlotsUnchanged=$false; Error=$null }
    try {
        foreach ($Phase in @('Write','Crash','Read')) {
            $Marker = Join-Path $UserDir ('Saved/DialogueValidation/' + $Phase + '.txt')
            $Log = Join-Path $RunRoot ($Phase + '.log')
            $Args = @('-NullRHI','-nosound','-unattended','-nosplash', '-log', "-userdir=$UserDir", "-abslog=$Log",
                "-DialoguePackageValidation=$DialogueValidationId", "-DialogueValidationPhase=$Phase", '-ini:Engine:[/Script/Sentry.SentrySettings]:InitAutomatically=False')
            foreach ($Value in $Args) { if ($Value -match '["\r\n]') { throw 'Unsupported argument character.' } }
            $Info = New-Object System.Diagnostics.ProcessStartInfo
            $Info.FileName = $Executable
            $Info.Arguments = ($Args | ForEach-Object { '"' + $_ + '"' }) -join ' '
            $Info.WorkingDirectory = Split-Path -Parent $Executable
            $Info.UseShellExecute = $false
            $Info.CreateNoWindow = $true
            $Process = [System.Diagnostics.Process]::Start($Info)
            $Deadline = (Get-Date).AddSeconds(100)
            $Forced = $false
            try {
                while (-not $Process.HasExited -and (Get-Date) -lt $Deadline) {
                    if ($Phase -eq 'Crash' -and (Test-Path -LiteralPath $Marker)) {
                        $Text = Get-Content -LiteralPath $Marker -Raw
                        if ($Text -match '(?m)^Status=Pending$' -and $Text -match ("(?m)^Pid=" + $Process.Id + '$')) {
                            # 이 래퍼가 시작했고 marker로 확인한 게임 프로세스만 종료한다.
                            & "$env:SystemRoot/System32/taskkill.exe" /PID $Process.Id /T /F | Out-Null
                            $Forced = $true
                            break
                        }
                    }
                    Start-Sleep -Milliseconds 200
                }
                if (-not $Process.WaitForExit(10000)) { throw 'Packaged validation timeout.' }
                $Process.Refresh()
                if (-not (Test-Path -LiteralPath $Marker)) { throw "Missing runtime result: $Phase" }
                $Text = Get-Content -LiteralPath $Marker -Raw
                if ($Phase -eq 'Crash') {
                    if (-not $Forced) { throw 'Crash phase did not reach the pending-write boundary.' }
                    Assert-SameHashes $DurableHashes (Get-SaveHashes (Join-Path $UserDir 'Saved/SaveGames'))
                } else {
                    if ($Process.ExitCode -ne 0 -or $Text -notmatch '(?m)^Status=Success$') { throw "Runtime validation failed: $Phase $Text" }
                    if ($Phase -eq 'Write') { $DurableHashes = Get-SaveHashes (Join-Path $UserDir 'Saved/SaveGames') }
                }
                $Errors = @(Select-String -LiteralPath $Log -Pattern ': Error:' -ErrorAction SilentlyContinue)
                if ($Errors.Count -gt 0) { throw "Runtime log contains errors: $Log" }
                $Result.Phases += [ordered]@{ Phase=$Phase; ExitCode=$Process.ExitCode; Forced=$Forced; Marker=$Marker; Log=$Log; RuntimeResult=$Text }
            } finally {
                if (-not $Process.HasExited) { & "$env:SystemRoot/System32/taskkill.exe" /PID $Process.Id /T /F | Out-Null; $Process.WaitForExit(10000) | Out-Null }
                $Process.Dispose()
            }
        }
        Assert-SameHashes $Before (Get-SaveHashes $ExistingSlots)
        $Result.ExistingSlotsUnchanged = $true
        $Result.Outcome = 'PackagedWriteCrashRestorePassed'
    } catch { $Result.Error = $_.Exception.Message; throw }
    finally {
        try { Assert-SameHashes $Before (Get-SaveHashes $ExistingSlots); $Result.ExistingSlotsUnchanged = $true }
        finally { $Result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8 }
    }
    Write-Host "Packaged runtime verification passed: $RunRoot"
} finally {
    if ($Acquired) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}
