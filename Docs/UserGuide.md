# DialogueMaker 사용자 안내서

## 목차

- [1. 기존 대화 기능 회귀 테스트](#1-기존-대화-기능-회귀-테스트)
- [2. 자동화 실행 결과와 실패 진단](#2-자동화-실행-결과와-실패-진단)
- [3. 지원 언어 목록의 게임 빌드 호환](#3-지원-언어-목록의-게임-빌드-호환)
- [4. 샘플 맵 패키징 검증](#4-샘플-맵-패키징-검증)
- [5. 안정 Block 식별자](#5-안정-block-식별자)
- [6. Node와 Choice 식별 계약](#6-node와-choice-식별-계약)
- [7. 범용 진단 연결](#7-범용-진단-연결)
- [8. 게임 상태 제공과 조건 평가](#8-게임-상태-제공과-조건-평가)
- [9. 효과 묶음과 게임 처리 계약](#9-효과-묶음과-게임-처리-계약)
- [10. 체크포인트 요청과 저장 결과 계약](#10-체크포인트-요청과-저장-결과-계약)
- [11. Chapter와 Scene 정의 및 소속 계약](#11-chapter와-scene-정의-및-소속-계약)
- [12. 공통 계약 연결 검증](#12-공통-계약-연결-검증)
- [13. Line 노드와 Block 실행 기반](#13-line-노드와-block-실행-기반)
- [14. 독립 Choice와 조건별 선택지](#14-독립-choice와-조건별-선택지)
- [15. Branch 조건 분기와 자동 순환 방어](#15-branch-조건-분기와-자동-순환-방어)
- [16. 상태 변경 노드와 게임 효과 경계](#16-상태-변경-노드와-게임-효과-경계)
- [17. End 완료·Subsystem 및 UI 연동](#17-end-완료subsystem-및-ui-연동)

- [18. 효과 상태와 실행 원장 복원](#18-효과-상태와-실행-원장-복원)

- [19. 게임 소유 FIFO와 비동기 합본 저장 샘플](#19-게임-소유-fifo와-비동기-합본-저장-샘플)

- [20. Block 로드 전 대상과 조건 필터](#20-block-로드-전-대상과-조건-필터)

## 1. 기존 대화 기능 회귀 테스트

### 기능과 준비 사항

게임/플러그인 개발자가 기존 선택지 조건과 열람 이력 저장의 회귀를 확인하는 기능입니다. 독립 Choice와 최소 실행기는 S2에서 추가했으며 13~17절을 참고합니다. 체크포인트 저장과 장소별 후보 선정은 후속 단계입니다.

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

## 5. 안정 Block 식별자

### 기능과 현재 범위

신규 Dialogue Graph에 이름·경로와 별개인 GUID를 부여합니다. `FDialogueBlockReference`는 `BlockId`와 `ContentVersion` 쌍이며 같은 ID의 다른 콘텐츠 버전은 구분됩니다. 현재는 식별 기반만 추가됐습니다. 기존 이름 기반 `GetPrimaryAssetId()`, 지역화, 열람 저장과 후보 선정은 아직 새 ID로 전환되지 않았습니다. 따라서 이 기능만으로 기존 에셋 이름 변경 후 저장/지역화 호환을 보장하지 않습니다.

### 게임 개발자 사용법

게임 모듈에서 `DialogueKit`에 의존하고 `DialogueGraph.h`를 포함합니다. 로드된 Graph에서 공개 API를 조회합니다.

```cpp
// Graph는 유효한 UDialogueGraph 포인터다.
const FGuid Id = Graph->GetBlockId();
const int32 Version = Graph->GetContentVersion();
const FDialogueBlockReference Reference = Graph->GetBlockReference();
if (!Reference.IsValid())
{
    // 기존 프로토타입 또는 누락된 ID: 게임/제작 도구의 오류 처리 경로로 반환한다.
}
```

세 getter는 Blueprint에서도 `DialogueKit|Identity` 범주의 Pure 함수입니다. 조회는 ID를 생성하거나 저장하지 않습니다. `IsValid()`는 유효한 GUID와 양수 버전을 요구합니다. 기본 콘텐츠 버전은 1이며 콘텐츠 버전은 저장 형식 버전/Line 열람 버전과 다릅니다.

C++ 제작 도구에서 새 `UDialogueGraph`를 생성했다면 `InitializeBlockIdentity()`를 명시 호출합니다. 새 ID를 부여한 경우에만 true이며 이미 ID가 있거나 CDO/Archetype 같은 템플릿이면 false입니다. Blueprint 실행용 함수로 노출하지 않습니다. 기존 에셋을 읽을 때 이 함수를 자동 호출해서는 안 됩니다. 이 함수 자체는 트랜잭션·디스크 저장을 수행하지 않으므로 별도 저작 도구를 만들면 해당 도구가 Undo/에셋 저장을 처리해야 합니다.

영속 에셋 카탈로그의 참조 배열을 검사할 때는 다음 API를 사용합니다.

```cpp
TArray<int32> InvalidIndices;
TArray<int32> DuplicateIndices;
const bool bValid = FDialogueBlockIdentity::ValidateCatalog(
    References, InvalidIndices, DuplicateIndices);
```

`References`는 `TArray<FDialogueBlockReference>`입니다. 반환값 false이면 누락/잘못된 버전 또는 동일 ID·버전 중복이 있습니다. 출력 배열은 매 호출 초기화되며 중복의 첫 항목 이후 인덱스만 보고합니다. 같은 ID의 다른 버전은 허용합니다. PIE/World/실행 복제본은 카탈로그에 넣지 않습니다. 자동 Asset Registry 수집이나 진단 UI를 수행하는 함수는 아닙니다.

### 시나리오 작가 사용법

콘텐츠 브라우저에서 기존 `Dialogue → Dialogue Graph` 생성 경로를 이용하면 Factory가 새 ID를 부여합니다. Graph를 열면 `DialoguePropertyTab`의 에셋 Details에 `DialogueKit|Identity` 값이 읽기 전용으로 노출됩니다. 값의 편집이나 버전 증가 버튼은 아직 추가하지 않았습니다. Factory 동작과 반영 필드는 자동 검증했지만 실제 UI 클릭/배치는 아직 수동 확인하지 않았습니다.

일반 에셋 복제는 새 Block ID와 버전 1을 얻고, World/PIE 실행 복제는 원본 참조를 유지합니다. 일반 복제는 기존 Block의 새 버전을 만드는 기능이 아닙니다. 신규 버전 저작 도구는 후속 작업입니다. 기존 프로토타입 에셋은 로드·저장만으로 자동 ID를 받지 않으며 합의한 수동 재작성 대상입니다.

### 확장과 검증 범위

정확한 참조는 해시/동등 비교를 지원해 `TMap`/`TSet` 키로 사용할 수 있습니다. 직렬화되는 USTRUCT이지만 실제 저장 조율·복원·구버전 해석은 후속 S3/S4 작업입니다. Node/Choice 식별과 Line 열람 버전 확장은 이번 항목에 포함되지 않습니다.

2026-10-14 S1-1 구현 당시: Editor 빌드, Factory·이름 변경·일반/World/PIE 복제·템플릿 방어·메모리 직렬화·카탈로그 검증을 포함한 DialogueKit 7개 테스트, Game 빌드/cook/package가 통과했습니다. 실제 legacy package 저장/재로드와 UI/PIE 관찰은 별도 미검증입니다.

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter DialogueKit.Contracts.BlockIdentity -TimeoutMinutes 5
```

## 6. Node와 Choice 식별 계약

기존 `NodeGuid`를 노드 식별자로 재사용하고 `FDialogueChoice::ChoiceId`로 선택 문구/순서와 분리합니다. 기존 저장 형식과 지역화 PinId는 유지합니다.

개발자는 `UDialogueNodeInfo::AddDialogueChoice(const FDialogueChoice&)`로 선택을 추가합니다. 이 명시적 편집 호출에서 누락 ID와 같은 목록의 중복 ID는 새로 부여합니다. Graph 전체 중복 검사는 `ValidateIdentifiers`가 담당합니다. `EnsureChoiceIds()`는 명시 편집 전용이며 로드·조회에는 호출하지 않습니다. `GetReadVersion()`은 현재 열람 버전(기본 1), `AdvanceReadVersion()`은 명시 갱신용이며 잘못된 값/최대값이면 false로 무변경입니다. Undo와 저장은 호출하는 편집 도구 책임입니다. NodeInfo의 일반 DuplicateObject는 편집/실행 왕복 때문에 ID와 열람 버전을 유지합니다.

작가는 기존 Line Details의 선택 배열을 편집합니다. ID가 있는 선택의 순서를 바꿔도 연결 핀과 목적지가 함께 유지됩니다. ID 없는 기존 프로토타입은 최초 명시 편집에서 기존 순서대로 핀을 승계하며, 그 전에 이미 바뀐 순서를 역추정하지 않습니다. 미열람 갱신 버튼과 독립 Choice 노드는 아직 후속 S5/S2 작업입니다.

`UDialogueRuntimeGraph::ValidateIdentifiers`는 현재 식별 오류의 문자열 코드 목록을 반환하는 검사 기반입니다. false여도 자료를 자동 고치지 않으며, Graph 검사 UI는 아직 추가하지 않았습니다. S5 노드 복제 도구는 NodeGuid·ChoiceId·PinId를 새로 부여해야 합니다. 노드 정보 복제 함수에 재발급을 넣으면 저장 왕복마다 ID가 바뀌므로 금지합니다.

Editor 핀의 PersistentGuid와 RuntimePin.ChoiceId를 직렬화 변환에서 함께 보존합니다. 자동화는 재정렬 시 핀/연결 보존, 자기 배열의 선택 복제, NodeInfo 메모리 왕복, 누락/중복, 열람 버전 경계를 검사합니다. 실제 전체 Graph 에셋 저장·재열기 및 Undo UI는 S2/S5에서 검증합니다. 기존 에셋의 자동 일괄 재작성은 하지 않습니다.

## 7. 범용 진단 연결

게임은 `UDialogueDiagnosticRouter`를 NewObject 또는 Blueprint의 Construct Object로 생성하고 UPROPERTY로 수명을 유지합니다. C++에서는 `OnDiagnosticNative.AddUObject`/`AddLambda`로 구독하고 반환 FDelegateHandle로 Remove합니다. Blueprint에서는 `OnDiagnostic` 이벤트를 바인딩/해제합니다. 두 경로에 동시에 같은 전송을 연결하면 중복 전송되므로 하나를 선택합니다.

`ReportDiagnostic(FDialogueDiagnostic)`는 로컬 로그를 남기고 같은 진단을 구독자에게 전달한 뒤 반환합니다. `Code`, `Severity`, `Message`, `Handling`, `Context`의 Block ID/콘텐츠 버전·NodeGuid·ExecutionId·SaveRequestId를 제공하며 GameName 빈 값은 현재 프로젝트 이름, Component는 DialogueMaker입니다. 게임 이름은 전송 목적지를 결정하지 않습니다. 모든 호출은 게임 스레드에서 수행합니다.

```cpp
FDelegateHandle Handle = Router->OnDiagnosticNative.AddLambda(
	[](const FDialogueDiagnostic& Diagnostic)
	{
		// 게임에서 초기화한 Sentry의 메시지별 scope에 game/component/code 및 Context를 넣는다.
		// 이곳에 게임의 CaptureMessageWithScope 등 실제 로그 전송 코드를 연결한다.
		// SDK/DSN, 빈도 제한, 중복 억제, 개인정보 정책은 게임이 관리한다.
	});
// 게임 연동 종료 시 Router->OnDiagnosticNative.Remove(Handle);
```

조건 평가 등 C++ 호출부는 `Dispatch(Diagnostic, Router)`를 사용할 수 있으며 Router가 없으면 로컬 로그만 유지합니다. 로그 전송 여부로 조건/효과/저장 반환값을 변경하지 않습니다. Sentry SDK는 코어 의존성이 아니며 위 예시는 전송 삽입 위치입니다. 실제 외부 수신 검증은 수행하지 않았습니다. 작가용 UI는 없으며 향후 Graph 검사 UI에서 이 구조를 이용합니다. native 이벤트 전달·해제와 일부 Context 직렬화는 자동화로 검증했습니다. Blueprint 이벤트의 실제 바인딩 실행과 모든 필드 왕복은 아직 별도 검증하지 않았습니다. Code에는 호출자가 비어 있지 않은 안정 오류 코드를 지정해야 하며 게임 스레드 호출 계약을 지켜야 합니다.

## 8. 게임 상태 제공과 조건 평가

`DialogueConditionTypes.h`의 FDialogueStateKey는 Kind(Tag/Value/Count), TargetId, ItemId 또는 GameplayTag로 항목을 구분합니다. TargetId는 호감도 대상 NPC 등 상태 소유자를 뜻하며 Block 진입 대상과 별개입니다. None은 게임의 전역 상태로 사용할 수 있습니다. 표시 이름은 식별자가 아닙니다.

게임 개발자는 별도 UObject/Component에 `IDialogueDefinitionProvider`와 `IDialogueStateProvider`를 구현합니다. Blueprint에서도 두 인터페이스를 추가하고 구현할 수 있습니다. 정의 제공자의 `GetDialogueDefinitions(OutDefinitions)`는 이름/종류/키 목록, 상태 제공자의 `QueryDialogueTag(TargetId, Tag, OutValue)`, `QueryDialogueValue(TargetId, ItemId, OutValue)`, `QueryDialogueCount(TargetId, ItemId, OutValue)`는 현재 값만 반환합니다. C++에서는 같은 이름의 `_Implementation`을 override합니다. 반환 bool은 조회 성공이며 실제 bool/double/int64 값과 구분됩니다. 조회로 게임 상태·저장·커서를 바꾸면 안 됩니다.

공통 FDialogueStateDefinition 행 DataTable은 `ReadDefinitionTable(Table, OutDefinitions)`로 읽습니다. 중복/잘못된 키나 다른 행 타입이면 false와 빈 배열입니다. 기존 게임의 아이템 DataTable이 다른 행 구조라면 게임 정의 제공자가 공통 목록으로 변환합니다. 최대 중첩 수를 현재 수량으로 전달하지 않습니다.

```cpp
UDialogueSampleStateProvider* Provider = NewObject<UDialogueSampleStateProvider>();
FDialogueStateDefinition Definition;
Definition.Key.Kind = EDialogueStateKind::Count;
Definition.Key.ItemId = TEXT("Gift");
Provider->AddDefinition(Definition);
FDialogueCondition Condition;
Condition.Key = Definition.Key;
Condition.Comparison = EDialogueComparison::GreaterOrEqual;
Condition.CountOperand = 1;
FDialogueConditionSet Set;
Set.Conditions.Add(Condition);
FDialogueDiagnosticContext Context;
const FDialogueConditionResult Result = UDialogueConditionLibrary::EvaluateConditions(
    Set, Provider, false, Context, nullptr);
// 정의된 Gift의 미설정 샘플 수량은 0: bPassed=false, bHadErrors=false.
```

샘플의 `SetSampleState(NewState)`로 메모리 값만 설정하며 미등록 키/음수 Count/비유한 Value는 false입니다. 샘플은 명시 생성해야 하고 실제 게임 제공자가 없다고 자동 선택되지 않습니다. 값의 원본과 상태 수명은 게임 책임입니다. 제공자 객체는 게임이 UPROPERTY로 보관합니다.

EvaluateConditions의 bBranchCondition은 빈 Branch 진단 여부입니다. 빈 일반 조건은 True/무오류, 빈 Branch는 True/진단입니다. 오류 항목만 True로 바꾼 뒤 AND/OR로 종합하므로 AND의 다른 False는 유지되고 OR의 오류 항목은 최종 True를 만들 수 있습니다. bHadErrors와 Diagnostics에서 이를 구별합니다. 효과/저장 실패에는 이 정책을 적용하지 않습니다. Tag는 Equal/NotEqual만, 수치는 여섯 비교 연산을 지원합니다. double 동등 비교는 정확 비교이며 Count는 int64 그대로 비교합니다. Count의 현재 값은 0 이상이지만 비교 기준에는 음수도 허용하여 그대로 수치 비교합니다. 미구현 인터페이스 기본 구현은 false와 초기화된 출력값을 반환합니다. DataTable 반환 순서는 저작 정렬 기준이 아니므로 UI에서 표시 순서를 별도 정합니다.

작가용 새 조건 선택 UI는 S2/S5에서 연결합니다. 현재는 독립 공통 계약과 샘플이며 기존 FPlayerCondition/Branch/내장 선택지 소비처를 바꾸지 않았습니다. 게임별 새 아이템/능력치는 목록과 제공자만 확장하면 되고 플러그인 enum에 게임 항목을 추가할 필요가 없습니다. 중첩 그룹은 지원하지 않습니다.

## 9. 효과 묶음과 게임 처리 계약

게임은 `IDialogueEffectProvider::ApplyDialogueEffects`를 C++의 `_Implementation` 또는 Blueprint로 구현합니다. 입력 FDialogueEffectBatch의 Context.ExecutionId는 이번 실행 발생의 GUID이며 정적 ChoiceId가 아닙니다. 상태 원본의 전체 적용 가능 여부를 확인하고 전부 성공하거나 아무것도 변경하지 않아야 합니다.

호출자는 `UDialogueEffectLibrary::ApplyEffectBatch(Batch, Provider, Router)`를 사용합니다. 반환 FDialogueEffectResult의 Applied/AlreadyApplied만 성공입니다. Failed의 ErrorCode/Message로 게임이 안내·재시도·이후 흐름을 결정하며 플러그인은 팝업을 띄우지 않습니다. 빈 묶음/잘못된 키·연산/미구현 제공자는 실패입니다. 조건 평가의 오류 True 정책은 여기 적용하지 않습니다.

| 종류 | 연산과 값 |
|---|---|
| Tag | Add/Remove. 이미 같은 상태여도 성공 가능 |
| Value | Add/Set, 유한 double. Add의 음수는 감소 |
| Count | Add/Set/Remove, 0 이상 int64. Remove가 소비이며 잔량 부족·overflow는 실패 |

효과 대상 Key는 조건 계약과 같습니다. 예를 들어 Gift Count Remove 1과 NPC Charm Value Add 5를 Effects에 순서대로 넣습니다. 같은 항목을 여러 번 처리하면 작성된 순서가 의미를 가지며 요청 동일성에도 포함됩니다. 비활성 값과 Context.SaveRequestId 차이는 효과 내용 충돌로 취급하지 않습니다.

메모리 테스트에서는 `UDialogueSampleEffectProvider`를 생성하고 앞 절의 AddDefinition/SetSampleState를 사용합니다. 같은 실행 ID의 성공은 AlreadyApplied로 재전달하고 상태를 바꾸지 않습니다. 같은 ID의 다른 효과/순서/발생 Block·Node는 ExecutionPayloadConflict입니다. 실패도 최종 결과로 기록하므로 게임이 새 재시도를 허용할 때는 새 ExecutionId를 발급해야 합니다. 재전달과 정상 재실행을 혼동하지 마세요.

성공 직후 게임은 **게임 상태·실행/선택 기록·후속 재개 위치를 같은 시점에 캡처하여 공통 FIFO 저장 요청**에 포함해야 합니다. 인터페이스만으로 게임의 원자성이나 영속 중복 방지가 보장되지 않습니다. 샘플은 임시 배열에서 전량 적용 후 상태와 ledger를 확정하지만 메모리 구현이며 디스크 저장/프로세스 재시작을 처리하지 않습니다. S3에서 실제 저장 조율과 복원을 연결합니다.

작가용 StateChange/Choice 효과 목록 UI는 S2에서 연결합니다. 이번 기능은 호출 계약과 게임 교체용 샘플입니다. 자동화는 부분 실패 무변경, 성공/실패 재전달, payload 충돌, 정상 새 실행, 수량 경계, 비유한 값, 상태·ledger 메모리 직렬화를 검사합니다. 실제 게임의 효과/저장/재시작 검증과 구분합니다.

- 효과 Context의 Block/Node는 진단용 선택 정보이며 Graph 연결부는 실제 위치를 제공해야 합니다. ExecutionId는 필수입니다. 성공 반환의 ErrorCode는 wrapper가 비웁니다. 샘플은 실행 ledger 양쪽 중 하나만 남아 있으면 적용을 거부합니다. 손상된 음수 Count는 거부하고 Value Set은 유한한 값으로 교체 가능합니다.

## 10. 체크포인트 요청과 저장 결과 계약

개발자용 S1 계약입니다. 기존 SaveGame 형식이나 실행기를 교체하지 않습니다. 현재 그래프 UI에 저장 설정을 추가한 단계가 아니며 실제 FIFO·파일 저장·복원은 S3에서 연결합니다.

- `FDialogueCheckpointSnapshot`: CheckpointId, SchemaVersion=1, 재개 위치, 원인 선택/효과, **전체 누적 열람 이력** 값입니다. 게임 상태·전체 선택 기록·효과 중복 방지 ledger는 게임이 같은 저장 단위에 보존합니다. 임시/증분 이력만 넣지 않습니다.
- `FDialogueResumePosition`: Block ID/콘텐츠 버전, BlockRunId, 대화 대상 TargetTag, 위치입니다. `BeforeNode`는 **해당 노드 실행 직전**이므로 효과가 성공한 노드의 다음 노드를 지정합니다. `BlockStart`/`BlockCompleted`는 NodeGuid를 비우며, 활성 대화가 없으면 `None`과 빈 참조를 사용합니다. 종료 위치는 활성 커서가 아닙니다.
- `FDialogueBlockReadRecord`: BlockId별 완료 여부와 Line의 NodeGuid/ReadVersion입니다. 누적 완료는 후보의 영구 소진 조건이 아닙니다. 콘텐츠 버전·저장 스키마·Line 열람 버전은 별개입니다.
- `FDialogueCheckpointRequest`: RequestId·QueueGenerationId·Sequence와 스냅샷입니다. 재시도는 같은 CheckpointId/스냅샷을 보존하고 새 RequestId를 사용합니다. 복원/새 게임 등 경계에서는 게임 조율자가 세대를 바꿔 오래된 완료 통지를 배제합니다.
- `IDialogueCheckpointProvider::RequestDialogueCheckpoint`를 게임 UObject에 C++ `_Implementation` 또는 Blueprint로 구현하고 `UDialogueCheckpointLibrary::SubmitCheckpoint`에 넘깁니다. 반환 `bAccepted`는 **큐 접수**만 뜻합니다. 실패는 ErrorCode/Message와 진단 Router로 전달됩니다. 개발자가 UI와 이후 흐름을 결정합니다.
- 실제 비동기 저장 완료는 게임이 `FDialogueCheckpointCompletion`으로 전달합니다. `MatchesCompletion`은 요청 ID·세대·순서·결과 형식만 검사합니다. 중복/취소 callback은 게임이 아직 대기 중인 요청인지 확인해서 거부해야 합니다. 일치하는 Failed도 true이므로 반드시 `bSaved`를 확인합니다.

호출 순서 예제:

```cpp
// 효과가 전부 성공한 직후, 다음 게임 상태 변경 전에 실행한다.
FDialogueCheckpointRequest Request; // 게임 조율자가 ID/세대/Sequence를 채운다.
// Snapshot.Resume에는 선택 이후 위치, Trigger에는 원인 선택을 기록한다.
// Snapshot.FullReadHistory에는 이 시점의 전체 누적 이력을 값 복사한다.
// 게임 provider는 함수 반환 전에 게임 상태/ledger와 Request를 함께 복사한다.
const FDialogueCheckpointAcceptance Result =
    UDialogueCheckpointLibrary::SubmitCheckpoint(Request, GameSaveProvider, DiagnosticRouter);
if (Result.bAccepted == false)
{
    // 게임에서 오류 안내/재시도 정책을 처리한다. 마지막 성공 저장은 유지한다.
}
// 접수 후 디스크 완료를 기다리지 않고 진행한다. 저장 표시는 실제 큐 상태에 연결한다.
```

위 예제의 빈 Request는 그대로 호출하면 검증에 실패합니다. 실제 조립·복사 예제는 `Private/Tests/DialogueCheckpointTests.cpp`와 `UDialogueSampleCheckpointProvider`에 있습니다. 샘플은 요청의 값 복사만 누적하며 파일 기록이나 성공 통지를 흉내 내지 않습니다. 실게임 조율자로 사용하지 마세요.

저장 시점은 결과가 있는 Choice/StateChange의 성공 직후, 선택적 Choice 저장, Save on End가 켜진 정상 종료, 대체 Block의 준비·보정 완료 후 시작 경계입니다. Save on End 기본값은 꺼짐이며 비활성 정상 종료의 완료·열람은 캐시만 반영합니다. 매 Line/조건 평가/PIE 테스트에서 저장하지 않습니다. 저장 실패 재시도는 보존한 값 복사를 다시 제출하고 이후 변경된 live 상태를 읽지 않습니다.

검증: 스키마/위치/원인·열람 중복 검사, 값 복사 격리, 구조체 직렬화 왕복, 오래된 세대/재시도/순서 거부, 제공자 누락/미구현 실패를 자동 테스트합니다. 파일 원자성·실제 비동기 중단/복원·Blueprint 구현은 아직 검증 대상이 아닙니다.

- `Replacement`는 합의대로 대체 Block 처음부터 시작합니다. 이 원인에는 Choice/효과 실행 ID를 넣지 않습니다. 필요한 보정·보정 완료 기록은 게임 상태가 소유하며 기존 효과 재실행으로 표현하지 않습니다. 저장할 DTO는 Snapshot이며 Request/Acceptance/Completion은 런타임 전달용입니다. 미완료 큐를 성공 저장처럼 복원하지 않습니다.

## 11. Chapter와 Scene 정의 및 소속 계약

S1은 저작 자료형과 순수 소속 판정 API를 제공합니다. 스토리보드 창·드롭다운·이동/삭제 확인 UI는 S5, 실제 시작/Refresh 후보 선별과 Asset Registry 메타데이터 연결은 S4에서 구현합니다. 기존 `ChapterID` 필터 소비처를 이번 단계에서 교체하지 않습니다.

개발자는 게임 콘텐츠의 `UDialogueStoryDefinition`을 정의 원본으로 사용합니다. `AddChapter(DisplayName)`와 `AddScene(ChapterId, DisplayName)`은 새 GUID를 반환하며 잘못된 부모의 Scene 생성은 빈 GUID를 반환합니다. 일반 생성자·로드·저장은 ID를 발급하거나 재발급하지 않습니다. `FindChapter`/`FindScene`은 누락 또는 중복이면 nullptr입니다. 배열이 수정되면 반환 포인터를 다시 조회해야 합니다. `ValidateDefinitions`는 오류를 수집하되 입력을 수정하지 않습니다.

`Chapters`와 `Scenes`는 별도 배열입니다. Chapter가 제거돼도 Scene을 보존할 수 있는 형식이며, 실제 삭제 도구가 하위 Scene의 부모를 비우는 작업은 S5에서 구현합니다. 배열을 직접 수정하면 이 동작이 자동 실행되지는 않습니다. 표시 이름/설명/SortOrder 변경은 ID를 바꾸지 않습니다. 현재 배열은 자동 정렬되지 않으며 UI 정렬은 S5에서 SortOrder와 안정 ID를 기준으로 처리할 예정입니다.

| Block의 `StoryScope.Mode` | 입력할 식별자 | 신규 후보 소속 판정 |
|---|---|---|
| Unassigned | 사용하지 않음 | 제외 |
| Unrestricted | 사용하지 않음 | Chapter/Scene 제한만 생략 |
| Chapter | ChapterId, SceneId는 비움 | 현재 Chapter와 정확히 일치 |
| Scene | SceneId, ChapterId는 비움 | 정의에서 부모를 조회하고 현재 Chapter·Scene과 일치 |

Scene 소속의 부모 Chapter는 정의 원본에서 조회합니다. 따라서 Scene을 이동할 때 Block의 SceneId는 유지할 수 있습니다. 게임의 `FDialogueStoryProgress`는 현재 ChapterId·SceneId를 제공하며 두 값의 부모 관계도 검사합니다. 실제 게임 진행도 전환은 게임이 수행합니다.

```cpp
UDialogueStoryDefinition* Definition = NewObject<UDialogueStoryDefinition>();
const FGuid Chapter = Definition->AddChapter(FText::FromString(TEXT("첫 장")));
const FGuid Scene = Definition->AddScene(Chapter, FText::FromString(TEXT("광장")));
FDialogueStoryScope Scope;
Scope.Mode = EDialogueStoryScope::Scene;
Scope.SceneId = Scene;
FDialogueStoryProgress Progress;
Progress.ChapterId = Chapter;
Progress.SceneId = Scene;
const FDialogueStoryScopeResult Result =
    UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress);
// Result.bValid와 Result.bMatches를 모두 확인한다.
```

`EvaluateScope`는 Blueprint에서도 호출할 수 있습니다. 소속 없음/제한 없음은 정의를 조회하지 않으며 해당 모드의 남은 ID를 사용하지 않습니다. 다른 항목의 소속 오류까지 모든 Block에 전파하지 않고, 참조한 Chapter/Scene과 게임 진행의 정합성을 검사합니다. 잘못된 정의/소속은 ErrorCode와 bValid=false, bMatches=false를 반환합니다. 유효하지만 현재 진행과 다른 소속은 bValid=true, bMatches=false입니다. 조건 시스템의 조회 실패 True 대체와는 별도 계약입니다. 저장된 활성 Block을 복원할 때 이 신규 후보 필터로 다시 거부하지 않습니다.

작가: 현재 `UDialogueGraph`의 Story 카테고리에 기본 자료형이 있지만 최종 스토리보드 드롭다운은 아직 제공하지 않습니다. 기존 에셋의 새 필드는 Unassigned이며 S4 후보 필터 연결 전 개발자/작가가 의도한 소속 또는 Unrestricted를 **명시적으로** 설정해야 합니다. 자동으로 제한 없음으로 바꾸지 않습니다. 정의 에셋 선택/로드의 공통 연결점도 S4에서 연결합니다.

검증: 소속 모드·정확 일치·부모 불일치·중복 ID·소속 없는 Scene·이동 후 동일 SceneId, 이름/정렬 변경 및 정의/Graph의 메모리 직렬화 왕복을 테스트합니다. 실제 에셋 파일 편집/재열기와 UI 이동·삭제는 S5 검증 항목입니다.

## 12. 공통 계약 연결 검증

개발자용 연결 예제는 `Plugins/DialogueKit/Source/DialogueKit/Private/Tests/DialogueContractFlowTests.cpp`입니다. 실제 Block/NodeInfo의 식별자를 조건 조회 → 효과 적용 → 체크포인트 요청으로 전달하고, 변경 직후 복사한 게임 상태와 실행 ledger가 이후 live 변경의 영향을 받지 않는지 검사합니다.

`DialogueKit.Contracts.Flow.StateEffectCheckpoint`는 메모리 연결 fixture입니다. Graph 실행기·디스크 파일·FIFO 큐·실제 UI까지 완성한 예제가 아닙니다. 기존 실행기 전환 S2, 저장 조율 S3, 후보·버전 연결 S4를 거쳐 실게임에 연결합니다. 샘플 provider를 실게임 저장 시스템으로 사용하지 않습니다.

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter DialogueKit -TimeoutMinutes 5
.\Scripts\AI\Invoke-PackageValidation.ps1 -Action Package
```

전체 결과와 후속 작업은 [S1 완료 보고서](Reports/S1-2026-10-14.md)에서 확인합니다. NullRHI 테스트와 패키징 성공은 실제 화면·입력·음향 및 패키지 실행 검증을 뜻하지 않습니다.

## 13. Line 노드와 Block 실행 기반

### 작가: 발화 작성

DialogueGraph 편집기 빈 공간 우클릭 → `Line`을 선택합니다. 노드 선택 후 Details에서 화자·대사·초상화 정보를 작성하고 `Start → Line → End`를 연결합니다. 출력 핀 하나는 목적지 하나만 가지며 새 목적지 연결 시 기존 출력 연결을 교체합니다. 입력에는 여러 출력이 합류할 수 있습니다. 기존 프로토타입 Dialogue 노드는 읽기를 유지하지만 새 저작은 Line으로 수동 재작성합니다.

### 개발자: 공개 실행 API

`UDialogueBlockRunner`를 UObject 소유자의 UPROPERTY에 보관하고 `OnPresentationChanged`(Blueprint) 또는 `OnPresentationChangedNative`(C++)에 UI를 연결한 뒤 `StartBlock(Block, StateProvider, EffectProvider, Router)`를 호출합니다. S2 완료 기준으로 Start·Line·Choice·Branch·StateChange·End가 실행됩니다. `AdvanceLine()`이 다음 노드로 진행하고 `GetPresentation()`은 현재 표시 데이터를 반환합니다. `GetDirectNextType()`은 직접 연결 타입만 조회합니다. UI에 전달하는 Line은 에셋 원본이 아닌 복사본입니다.

실행 중 StartBlock 재호출은 false로 거부합니다. 잘못된 연결은 Failed와 ErrorCode를 반환하며 정상 완료나 저장으로 바꾸지 않습니다. 개발자는 Router로 외부 로그를 전송하고 반환값과 State로 복구 UI를 구현합니다. 이 실행기는 실제 게임 저장을 호출하지 않습니다.

### 검증과 제한

Editor와 테스트는 `FDialogueGraphCodec::ExportGraph/ImportGraph`를 함께 사용합니다. 변환 실패 시 기존 자료를 보존합니다. 에디터 저장/다른 이름 저장은 변환 실패를 안내합니다. 상세 경고 목록 UI는 S5에서 제공합니다. 실제 파일 저장·언로드·재로드와 표시 이벤트는 자동 테스트로 확인했고, GUI 화면·음향은 아직 검증하지 않았습니다.

## 14. 독립 Choice와 조건별 선택지

### 작가: 선택 목록과 연결

Line 출력에서 빈 공간으로 드래그하여 `Choice`를 생성하면 자동 연결됩니다. Choice의 Details → Options 배열에서 항목을 추가·삭제하고 Text·Conditions를 입력합니다. `bDefault`는 최대 한 항목만 지정합니다. 각 항목 출력 핀을 후속 Line/End 등에 연결합니다. 순서를 바꾸어도 ChoiceId와 기존 연결을 유지합니다. 입력은 여러 경로가 공유할 수 있습니다.

일반 선택지의 빈 조건은 항상 표시입니다. 조건 통과 일반 항목이 하나라도 있으면 기본 항목은 숨깁니다. 일반 항목이 전부 탈락하면 기본 항목만 표시합니다. 기본도 없으면 실제 Choice 진입 시 진단을 보내고 정상 완료합니다. Line에서 직접 다음 Choice를 표시하는 조회만으로는 종료하지 않습니다. 빈 Choice 저장 경고/차단 팝업은 S5 검증 UI에서 완성합니다.

### 개발자: 표시와 선택

`FDialoguePresentation.Choices`의 Text와 ChoiceId로 버튼을 만들고 클릭 시 `SelectChoice(ChoiceId)`를 호출합니다. Line 바로 다음이 Choice이면 Line 표시 이벤트에 목록을 함께 제공합니다. `AdvanceLine()`으로 Choice에 진입해 기다릴 수도 있습니다. `SelectChoice`는 현재 조건을 다시 검사하여 숨겨진 ID를 거부하고 변경된 목록을 다시 게시합니다. 반환 false이면 버튼 목록/상태를 재확인하십시오. 실제 조건 제공자는 S1의 `IDialogueStateProvider`입니다.

반복 질문처럼 선택 이후 원래 Line으로 돌아오는 연결은 정상 동작입니다. 새 선택마다 선택 가능 목록을 재평가합니다. 게임 효과 경계는 16절, End/Subsystem 연동은 17절을 참고합니다. 실제 체크포인트 저장은 S3입니다.

선택 승인에는 마지막 표시 목록과 현재 조건을 모두 사용합니다. 새로 통과한 숨은 ID는 목록을 먼저 갱신한 뒤 다음 입력에서 허용합니다. 실제 Choice 대기 중 모든 항목이 사라지면 정상 완료하며, 거부된 선택 함수는 false를 반환하므로 State도 함께 확인합니다.

## 15. Branch 조건 분기와 자동 순환 방어

### 작가: 조건 분기

생성 메뉴의 `Branch Node`를 추가하고 Details → DialogueKit → Branch → Conditions에서 Tag/Value/Count 조건과 All/Any 결합을 설정합니다. True/False 출력 각각을 후속 Line 등으로 연결합니다. Legacy 조건은 구형 실행기 호환 자료이며 신규 BlockRunner는 Conditions만 사용합니다.

빈 Branch는 진단을 남기고 True로 진행합니다. 조건 조회 오류도 공통 평가 규칙대로 해당 항목만 True 대체한 후 나머지 조건과 결합합니다. 제작사는 진단 Router를 이용해 오류를 수집합니다.

### 개발자: 자동 진행과 입력 반복 구분

Line의 `DirectNextType`이 Branch이면 그 타입까지만 알립니다. 미리보기에서 조건을 평가하거나 Branch 너머를 탐색하지 않습니다. `AdvanceLine()` 이후 실제 진입 때 조건을 평가합니다.

한 번의 자동 진행에서 같은 Branch에 다시 진입하면 재평가 전에 `AutomaticBranchCycle`로 중단합니다. Line/Choice에서 플레이어 입력을 기다린 뒤 다시 진행하는 경우 방문 기록은 새로 시작하므로 정상 질문 반복은 허용됩니다. 실패 시 게임 저장은 호출하지 않습니다. 극단적인 자동 작업량은 공통 실행 예산으로도 방어합니다.

## 16. 상태 변경 노드와 게임 효과 경계

### 작가: 효과 작성

생성 메뉴의 `상태 변경`(StateChange)을 연결하고 Details의 Effects 배열에 Tag/Value/Count 키와 연산·값을 작성합니다. 입력 하나·출력 하나이며 실제 진입 시 효과를 적용한 뒤 다음 노드로 자동 진행합니다. 비어 있는 효과 목록은 실행 오류입니다. Choice의 각 Options 항목에도 Effects를 작성할 수 있습니다. Choice의 `bSaveOnSelection`은 효과 없는 선택에도 저장 경계를 요청하는 옵션이며, 효과 있는 선택과 StateChange는 항상 저장 요청 경계를 제공합니다.

### 개발자: 제공자와 저장 시점

`StartBlock`에 `IDialogueEffectProvider` 구현 객체를 전달합니다. 테스트에는 `UDialogueSampleEffectProvider`를 상태·효과 제공자로 함께 전달하고 먼저 정의를 등록합니다. 실제 게임 인벤토리/퀘스트/능력치 처리는 제공자가 구현합니다. 선택·상태변경은 다음 연결을 먼저 검사하여 끊긴 연결에서 지급하지 않습니다. 제공자 실패는 Failed로 중단하고 성공 경계는 방송하지 않습니다.

`OnExecutionBoundary` 또는 `OnExecutionBoundaryNative`는 효과 성공 직후, 다음 노드 실행 전에 동기 호출됩니다. Boundary의 Context(Block·Node·ExecutionId), ChoiceId, NextNodeGuid, bHasEffects, bSaveRequested를 읽습니다. 게임은 이 시점에 변경된 게임 상태·선택/실행 기록·다음 위치를 함께 복사해 공통 FIFO 저장기에 제출해야 합니다. 이후 디스크 완료를 기다리지 않고 진행합니다. S2에는 이 이벤트까지 구현됐으며 실제 FIFO·파일 원자성·복원은 S3에서 제공합니다. 이벤트만 구독했다고 저장된 것은 아닙니다.

정상 반복은 새 ExecutionId를 받습니다. 일회성 보상 제한은 게임이 판단합니다. 재진입 호출은 거부하며 사전 다음 타입 조회에는 효과 적용이나 저장 경계 이벤트가 없습니다. 제공자 오류의 상세 진단은 Router로 전달되므로 게임의 로그 전송과 안내를 연결하십시오.

효과 없는 Choice에도 선택 기록용 경계 이벤트는 전달합니다. 이 경우 ExecutionId는 무효이며 bSaveRequested가 false이면 캐시 기록만 처리하고 디스크 저장을 요청하지 않습니다. true일 때만 불변 스냅샷을 제출합니다. 저장 접수 실패/재시도 처리는 S3의 게임 저장 조율 책임입니다.

## 17. End 완료·Subsystem 및 UI 연동

### 작가: 완료와 저장 요청

End 노드 Details의 `bSaveOnEnd` 기본값은 false입니다. 활성화하면 정상 완료 경계에 저장 요청 표시가 포함됩니다. 비활성화해도 Block은 정상 완료되며 게임이 완료/열람 캐시를 갱신할 수 있습니다. 기본 선택지까지 없는 Choice의 방어 종료도 같은 완료 경로를 사용합니다. 실패 중단은 정상 완료 이벤트를 발생시키지 않습니다. End의 구형 Quest 필드는 legacy 실행용이며 새 실행기는 게임 효과 제공자를 사용합니다.

### 개발자: GameInstance Subsystem 연결

1. `UDialogueSubsystem`의 `ConfigureBlockProviders(StateProvider, EffectProvider, Router)`로 게임 제공자를 등록합니다.
2. `GetBlockRunner()`의 `OnExecutionBoundary`에 게임의 기록/저장 조율을 연결합니다. `bBlockCompleted=true`이면 NextNodeGuid/ExecutionId는 무효이며 완료 캐시를 갱신합니다. `bSaveRequested`인 경우만 저장을 요청합니다.
3. UI는 `OnBlockPresentationChanged`에 바인딩하고 `StartDialogueBlock(LoadedGraph)`를 호출합니다. 로컬라이즈된 Line·Choices·State를 받습니다. 시작 false면 `GetBlockPresentation`의 상태/오류를 확인하고 게임이 후속 흐름을 결정합니다.
4. 발화 진행에는 `GetBlockRunner()->AdvanceLine()`, 선택에는 `SelectBlockChoice(ChoiceId)`를 사용합니다. `CanStartBlock()`은 실행/콜백 중 교체를 거부합니다. 이벤트 안에서 새 Block 시작을 바로 호출하지 말고 이벤트가 끝난 뒤 게임 흐름에서 시작합니다.

기존 `ProgressNextDialogue`, `GetCurrentDialogueNodeInfo`, `HasChoicesInCurrentDialogue`, `GetSelectableChoiceTexts`도 새 실행기 표시 데이터로 연결합니다. 기존 인덱스는 현재 표시 배열 기준이며 새 UI는 ChoiceId 사용을 권장합니다. 자동 후보 경로가 새 타입을 포함한 Graph를 받으면 새 실행기로 전달합니다. 후보 필터/비동기 로드 개편은 S4입니다.

### 기존 기능과 경계

신규 실행 중에는 legacy EndDialogue의 Quest 처리·저장과 노드별 열람 bool 변경을 호출하지 않습니다. Subsystem 자체 초기화의 기존 저장 로드는 남아 있으므로, 저장 없는 단독 검증은 UDialogueBlockRunner와 메모리 샘플 제공자를 사용합니다. 누적 열람/복원은 S3입니다.

기존 CSV 내보내기는 Line 문구와 독립 Choice의 문구를 NodeGuid·PinId 키로 내보냅니다. 변환 실패 시 이전 Graph를 새 결과로 내보내지 않습니다. StateChange·Branch·End는 번역할 문구가 없는 제어 노드이므로 CSV 대상이 아닙니다. Subsystem 표시 브리지는 기존 Localization 조회를 사용하며 Runner 단독 사용 시에는 원문을 반환합니다.

S2는 표시 자료와 Blueprint 동적 이벤트 바인딩까지 검증한 단계입니다. 기존 바이너리 샘플 Widget의 화면 배치·초상화 연출·입력·음향·단독 PIE 도구는 S6에서 완성/관찰합니다. 새 API가 Widget을 자동 생성하거나 게임 입력 모드를 변경하지는 않습니다.

## 18. 효과 상태와 실행 원장 복원

### 개발자 사용법

`UDialogueSampleEffectProvider::CaptureEffectSnapshot()`은 항목 정의, 현재 상태, 실행 요청과 결과 원장을 값 복사합니다. 효과 성공 직후 후속 변경 전에 호출하고 대화 체크포인트와 같은 게임 저장 단위에 보관합니다. 호출 자체는 파일을 저장하지 않습니다.

```cpp
const FDialogueSampleEffectSnapshot Frozen = Provider->CaptureEffectSnapshot();
FName Error;
if (!Provider->RestoreEffectSnapshot(Frozen, Error))
{
    // Error를 기록하고 게임의 복원 실패 흐름으로 처리합니다.
}
```

`RestoreEffectSnapshot`은 버전·키 중복·수치·원장 대응·성공 효과의 상태 존재를 검사한 뒤 전체 상태를 교체합니다. 실패하면 기존 상태와 원장을 유지합니다. 성공해도 효과를 재실행하지 않습니다. 같은 ExecutionId는 저장된 결과를 반환하며 새 ExecutionId의 정상 반복은 허용합니다.

### 확장과 제한

실제 게임은 `IDialogueEffectProvider` 구현에서 게임 상태와 실행 원장을 함께 캡처·복원해야 합니다. 이 샘플은 인벤토리 원본을 대신하지 않습니다. 공개 API는 게임 스레드에서 사용합니다. 기존 메모리 샘플 API를 유지하며 별도 SchemaVersion 1 자료형을 추가했습니다.

이 자료형의 현재 검증은 메모리 직렬화 왕복입니다. 디스크 파일·비동기 FIFO·대화 위치 합본 복원은 S3 후속 항목입니다. 커스텀 `ArIsSaveGame` 필터를 사용하는 게임은 중첩 필드 직렬화 정책을 별도로 구성해야 합니다. 에디터 작가용 UI는 추가하지 않습니다.

`DialogueKit.Checkpoints.EffectSnapshot`은 스냅샷 독립성, 직렬화 후 중복 지급 방지, 정상 반복, 손상 자료 거부와 실패 무변경을 검사합니다.

## 19. 게임 소유 FIFO와 비동기 합본 저장 샘플

### 게임 개발자 연동

게임 공통 저장 조율자가 `UDialogueSaveQueue`를 소유합니다. `InitializeQueue(NewGuid)`로 세대를 만들고, 게임 상태·실행 원장·대화 스냅샷을 같은 시점에 캡처한 뒤 `Enqueue(Request, GamePayload)`를 호출합니다. 큐는 두 자료를 값 복사합니다. Request의 Sequence는 세대 안에서 증가시켜야 하며 RequestId와 CheckpointId를 재사용하지 않습니다.

C++에서는 `OnWriteRequestedNative.Bind...`, Blueprint에서는 `OnWriteRequested` 중 한 경로에 **저장기 하나만** 연결합니다. 요청을 받은 저장기는 합본을 비동기로 기록하고 `CompleteWrite(Completion)`에 원래 RequestId·QueueGenerationId·Sequence와 성공 여부를 전달합니다. 성공은 ErrorCode 없음, 실패는 오류 코드를 요구합니다. 두 경로 연결 또는 연결 누락은 자료를 보존한 실패로 처리합니다.

실패 head가 있으면 후속 항목은 대기합니다. `RetryFailedWrite()`는 같은 자료·순서·CheckpointId를 유지하고 RequestId만 새로 발급합니다. 저장 완료 이벤트 안에서 즉시 재시도하지 말고 이벤트 반환 후 게임의 재시도 흐름에서 호출합니다. 성공은 해당 head만 제거합니다. 정상 반복 선택은 새 논리 스냅샷으로 요청합니다.

`OnSaveStateChanged(bWriting, bFailed)`는 저장 이미지/실패 안내용입니다. 대화 진행을 기다리게 하지 않습니다. `OnWriteCompleted`로 성공·실패를 처리하며 `DiagnosticRouter`를 게임의 Sentry 등 전송 경계에 연결할 수 있습니다. 종료 요청은 `GetPendingCount()==0`을 확인하고, 실패 시 재시도·종료 취소·미저장 종료를 게임이 구분합니다. 큐 재초기화는 대기 항목이 없을 때 새 세대로만 가능합니다. 세대별 중복 ID 집합은 해당 세대가 끝날 때 정리합니다.

### 플러그인 단독 샘플

`UDialogueSampleSaveCoordinator`와 `UDialogueSampleEffectProvider`를 GameInstance 등에서 보유합니다.

1. `Initialize("MyDialogueSample_", Provider, Error)`를 호출합니다. 이 이름의 내부 슬롯 두 개는 샘플 전용으로 사용합니다.
2. `GetSaveQueue()->GetGeneration()`을 Request의 세대로 사용합니다.
3. 효과 성공 직후 `IDialogueCheckpointProvider::Execute_RequestDialogueCheckpoint(Coordinator, Request)`를 호출합니다. 샘플은 상태/원장을 즉시 복사하고 큐에 접수합니다.
4. 대화는 계속 진행합니다. 실패하면 큐의 `RetryFailedWrite()`를 게임 UI/정책에서 호출합니다.
5. 큐가 비어 있을 때 `RestoreLatest(OutSnapshot, Error)`로 게임 상태를 복원하고, 반환된 대화 스냅샷을 대화 재개 경로에 전달합니다. 현재 기능만으로 Runner 위치를 자동 복원하지는 않습니다.
6. 저장 종료 시 먼저 큐를 비우고 `Shutdown()`을 호출합니다. 같은 슬롯 이름을 두 조율자가 동시에 소유할 수 없습니다.

샘플은 UE `AsyncSaveGameToSlot`을 사용합니다. CRC로 감싼 합본을 두 내부 슬롯에 번갈아 기록하고, 온전한 최신 Revision 하나만 읽습니다. 새 슬롯이 손상되면 직전 온전한 저장을 사용합니다. 사용자용 Multi Save/Load는 제공하지 않습니다. CRC는 파일 편집 방지 인증이 아니며 전원 손실·하드웨어 오류의 완전한 복구를 보장하지 않습니다. 디스크 완료 전 강제 종료하면 미저장 결과를 잃을 수 있습니다.

실제 게임은 샘플을 게임 전체 저장과 병행하지 말고 공통 조율자에 계약을 통합해야 합니다. 샘플의 GamePayload는 게임별 공개 저장 자료로 교체할 수 있습니다. 큐/조율자는 게임 스레드에서 사용합니다. 시나리오 작가용 UI 추가는 없습니다.

### 검증 범위

`DialogueKit.Checkpoints.ImmutableFIFO`는 불변 자료·실패 후 대기·재시도·오래된 완료·즉시 완료를 검사합니다. `AsyncCombinedFile`은 고유 슬롯의 실제 비동기 저장, 새 객체 복원, 미저장 상태 배제, 원장 재전달, 손상된 다른 슬롯에서의 이전 저장 보존을 검사합니다. 프로세스 강제 종료·재실행과 Runner 재개 연결은 별도 S3 검증 대상입니다.

## 20. Block 로드 전 대상과 조건 필터

### 작가 설정

Block 에셋의 Details에서 `DialogueKit > Selection`의 `Dialogue Target`에 대상 GameplayTag 하나를 설정합니다. NPC 대화는 NPC 태그, 장소 대화는 MAP 태그처럼 게임이 등록한 태그를 사용합니다. 화자 설정과는 별개이며 하위 태그를 자동 포함하지 않습니다.

`Story Scope`에서 Chapter/Scene 소속 또는 `Unrestricted`를 설정합니다. `Unassigned`는 신규 후보에서 제외됩니다. `Entry Conditions`에는 기존 Tag/Value/Count 조건을 등록합니다. `Default Block`과 `Top Priority Block`은 동시에 사용할 수 없습니다. 숫자 우선순위는 기존 `Dialogue Priority Weight` 필드를 사용합니다.

`Availability=RestoreOnly`는 신규 후보에서 제외하기 위한 표시입니다. 기존 버전의 실제 저장 복원 연결은 후속 S4 작업입니다. 기존 에셋의 `Required All Tags`, `Required Any Tags`, `Blocked Any Tags`가 남아 있으면 신규 후보 자료는 유효하지 않은 것으로 취급합니다. 프로토타입 에셋은 확정된 방침에 따라 수동 재작성하고 새 Entry Conditions의 의미를 확인한 뒤 기존 조건을 정리합니다. 자동 조건 변환은 하지 않습니다.

태그 루트별 선택 UI·스토리보드 관리 UI는 S5 대상이며 이 항목에서는 기본 Details 필드를 제공합니다.

### 개발자 사용법과 확장

`UDialogueGraph::GetSelectionMetadata()`는 에셋 원본에서 메타데이터를 만듭니다. 저장/cook 시 `GetAssetRegistryTags()`가 `DialogueKit.MetadataV1`과 `DialogueKit.Target`을 기록합니다. `FDialogueBlockMetadata::ReadAsset(FAssetData, OutMetadata)`는 Registry만 읽고 `GetAsset()`/`TryLoad()`를 호출하지 않습니다.

읽은 자료는 `UDialogueMetadataLibrary::IsEligible(Metadata, Target, StoryDefinition, StoryProgress, StateProvider, Router)`로 검사합니다. 게임이 현재 진행도와 상태 제공자를 전달합니다. 대상 정확 일치·신규 노출 허용·소속을 확인한 뒤 기존 조건 평가 계약을 적용합니다. 상태 조회 오류의 True 대체와 진단은 기존 공통 계약을 따릅니다. 이 API는 신규 시작/Refresh 전용이며 활성 체크포인트 복원에 적용하지 않습니다.

게임별 아이템/능력치/태그 이름과 현재 값은 Provider에서 확장합니다. Registry에는 조건 정의만 들어 있고 플레이어 현재 값은 들어 있지 않습니다. `int64` 수량은 UE 구조체 텍스트로 보존합니다. 메타데이터 읽기 실패 시 출력값은 바꾸지 않으며 실패를 무조건 적격으로 취급하지 않습니다.

### 검증

`DialogueKit.Selection.Metadata`는 값 왕복·대상 정확 일치·신규 노출 중단·소속 없음·기존 조건 재작성 검사를, `UnloadedRegistry`는 Saved 아래 고유 패키지의 실제 저장·언로드 후 Registry 조회와 무로딩 평가를 검사합니다.

패키징 후 별도로 `Invoke-UE.ps1 -Action Test -TestFilter ProjectCook.DialogueRegistry`를 실행하면 실제 cooked Registry의 메타데이터와 PrimaryAssetId 보존을 확인합니다. 기존 샘플의 미설정 Block ID/대상 조건이 올바른 신규 콘텐츠라는 뜻은 아닙니다. 후보 추첨·게임 실행 연결과 구버전 복원 검증은 별도 항목입니다.
