[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.4',
    [ValidateSet('Check', 'Package')][string]$Action = 'Check',
    [ValidateRange(1, 240)][int]$TimeoutMinutes = 30,
    [ValidatePattern('^/[A-Za-z0-9_/]+$')][string]$Map = '/DialogueKit/Sample/Maps/NewWorld'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$ProjectFile = Join-Path $ProjectRoot 'DialogueMaker.uproject'
$RunUAT = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
# 기존 환경 검사를 재사용하고 사용자 프로세스를 중단하지 않는다.
& (Join-Path $PSScriptRoot 'Invoke-UE.ps1') -Action Check -EngineRoot $EngineRoot
if (-not (Test-Path -LiteralPath $RunUAT -PathType Leaf)) { throw "Missing RunUAT: $RunUAT" }
if ($Action -eq 'Check') { Write-Host "Package entry ready: $RunUAT"; exit 0 }
$Mutex = New-Object System.Threading.Mutex($false, 'Local\UE544-AI-Verification')
$Acquired = $false
try {
    try { $Acquired = $Mutex.WaitOne(0) }
    catch [System.Threading.AbandonedMutexException] { $Acquired = $true }
    if (-not $Acquired) { throw 'Another AI verification is running.' }
    $Busy = @(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool,MSBuild,LiveCodingConsole,AutomationTool -ErrorAction SilentlyContinue)
    if ($Busy.Count -gt 0) { throw 'Editor/build/automation process is running. Close it normally before retrying.' }
    $RunRoot = Join-Path $ProjectRoot ('Saved/AI/Package_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '_' + [guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $RunRoot | Out-Null
    $Log = Join-Path $RunRoot 'Package.log'
    $Archive = Join-Path $RunRoot 'Archive'
    $Staging = Join-Path $RunRoot 'Staged'
    $Result = [ordered]@{ Action = 'Package'; Outcome = 'Failed'; ExitCode = $null; FailureCategory = $null; Map = $Map; Log = $Log; Archive = $Archive; Staging = $Staging; Executables = @(); Containers = @(); Error = $null }
    try {
        # 실행마다 별도 출력 경로를 사용한다. 패키지 생성은 실행/화면 검증을 뜻하지 않는다.
        $Arguments = @('BuildCookRun', "-project=$ProjectFile", '-nop4', '-unattended', '-utf8output', '-installed',
            '-platform=Win64', '-clientconfig=Development', '-build', '-cook', '-stage', '-pak', '-package', '-archive',
            "-map=$Map", "-archivedirectory=$Archive", "-stagingdirectory=$Staging",
            '-AdditionalCookerOptions=-ini:Engine:[/Script/Sentry.SentrySettings]:InitAutomatically=False')
        # cmd.exe에 전달하는 값은 고정 인자와 검증된 경로로 제한한다.
        foreach ($Value in (@($RunUAT, $Log) + $Arguments)) {
            if ($Value -match '["&|<>^%!]') { throw 'Unsupported shell character in package path/argument.' }
        }
        $Command = '"' + $RunUAT + '" ' + (($Arguments | ForEach-Object { '"' + $_ + '"' }) -join ' ') + ' > "' + $Log + '" 2>&1'
        $StartInfo = New-Object System.Diagnostics.ProcessStartInfo
        $StartInfo.FileName = $env:ComSpec
        $StartInfo.Arguments = '/d /s /c "' + $Command + '"'
        $StartInfo.WorkingDirectory = $ProjectRoot
        $StartInfo.UseShellExecute = $false
        $StartInfo.CreateNoWindow = $true
        $Process = [System.Diagnostics.Process]::Start($StartInfo)
        if (-not $Process.WaitForExit($TimeoutMinutes * 60000)) {
            $Result.FailureCategory = 'Timeout'
            # 이 스크립트가 시작한 프로세스 트리만 종료한다. 기존 사용자 에디터는 대상이 아니다.
            & "$env:SystemRoot/System32/taskkill.exe" /PID $Process.Id /T /F
            if (-not $Process.WaitForExit(10000)) { throw 'Timed-out package process did not exit; inspect child processes before retrying.' }
            throw "Package timed out after $TimeoutMinutes minutes. Log: $Log"
        }
        $Process.Refresh()
        $Result.ExitCode = $Process.ExitCode
        Get-Content -LiteralPath $Log -Tail 60
        if ($Process.ExitCode -ne 0) {
            $Result.FailureCategory = 'AutomationToolExit'
            throw "BuildCookRun failed with $($Process.ExitCode). Inspect $Log"
        }
        $Executables = @(Get-ChildItem -LiteralPath $Archive -Recurse -File -Filter 'DialogueMaker*.exe' | Where-Object { $_.Length -gt 0 })
        $Paks = @(Get-ChildItem -LiteralPath $Archive -Recurse -File -Filter '*.pak' | Where-Object { $_.Length -gt 0 })
        if ($Executables.Count -eq 0 -or $Paks.Count -eq 0) {
            $Result.FailureCategory = 'MissingArtifacts'
            throw "Fresh archive requires nonempty game executable and pak: $Archive"
        }
        $Result.Executables = @($Executables | ForEach-Object { $_.FullName })
        $Result.Containers = @($Paks | ForEach-Object { $_.FullName })
        $Result.Outcome = 'PackagedNotLaunched'
    } catch {
        if (-not $Result.FailureCategory) { $Result.FailureCategory = 'ExecutionError' }
        $Result.Error = $_.Exception.Message
        throw
    } finally {
        $Result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8
    }
    Write-Host "Package created, not launched. Evidence: $RunRoot"
} finally {
    if ($Acquired) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}
