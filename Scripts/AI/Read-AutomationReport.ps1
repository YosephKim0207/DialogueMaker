[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ReportFile,
    [Parameter(Mandatory = $true)][string]$TestFilter
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# 새 실행의 보고서와 요청 범위를 검사하며 실패도 구조화된 결과로 반환한다.
$Summary = [ordered]@{ FailureCategory = $null; TestCount = 0; PassedCount = 0; FailedCount = 0; Tests = @() }
if (-not (Test-Path -LiteralPath $ReportFile -PathType Leaf)) {
    $Summary.FailureCategory = 'NoReport'
    return [pscustomobject]$Summary
}
try {
    $Data = Get-Content -LiteralPath $ReportFile -Raw | ConvertFrom-Json
    if ($null -eq $Data -or $null -eq $Data.PSObject.Properties['tests'] -or $null -eq $Data.tests) {
        $Summary.FailureCategory = 'InvalidReport'
        return [pscustomobject]$Summary
    }
    $Tests = @($Data.tests)
    $Summary.TestCount = $Tests.Count
    if ($Tests.Count -eq 0) {
        $Summary.FailureCategory = 'ZeroTests'
        return [pscustomobject]$Summary
    }
    foreach ($Test in $Tests) {
        if ($null -eq $Test -or $null -eq $Test.PSObject.Properties['fullTestPath'] -or
            $null -eq $Test.PSObject.Properties['state'] -or
            $Test.fullTestPath -isnot [string] -or $Test.state -isnot [string]) {
            $Summary.FailureCategory = 'InvalidReport'
            return [pscustomobject]$Summary
        }
        $Name = $Test.fullTestPath
        # 접두사와 정확히 일치하거나 점으로 구분되는 하위 테스트만 인정한다.
        if (-not ($Name.Equals($TestFilter, [StringComparison]::OrdinalIgnoreCase) -or
            $Name.StartsWith($TestFilter + '.', [StringComparison]::OrdinalIgnoreCase))) {
            $Summary.FailureCategory = 'UnexpectedTests'
        }
        if ($Test.state -ceq 'Success') { $Summary.PassedCount++ } else { $Summary.FailedCount++ }
    }
    $Summary.Tests = @($Tests | Select-Object fullTestPath,state)
    if ($null -eq $Summary.FailureCategory -and $Summary.FailedCount -gt 0) {
        $Summary.FailureCategory = 'TestFailure'
    }
} catch {
    $Summary.FailureCategory = 'InvalidReport'
}
[pscustomobject]$Summary
