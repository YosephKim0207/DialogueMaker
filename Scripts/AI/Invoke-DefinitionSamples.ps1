[CmdletBinding()]
param(
    [ValidateSet('Validate', 'Create')][string]$Action = 'Validate',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.4',
    [ValidateRange(1, 30)][int]$TimeoutMinutes = 5
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$InvokeUE = Join-Path $PSScriptRoot 'Invoke-UE.ps1'
if ($Action -eq 'Validate') {
    & $InvokeUE -Action Test -EngineRoot $EngineRoot -TestFilter DialogueKit.Samples -TimeoutMinutes $TimeoutMinutes
    exit 0
}
# 엔진/프로젝트 확인 후 공통 잠금으로 빌드/테스트/생성의 동시 실행을 방지한다.
& $InvokeUE -Action Check -EngineRoot $EngineRoot
if ($LASTEXITCODE -ne 0) { throw 'UE environment check failed.' }
$Mutex = New-Object System.Threading.Mutex($false, 'Local\UE544-AI-Verification')
$Acquired = $false
try {
    try { $Acquired = $Mutex.WaitOne(0) }
    catch [System.Threading.AbandonedMutexException] { $Acquired = $true }
    if (-not $Acquired) { throw 'Another AI verification is running.' }
    $Busy = @(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool,MSBuild,LiveCodingConsole -ErrorAction SilentlyContinue)
    if ($Busy.Count -gt 0) { throw 'Close the existing editor/build normally before creating samples.' }
    $RunRoot = Join-Path $ProjectRoot ('Saved\AI\DefinitionSamples_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '_' + [guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $RunRoot | Out-Null
    $Log = Join-Path $RunRoot 'Create.log'
    $Result = [ordered]@{ Action = 'CreateDefinitionSamples'; Outcome = 'Failed'; ExitCode = $null; Log = $Log; Error = $null; Assets = @() }
    try {
        $ProjectName = Split-Path $ProjectRoot -Leaf
        $ProjectFile = Join-Path $ProjectRoot "$ProjectName.uproject"
        $Arguments = @(
            ('"' + $ProjectFile + '"'),
            '-run=DialogueCreateDefinitionSamples',
            ('"-abslog=' + $Log + '"'),
            '-unattended', '-nop4', '-nosplash', '-NullRHI', '-nosound',
            '-ini:Engine:[/Script/Sentry.SentrySettings]:InitAutomatically=False'
        ) -join ' '
        $Process = New-Object System.Diagnostics.Process
        $Process.StartInfo.FileName = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
        $Process.StartInfo.Arguments = $Arguments
        $Process.StartInfo.WorkingDirectory = $ProjectRoot
        $Process.StartInfo.UseShellExecute = $false
        $Process.StartInfo.CreateNoWindow = $true
        if (-not $Process.Start()) { throw 'Unable to start the sample creation commandlet.' }
        if (-not $Process.WaitForExit($TimeoutMinutes * 60000)) {
            # 이 래퍼가 시작한 프로세스만 종료하며 기존 에디터는 건드리지 않는다.
            $Process.Kill()
            $Process.WaitForExit()
            throw 'Sample creation timed out. Partial new files are preserved.'
        }
        $Result.ExitCode = $Process.ExitCode
        if ($Process.ExitCode -ne 0) { throw "Commandlet failed ($($Process.ExitCode)). Existing assets are never overwritten. See $Log" }
        $Errors = @(Select-String -LiteralPath $Log -Pattern '\bError:')
        if ($Errors.Count -gt 0) { throw "Commandlet log has $($Errors.Count) error lines." }
        $Folder = Join-Path $ProjectRoot 'Plugins\DialogueKit\Content\Sample\GameDefinitions'
        foreach ($Name in @('DA_SampleNPC_Merchant', 'DA_SampleNPC_Guard', 'DT_SampleItems', 'DT_SampleSkills', 'DA_SampleDefinitionCatalog')) {
            $Asset = Join-Path $Folder "$Name.uasset"
            if (-not (Test-Path -LiteralPath $Asset -PathType Leaf)) { throw "Expected asset missing: $Asset" }
            $Result.Assets += @{ Path = $Asset; SHA256 = (Get-FileHash -LiteralPath $Asset -Algorithm SHA256).Hash }
        }
        $Result.Outcome = 'CreatedNotVisuallyInspected'
    } catch {
        $Result.Error = $_.Exception.Message
        throw
    } finally {
        if (Get-Variable Process -ErrorAction SilentlyContinue) { if ($null -ne $Process) { $Process.Dispose() } }
        $Result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8
        Write-Output "Sample creation result: $RunRoot"
    }
} finally {
    if ($Acquired) { $Mutex.ReleaseMutex() }
    $Mutex.Dispose()
}
