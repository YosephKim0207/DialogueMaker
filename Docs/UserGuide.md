# DialogueMaker 사용자 안내서

## 목차

- [1. 기존 대화 기능 회귀 테스트](#1-기존-대화-기능-회귀-테스트)
- [2. 자동화 실행 결과와 실패 진단](#2-자동화-실행-결과와-실패-진단)
- [3. 지원 언어 목록의 게임 빌드 호환](#3-지원-언어-목록의-게임-빌드-호환)
- [4. 샘플 맵 패키징 검증](#4-샘플-맵-패키징-검증)

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

## 3. 지원 언어 목록의 게임 빌드 호환

### 기능과 개발자 사용법

기존 언어 목록 조회가 Editor 전용 메타데이터 때문에 Game 타깃에서 컴파일되지 않던 문제를 수정했습니다. 공개 API와 지원 언어 12개는 유지합니다. 사용하는 게임 모듈의 Build.cs에 `DialogueKit` 의존성을 추가하고 다음 공개 C++ API를 호출합니다.

```cpp
#include "DialogueLocalizationUtility.h"

TArray<FDialogueLanguageMapping> Mappings;
FDialogueLocalizationUtility::GetSupportedLanguageMappings(Mappings);
FString KoreanCulture = FDialogueLocalizationUtility::ToCultureCode(ELanguage::Korean);
// KoreanCulture == TEXT("ko-KR")
```

`GetSupportedLanguageMappings(TArray<FDialogueLanguageMapping>& OutMappings)`는 기존 출력 배열을 비우고 Language와 CultureCode 쌍으로 채웁니다. 반환형은 void이며 enum 조회 실패 시 빈 배열을 돌려줍니다. `ToCultureCode(ELanguage)`는 문화권 코드 FString을 반환하고 정의되지 않은 값에는 en-US를 반환합니다. 동기 조회이며 게임 상태 변경·저장·비동기 요청을 수행하지 않습니다.

### 시나리오 작가 사용법

이 수정은 새 에디터 메뉴나 설정을 추가하지 않습니다. 기존 대사 작성 절차는 그대로 사용합니다.

### 확장 방법과 제한

현재 언어 추가는 `ELanguage`와 `ToCultureCode` 매핑을 함께 수정하고 테스트의 기대 목록을 갱신하는 C++ 작업입니다. 동적 언어 제공자 인터페이스는 없습니다. `Hidden` 메타데이터는 Editor에서만 확인할 수 있습니다. 향후 게임에서 숨겨야 할 언어를 추가하면 명시적 런타임 지원 목록을 별도로 구현해야 합니다.

### 검증

Game 및 Editor Win64 Development 빌드를 검증합니다. 매핑 개수·중복·공개 변환 일치·한국어/영어 유지를 확인하는 테스트는 다음 명령으로 실행합니다.

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter DialogueKit.Localization -TimeoutMinutes 5
```

이 테스트는 언어 목록을 검증하며 실제 번역 리소스나 화면 출력, 패키징 전체를 보증하지 않습니다.

## 4. 샘플 맵 패키징 검증

### 기능과 개발자 사용법

UE 5.4.4와 C++ 빌드 도구가 설치된 Windows에서 에디터와 다른 빌드를 정상 종료한 뒤 프로젝트 루트에서 실행합니다.

```powershell
.\Scripts\AI\Invoke-PackageValidation.ps1 -Action Check
.\Scripts\AI\Invoke-PackageValidation.ps1 -Action Package -TimeoutMinutes 30
```

`Check`가 기본 동작이며 환경과 RunUAT 경로를 확인합니다. `Package`는 Win64 Development로 Build/Cook/Stage/Package/Archive를 수행합니다. `-EngineRoot`는 엔진 경로, `-Map`은 맵의 패키지 경로(기본 `/DialogueKit/Sample/Maps/NewWorld`), `-TimeoutMinutes`는 1~240분 제한입니다. 맵 경로는 영문·숫자·밑줄·슬래시만 지원합니다.

출력은 매번 고유한 `Saved/AI/Package_<시각>_<GUID>/` 안에 생성합니다. `result.json`의 Outcome이 `PackagedNotLaunched`이고 ExitCode가 0이면 UAT 성공과 새 실행 파일·pak의 존재를 확인한 상태입니다. `Archive`는 배포 형태의 출력, `Staged`는 중간 배치, `Package.log`는 원문 로그입니다. 자동 게임 실행은 하지 않습니다.

실행 후 실패는 `AutomationToolExit`, `Timeout`, `MissingArtifacts`, `ExecutionError`로 기록하고 명령도 실패합니다. 환경/다른 프로세스 검사에서 거부되면 실행 폴더 생성 전 콘솔 오류가 표시됩니다. 시간 초과 시 도구 자신이 시작한 프로세스 트리만 종료합니다. 기존 저장 파일이나 Content 에셋은 수정하지 않습니다. Cooker의 Sentry 자동 초기화만 실행 인자로 끄며 패키지에 저장된 게임 설정은 바꾸지 않습니다.

### 시나리오 작가 사용법

이 도구는 개발자 명령행 검증용이며 작가용 UI는 추가하지 않습니다. Block 단독 연출 테스트는 후속 기능입니다.

### 확장 방법과 주의사항

다른 맵은 `-Map /Game/Maps/맵이름` 형태로 지정하되 현재 인자 검증이 허용하는 영문 패키지 이름을 사용합니다. 실제 게임의 Asset Manager/cook 설정과 구버전 보존 에셋 포함은 별도 확인해야 합니다. 현재 검사는 실행 파일과 pak 존재까지이며 IoStore 파일 전체 무결성이나 런타임 동작을 보증하지 않습니다. 다른 터미널/IDE에서 수동 RunUAT를 동시에 실행하지 마세요. 공통 mutex는 이 저장소의 자동화 도구끼리만 직렬화합니다.

### 검증

2026-10-14 기본 샘플 맵에서 UAT ExitCode 0, 실행 파일과 pak 생성, 로그 Error 0건을 확인했습니다. 최초 실제 UAT 컴파일 실패도 Failed/AutomationToolExit로 기록됐습니다. 실제 시간 초과 분기는 실험하지 않았습니다. 패키지 화면·입력·음향 및 PIE는 미검증입니다. 결과와 패키지 원본은 로컬 Saved에 두고 도구와 문서만 Git에서 관리합니다.
