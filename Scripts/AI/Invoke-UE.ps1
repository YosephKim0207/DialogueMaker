[CmdletBinding()]
param(
    [ValidateSet('Check', 'Build', 'Test')][string]$Action = 'Check',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.4',
    [string]$TestFilter,
    [ValidateRange(1, 240)][int]$TimeoutMinutes = 30
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$ProjectName = Split-Path $ProjectRoot -Leaf
$ProjectFile = Join-Path $ProjectRoot "$ProjectName.uproject"
$Target = "$($ProjectName)Editor"
$BuildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$VersionFile = Join-Path $EngineRoot 'Engine\Build\Build.version'
foreach ($Path in @($ProjectFile, $BuildScript, $EditorCmd, $VersionFile,
    (Join-Path $ProjectRoot "Source\$Target.Target.cs"))) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing file: $Path" }
}
$Version = Get-Content -LiteralPath $VersionFile -Raw | ConvertFrom-Json
if ("$($Version.MajorVersion).$($Version.MinorVersion).$($Version.PatchVersion)" -ne '5.4.4') {
    throw 'Expected Unreal Engine 5.4.4. Engine changes require user approval.'
}
$Descriptor = Get-Content -LiteralPath $ProjectFile -Raw | ConvertFrom-Json
if ($Descriptor.EngineAssociation -ne '5.4') { throw 'Unexpected EngineAssociation; inspect before building.' }
[pscustomobject]@{
    Project = $ProjectFile; Engine = $EngineRoot; Version = '5.4.4'
    Target = $Target; Configuration = 'Development'; Platform = 'Win64'
} | Format-List
if ($Action -eq 'Check') { exit 0 }
if ($Action -eq 'Test' -and $TestFilter -notmatch '^[A-Za-z0-9_.]+$') {
    throw 'Provide one registered test name or prefix using letters, digits, underscore and dot.'
}
# Serialize these wrappers across both projects without stopping user processes.
$Mutex = New-Object System.Threading.Mutex($false, 'Local\UE544-AI-Verification')
$Acquired = $false
try {
    try { $Acquired = $Mutex.WaitOne(0) }
    catch [System.Threading.AbandonedMutexException] { $Acquired = $true }
    if (-not $Acquired) { throw 'Another AI verification is running.' }
    $Busy = @(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool,MSBuild,LiveCodingConsole -ErrorAction SilentlyContinue)
    if ($Busy.Count -gt 0) { throw "Editor/build process is running: $($Busy.ProcessName -join ', '). Close it normally before retrying." }
    $RunRoot = Join-Path $ProjectRoot ('Saved\AI\' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '_' + [guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $RunRoot | Out-Null
    $Log = Join-Path $RunRoot "$Action.log"
    if ($Action -eq 'Build') {
        # Build.bat selects the engine-bundled .NET SDK; do not use project dotnet build.
        & $BuildScript $Target Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReloadFromIDE 2>&1 |
            Tee-Object -FilePath $Log
        $Code = $LASTEXITCODE
        @{ Action = $Action; Target = $Target; ExitCode = $Code; Log = $Log } |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8
        if ($Code -ne 0) { throw "Build failed ($Code). Log: $Log" }
    } else {
        $TestResult = [ordered]@{ Action = $Action; Outcome = 'Failed'; FailureCategory = $null; ReportFailureCategory = $null; KnownBaselineDiagnostics = 0; UnexpectedLogErrors = @(); ExitCode = $null; TestFilter = $TestFilter; TestCount = 0; PassedCount = 0; FailedCount = 0; Report = $null; Log = $Log; LogErrorLines = 0; StartupClean = $false; Tests = @(); Error = $null }
        try {
        $Report = Join-Path $RunRoot 'Report'
        $Arguments = @(
            ('"' + $ProjectFile + '"'),
            ('"-ExecCmds=Automation RunTests ' + $TestFilter + '; Quit"'),
            ('"-ReportExportPath=' + $Report + '"'),
            ('"-abslog=' + $Log + '"'),
            '-unattended', '-nop4', '-nosplash', '-NullRHI', '-nosound',
            '-ini:Engine:[/Script/Sentry.SentrySettings]:InitAutomatically=False'
        )
        $StartInfo = New-Object System.Diagnostics.ProcessStartInfo
        $StartInfo.FileName = $EditorCmd
        $StartInfo.Arguments = $Arguments -join ' '
        $StartInfo.WorkingDirectory = $ProjectRoot
        $StartInfo.UseShellExecute = $false
        $StartInfo.CreateNoWindow = $true
        $Process = [System.Diagnostics.Process]::Start($StartInfo)
        if (-not $Process.WaitForExit($TimeoutMinutes * 60000)) {
            $TestResult.FailureCategory = 'Timeout'
            $Process.Kill()
            $Process.WaitForExit()
            $TestResult.FailureCategory = 'Timeout'
            throw "Automation timed out; stopped only the process launched by this script. Log: $Log"
        }
        $Process.Refresh()
        $TestResult.ExitCode = $Process.ExitCode
        $ReportFile = Join-Path $Report 'index.json'
        $TestResult.Report = $ReportFile
        $Summary = & (Join-Path $PSScriptRoot 'Read-AutomationReport.ps1') -ReportFile $ReportFile -TestFilter $TestFilter
        foreach ($Field in @('TestCount', 'PassedCount', 'FailedCount', 'Tests')) { $TestResult[$Field] = $Summary.$Field }
        $TestResult.ReportFailureCategory = $Summary.FailureCategory
        if ($Process.ExitCode -ne 0) {
            $TestResult.FailureCategory = 'EditorExit'
            throw "Editor exited with $($Process.ExitCode); report status: $($Summary.FailureCategory). Log: $Log"
        }
        if ($Summary.FailureCategory) {
            $TestResult.FailureCategory = $Summary.FailureCategory
            throw "Automation report rejected ($($Summary.FailureCategory)). Report: $ReportFile"
        }
        $Summary.Tests | Format-Table -AutoSize
        $TestResult.Outcome = 'Passed'
        Write-Host "Passed $($Summary.TestCount) tests. Report: $ReportFile"
        } catch {
            if (-not $TestResult.FailureCategory) { $TestResult.FailureCategory = 'ExecutionError' }
            $TestResult.Error = $_.Exception.Message
            throw
        } finally {
            if (Test-Path -LiteralPath $Log -PathType Leaf) {
                $LogErrors = @(Select-String -LiteralPath $Log -Pattern '\bError:')
                $TestResult.LogErrorLines = $LogErrors.Count
                $TestResult.StartupClean = ($LogErrors.Count -eq 0)
                # 초기화 누락 6종을 수정했으므로 재발도 신규 오류로 처리한다.
                $Known = @()
                $Counts = @{}
                $Unexpected = @()
                foreach ($Entry in $LogErrors) {
                    $Line = $Entry.Line
                    $Signature = $Line -replace '^.*?(?:LogClass: Error: |LogAutomationTest: Error: LogClass: )', '' -replace ' \[log\]$', ''
                    if ($Known -ccontains $Signature) {
                        if (-not $Counts.ContainsKey($Signature)) { $Counts[$Signature] = 0 }
                        $Counts[$Signature]++
                        if ($Counts[$Signature] -le 2) { $TestResult.KnownBaselineDiagnostics++; continue }
                    }
                    $Unexpected += $Line
                }
                $TestResult.UnexpectedLogErrors = @($Unexpected)
                if ($LogErrors.Count -gt 0) {
                    if ($TestResult.Outcome -eq 'Passed') { $TestResult.Outcome = 'PassedWithDiagnostics' }
                    Write-Warning "$($LogErrors.Count) engine errors; known baseline $($TestResult.KnownBaselineDiagnostics), unexpected $($Unexpected.Count). Log: $Log"
                }
                if ($Unexpected.Count -gt 0 -and $TestResult.Outcome -eq 'PassedWithDiagnostics') {
                    $TestResult.Outcome = 'Failed'
                    $TestResult.FailureCategory = 'UnexpectedLogErrors'
                }
            }
            $TestResult | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8
        }
        if ($TestResult.Outcome -eq 'Failed') { throw "Automation failed ($($TestResult.FailureCategory)). Log: $Log" }
    }
    Write-Host "Completed $Action. Log: $Log"
} finally {
    if ($Acquired) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}
