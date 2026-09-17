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
            $Process.Kill()
            $Process.WaitForExit()
            throw "Automation timed out; stopped only the process launched by this script. Log: $Log"
        }
        $Process.Refresh()
        if ($Process.ExitCode -ne 0) { throw "Editor exited with $($Process.ExitCode). Log: $Log" }
        $ReportFile = Join-Path $Report 'index.json'
        if (-not (Test-Path -LiteralPath $ReportFile)) { throw "No fresh test report: $ReportFile" }
        $Results = Get-Content -LiteralPath $ReportFile -Raw | ConvertFrom-Json
        $Tests = @($Results.tests)
        if ($Tests.Count -eq 0) { throw 'Zero selected tests is not a pass.' }
        $Bad = @($Tests | Where-Object { $_.state -ne 'Success' })
        $Tests | Select-Object fullTestPath,state | Format-Table -AutoSize
        if ($Bad.Count -gt 0) { throw "$($Bad.Count) tests did not succeed. Report: $ReportFile" }
        $LogErrors = @(Select-String -LiteralPath $Log -Pattern '\bError:' )
        @{ Action = $Action; ExitCode = $Process.ExitCode; TestCount = $Tests.Count; Report = $ReportFile; Log = $Log; LogErrorLines = $LogErrors.Count } |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8
        if ($LogErrors.Count -gt 0) { Write-Warning "$($LogErrors.Count) error lines in the engine log. Selected test success does not imply clean project startup. Inspect: $Log" }
        Write-Host "Passed $($Tests.Count) tests. Report: $ReportFile"
    }
    Write-Host "Completed $Action. Log: $Log"
} finally {
    if ($Acquired) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}
