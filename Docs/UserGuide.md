# DialogueMaker 사용자 안내서

## 목차

- [1. 기존 대화 기능 회귀 테스트](#1-기존-대화-기능-회귀-테스트)
- [2. 자동화 실행 결과와 실패 진단](#2-자동화-실행-결과와-실패-진단)

## 1. 기존 대화 기능 회귀 테스트

### 기능과 준비 사항

게임/플러그인 개발자가 기존 선택지 조건과 열람 이력 저장의 회귀를 확인하는 기능입니다. 신규 독립 Choice, 체크포인트, 장소별 대화 기능은 아직 구현 완료되지 않았습니다.

Windows PowerShell에서 DialogueMaker 프로젝트 루트로 이동합니다. UE 5.4.4와 프로젝트에 맞는 C++ 도구가 필요합니다. 실행 중인 에디터·Live Coding·다른 빌드는 먼저 정상 종료해야 합니다. 스크립트가 사용자 프로세스를 강제 종료하지 않습니다.

### 개발자 사용법

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Check
.\Scripts\AI\Invoke-UE.ps1 -Action Build
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter DialogueKit.Baseline -TimeoutMinutes 5
```

현재 접두사에는 다음 세 테스트가 등록되어 있습니다.

| 테스트 이름 | 확인하는 동작 |
|---|---|
| DialogueKit.Baseline.ChoiceConditions | 빈 조건 허용, 레벨 경계, 태그 충족 여부와 레벨/태그 동시 만족 |
| DialogueKit.Baseline.HistoryMemoryRoundTrip | Graph별 이력 격리, 메모리 직렬화/역직렬화, 저장 객체 간 변경 격리 |
| DialogueKit.Baseline.HistoryIsolatedFileRoundTrip | 테스트 전용 고유 슬롯의 파일 저장/복원과 정상 종료 경로의 정리 |

테스트가 사용하는 공개 API는 기존 `FDialogueChoice::IsPossibleToShow(const FPlayerCondition&)`, `UShownDialogueSaveData::SetShownDialogues(...)`, `GetShownDialogues(...)`입니다. 기존 이름 기반 Graph ID와 GUID 이력을 검사하는 용도이며 새 게임 연동 계약의 권장 API가 확정·완성됐다는 뜻은 아닙니다.

테스트 결과는 콘솔에 출력되는 실행별 `Report/index.json`과 `result.json`에서 확인합니다. 테스트 3개가 Success여도 초기화 오류·PIE·패키징 검증까지 통과한 것은 아닙니다.

### 시나리오 작가 사용법

이 항목은 개발자용 검증 기반이며 에디터 UI 메뉴를 추가하지 않습니다. 작가용 Block 단독 PIE 테스트 도구는 후속 구현 대상입니다.

### 확장 방법

`Plugins/DialogueKit/Source/DialogueKit/Private/Tests/`에 `WITH_DEV_AUTOMATION_TESTS`로 보호한 Unreal Automation 테스트를 추가하고 고유한 `DialogueKit.<기능>.<사례>` 이름을 등록합니다. Editor 전용 검증은 DialogueKitEditor 모듈의 테스트에서 처리합니다. 등록 후 다시 빌드하고 해당 이름/접두사로 실행합니다. 기존 테스트 파일은 [DialogueBaselineTests.cpp](../Plugins/DialogueKit/Source/DialogueKit/Private/Tests/DialogueBaselineTests.cpp)를 참고합니다.

### 주의사항과 검증 범위

- 파일 테스트는 실행마다 `DialogueKit_Automation_<GUID>` 슬롯을 생성합니다. 실제 사용자/게임 슬롯은 사용하지 않습니다. 정상 함수 반환과 실패 반환에서 테스트 소유 슬롯만 정리합니다. 프로세스 강제 종료 시 정리가 실행되지 않을 수 있으므로 이를 무조건 삭제하는 자동 정리는 하지 않습니다.
- 테스트 실행은 `-NullRHI -nosound`이므로 화면·입력·음향 검증을 대신하지 않습니다.
- 2026-10-14: Editor 빌드와 위 3개 테스트가 통과했습니다. 기존 구조체 초기화 오류 6종이 로그에서 12줄로 나타났으며 이 문제는 별도 수정 대상입니다.

## 2. 자동화 실행 결과와 실패 진단

### 기능과 준비 사항

게임 개발자가 테스트 성공과 실행 실패를 구분하는 도구입니다. `Invoke-UE.ps1 -Action Test`는 실행별 결과를 남기며, 엔진 프로세스를 시작한 뒤 실패해도 `result.json`에 원인을 기록합니다. 실행 전 환경 검사·프로세스 충돌 같은 선행 오류는 콘솔 오류로 보고합니다.

### 개발자 사용법

첫 항목의 Test 명령을 실행하고 콘솔에 표시된 실행 폴더의 `result.json`을 확인합니다.

| 필드 | 의미 |
|---|---|
| Outcome | Passed / PassedWithDiagnostics / Failed |
| FailureCategory | EditorExit, Timeout, NoReport, InvalidReport, ZeroTests, UnexpectedTests, TestFailure, UnexpectedLogErrors, ExecutionError |
| ReportFailureCategory | 엔진 종료 실패와 별도로 기록하는 보고서 문제 |
| ExitCode | 엔진 프로세스 종료 코드. 시작 실패/timeout 등에서는 없을 수 있음 |
| TestCount / PassedCount / FailedCount | 실제 보고서의 선택/성공/실패 수 |
| KnownBaselineDiagnostics | 현재 분리 추적하는 기존 초기화 오류 수 |
| UnexpectedLogErrors | 기존 범위에 없는 Error 로그. 정상 테스트 뒤에도 발견되면 실패 |
| Log / Report | 해당 실행의 로그와 보고서 경로 |

`PassedWithDiagnostics`는 깨끗한 시작 상태를 의미하지 않습니다. 현재 구조체 초기화 오류 6종만 각 최대 2줄까지 알려진 진단으로 구분합니다. 문제가 수정되면 `Invoke-UE.ps1`의 해당 기준도 제거해 재발을 허용하지 않아야 합니다.

보고서 판정만 빠르게 확인하려면 다음을 실행합니다. 실제 게임 테스트와 별개로 합성 보고서 9개를 검사합니다.

```powershell
.\Scripts\AI\Test-AutomationReport.ps1
```

보고서를 다른 개발 도구에서 검사할 때는 다음 공개 스크립트 진입점을 사용할 수 있습니다.

```powershell
$summary = .\Scripts\AI\Read-AutomationReport.ps1 -ReportFile '경로/Report/index.json' -TestFilter 'DialogueKit.Baseline'
# 판정기는 예외 대신 FailureCategory와 검사 수를 반환한다.
if ($summary.FailureCategory) { throw $summary.FailureCategory }
```

선택한 정확한 테스트 이름 또는 점으로 구분되는 하위 이름만 허용합니다. 보고서 누락·손상·0개·미실행·실패·다른 범위의 테스트를 성공으로 인정하지 않습니다.

### 시나리오 작가 사용법

작가용 에디터 UI는 추가하지 않습니다. 개발자에게 실행 폴더의 결과를 전달하여 문제를 확인하는 도구입니다.

### 확장 방법

새 테스트는 기존 보고서 형식을 유지하며 등록하고, 새로운 판정 규칙은 `Test-AutomationReport.ps1`에 성공·실패 경계 사례를 추가해 검증합니다. 이 도구는 게임 런타임 API가 아니며 플러그인에 설치할 게임 상태 제공자와는 별개입니다.

### 주의사항과 검증 범위

보고서 판정 9개와 실제 UE의 baseline 3개 성공 경로, 존재하지 않는 테스트 요청의 실패 결과 기록을 확인했습니다. 실제 장시간 timeout/프로세스 강제 종료 분기는 아직 별도 실험하지 않았습니다. 결과 파일은 로컬 Saved/AI에 생성되며 사용자 안내서와 스크립트만 Git에서 관리합니다.
