[CmdletBinding()]
param([switch]$IncludePackage)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$Id = [guid]::NewGuid().ToString('N')
$Previous = $env:DIALOGUE_VALIDATION_ID
$RunRoot = Join-Path $ProjectRoot ("Saved/AI/RestoreValidation/" + $Id)
New-Item -ItemType Directory -Path $RunRoot -Force | Out-Null
$Result = [ordered]@{ Id = $Id; Outcome = 'Failed'; ProcessRestart = $false; CookVersions = $false; Error = $null }
function Invoke-Step([string]$Script, [string[]]$StepArguments) {
    # 별도 PowerShell과 UE 프로세스가 종료된 다음 다음 단계로 진행한다.
    & "$PSHOME/powershell.exe" -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot $Script) @StepArguments
    if ($LASTEXITCODE -ne 0) { throw "Validation step failed: $Script $StepArguments" }
}
try {
    $env:DIALOGUE_VALIDATION_ID = $Id
    Invoke-Step 'Invoke-UE.ps1' @('-Action', 'Test', '-TestFilter', 'ProjectCheckpoint.Write', '-TimeoutMinutes', '5')
    Invoke-Step 'Invoke-UE.ps1' @('-Action', 'Test', '-TestFilter', 'ProjectCheckpoint.Read', '-TimeoutMinutes', '5')
    $Result.ProcessRestart = $true
    if ($IncludePackage) {
        Invoke-Step 'Invoke-PackageValidation.ps1' @('-Action', 'Package', '-DialogueValidationId', $Id, '-TimeoutMinutes', '30')
        Invoke-Step 'Invoke-UE.ps1' @('-Action', 'Test', '-TestFilter', 'ProjectCook', '-TimeoutMinutes', '5')
        $Result.CookVersions = $true
    }
    $Result.Outcome = 'Passed'
} catch {
    $Result.Error = $_.Exception.Message
    throw
} finally {
    $Result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8
    $env:DIALOGUE_VALIDATION_ID = $Previous
    Write-Host "Restore validation evidence: $RunRoot"
}
