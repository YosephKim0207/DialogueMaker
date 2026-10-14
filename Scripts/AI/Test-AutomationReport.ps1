[CmdletBinding()]
param()
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$RunRoot = Join-Path $ProjectRoot ('Saved/AI/ReportValidation_' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $RunRoot | Out-Null
$Reader = Join-Path $PSScriptRoot 'Read-AutomationReport.ps1'
$Results = @()
$Cases = @(
    @{ Name = 'Success'; Json = '{"tests":[{"fullTestPath":"DialogueKit.Baseline.One","state":"Success"}]}'; Category = $null; Count = 1; Passed = 1; Failed = 0 },
    @{ Name = 'Failed'; Json = '{"tests":[{"fullTestPath":"DialogueKit.Baseline.One","state":"Fail"}]}'; Category = 'TestFailure'; Count = 1; Passed = 0; Failed = 1 },
    @{ Name = 'Pending'; Json = '{"tests":[{"fullTestPath":"DialogueKit.Baseline.One","state":"NotRun"}]}'; Category = 'TestFailure'; Count = 1; Passed = 0; Failed = 1 },
    @{ Name = 'Zero'; Json = '{"tests":[]}'; Category = 'ZeroTests'; Count = 0; Passed = 0; Failed = 0 },
    @{ Name = 'Missing'; Json = $null; Category = 'NoReport'; Count = 0; Passed = 0; Failed = 0 },
    @{ Name = 'Malformed'; Json = '{'; Category = 'InvalidReport'; Count = 0; Passed = 0; Failed = 0 },
    @{ Name = 'NullTests'; Json = '{"tests":null}'; Category = 'InvalidReport'; Count = 0; Passed = 0; Failed = 0 },
    @{ Name = 'WrongScope'; Json = '{"tests":[{"fullTestPath":"DialogueKit.BaselineOther.One","state":"Success"}]}'; Category = 'UnexpectedTests'; Count = 1; Passed = 1; Failed = 0 },
    @{ Name = 'ExactName'; Json = '{"tests":[{"fullTestPath":"DialogueKit.Baseline","state":"Success"}]}'; Category = $null; Count = 1; Passed = 1; Failed = 0 }
)
# 별도 생성한 합성 보고서만 사용하여 판정기의 성공/실패 경계를 검사한다.
foreach ($Case in $Cases) {
    $File = Join-Path $RunRoot ($Case.Name + '.json')
    if ($null -ne $Case.Json) { Set-Content -LiteralPath $File -Value $Case.Json -Encoding UTF8 }
    $Actual = & $Reader -ReportFile $File -TestFilter 'DialogueKit.Baseline'
    $OK = ($Actual.FailureCategory -eq $Case.Category -and $Actual.TestCount -eq $Case.Count -and
        $Actual.PassedCount -eq $Case.Passed -and $Actual.FailedCount -eq $Case.Failed)
    $Results += [pscustomobject]@{ Case = $Case.Name; Passed = $OK; Actual = $Actual }
}
$Results | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $RunRoot 'result.json') -Encoding UTF8
$Results | Select-Object Case,Passed | Format-Table -AutoSize
if (@($Results | Where-Object { -not $_.Passed }).Count -gt 0) { throw "Report validation failed: $RunRoot" }
Write-Host "Passed $($Results.Count) report checks. Evidence: $RunRoot"
