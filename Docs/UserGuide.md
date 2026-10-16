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

- [21. 우선순위 선정과 대화 세션 순회](#21-우선순위-선정과-대화-세션-순회)

- [22. 최신 요청 기준 비동기 Block 로드](#22-최신-요청-기준-비동기-block-로드)

- [23. 구조체 기본값과 초기화 오류 검사](#23-구조체-기본값과-초기화-오류-검사)

- [24. 누적 열람과 실행별 저장 증거](#24-누적-열람과-실행별-저장-증거)
- [25. 게임 저장 세션과 실행 재개](#25-게임-저장-세션과-실행-재개)
- [26. 정확 콘텐츠 버전 복원과 버전 작성](#26-정확-콘텐츠-버전-복원과-버전-작성)

- [27. 명시적 버전 교체와 게임 보정](#27-명시적-버전-교체와-게임-보정)

- [28. 실제 재시작과 버전 cook 검증](#28-실제-재시작과-버전-cook-검증)

- [29. 노드 색상과 우클릭 삭제](#29-노드-색상과-우클릭-삭제)

- [30. 연속 화자와 Line 저작 설정](#30-연속-화자와-line-저작-설정)

- [31. 경유점과 영역 주석](#31-경유점과-영역-주석)

- [32. 선택 노드 복제](#32-선택-노드-복제)

- [33. 현재 Block 검색](#33-현재-block-검색)

- [34. 그래프 검사와 저장 방어](#34-그래프-검사와-저장-방어)

- [35. 스토리보드와 Block 소속·대상 편집](#35-스토리보드와-block-소속대상-편집)

- [36. 신규 조건과 구형 조건 구분](#36-신규-조건과-구형-조건-구분)

- [37. 텍스트 표시 상태와 안전한 입력](#37-텍스트-표시-상태와-안전한-입력)
- [38. 기본 대화 화면과 이미지·음향](#38-기본-대화-화면과-이미지음향)
- [39. 비동기 저장 표시](#39-비동기-저장-표시)
- [40. Block 테스트의 게임 상태 격리](#40-block-테스트의-게임-상태-격리)
- [41. 현재 Block 단독 PIE 테스트](#41-현재-block-단독-pie-테스트)


- [42. 안정 선택 ID와 하위 폴더 지역화](#42-안정-선택-id와-하위-폴더-지역화)

- [43. 샘플 게임 호스트로 선정과 저장 연결](#43-샘플-게임-호스트로-선정과-저장-연결)

- [44. 실제 패키지 저장과 강제 종료 복원 검사](#44-실제-패키지-저장과-강제-종료-복원-검사)
- [45. 초상화 없는 기존 대화와 Shipping 샘플](#45-초상화-없는-기존-대화와-shipping-샘플)

- [46. Line 노드 내용 표시](#46-line-노드-내용-표시)

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

## 21. 우선순위 선정과 대화 세션 순회

게임은 `UDialogueCandidateSelector::BeginSession(Target)`으로 장소/NPC 대화 세션을 시작하고, 명시적인 시작/Refresh 요청에서만 `Select(Candidates, Story, Progress, Provider, Router)`를 호출합니다. `FDialogueBlockCandidate`는 SoftObjectPath와 Registry 메타데이터를 담습니다. 함수는 에셋을 로드하지 않습니다.

선정 순서는 적격 최우선 Block → 미완료 일반 Block의 최고 우선순위 → 기본 대화입니다. 최우선/일반의 같은 우선순위는 균등 추첨합니다. 실제 실행/표시에 성공하면 `RecordExposed(BlockReference)`, 정상 End에서만 `RecordCompleted(BlockReference)`를 호출합니다. 로딩 취소·실행 실패는 완료로 기록하지 않습니다.

일반 대화는 세션 내 정상 완료 목록과 직전 노출을 제외합니다. 모든 일반 후보가 소진되면 기본 대화로 넘어갑니다. 기본 대화는 현재 적격 목록을 한 번씩 완료하면 새 순회를 시작하며, 다른 후보가 있으면 직전 기본 대화의 연속 노출을 피합니다. 최우선 대화는 완료해도 조건이 남으면 다시 후보가 됩니다. 일회성 제한은 게임이 저장하는 조건으로 처리합니다.

해당 장소/NPC와의 대화 상황이 끝날 때 `EndSession()`을 호출합니다. 일반/기본 소진과 직전 노출만 초기화하며 누적 열람이나 게임 저장을 변경하지 않습니다. 같은 대상의 `BeginSession`은 현재 세션을 유지하고 다른 대상은 새 세션으로 전환합니다. 테스트/게임 난수 제어가 필요하면 `SetRandomSeed`를 사용합니다.

결과의 `bSelected`와 `ErrorCode`를 확인합니다. 같은 대상에서 활성 Block ID가 중복되면 `InvalidOrAmbiguousBlockCatalog`, 적격 후보가 없으면 `NoEligibleDialogueBlock`입니다. 공통 인사 로드와 실제 실행 연결은 로더가 담당하는 후속 기능입니다. 전역 버전 카탈로그 검증은 별도 S4 복원 항목입니다.

시나리오 작가는 20절의 기본·최우선·우선순위·진입 조건을 설정합니다. 세션 완료 여부는 에셋에 저장하지 않습니다. `DialogueKit.Selection.PriorityAndSession`은 일반 소진, 하위 우선순위, 기본 순회, 최우선 반복/동률, 중단 후 직전 제외, 세션 초기화를 검증합니다.

## 22. 최신 요청 기준 비동기 Block 로드

### 게임 개발자 호출 순서

`UDialogueBlockLoader`를 게임 소유 UObject의 `UPROPERTY`로 보유합니다. 게임 스레드에서 사용하며, 기존 `BeginDialogue(Speaker)` 경로를 자동 교체하지 않습니다. 새 대상 기반 경로에서는 다음과 같이 연결합니다.

1. 호출 전에 `OnLoadCompleted`(Blueprint) 또는 `OnLoadCompletedNative`(C++)에 완료 처리를 연결합니다. 즉시 완료도 가능하므로 호출 반환 후 이벤트를 연결하지 않습니다.
2. 게임이 정한 대화 시작 또는 Refresh 시점에 `RequestFromRegistry(Target, StoryDefinition, CurrentProgress, StateProvider, DiagnosticRouter)`를 호출합니다. 반환값은 요청 식별자이며 성공 여부가 아닙니다.
3. 결과 이벤트의 `IsCurrentRequest(Result.RequestId)`와 `Result.bLoaded`를 검사합니다. 실패 시 `ErrorCode`로 게임이 후속 흐름을 결정합니다. 예를 들어 `MissingCommonGreetingBlock`이면 콘텐츠 설정을 수정해야 합니다. 플러그인은 게임용 팝업을 강제로 띄우지 않습니다.
4. 성공한 `Result.Block`을 `UDialogueSubsystem::StartDialogueBlock`에 전달합니다. 실행 전 `ConfigureBlockProviders`로 게임 제공자를 연결해야 합니다. 실행/표시가 성공하면 `RecordBlockExposed(Result.RequestId, BlockReference)`를 호출합니다.
5. 해당 실행의 정상 End에서만 `RecordBlockCompleted(원래 RequestId, BlockReference)`를 호출합니다. 중단·선정·로드만으로 완료 처리하지 않습니다. 같은 Block이라도 이전 요청 ID의 완료는 거부합니다.
6. 대화 상황을 종료할 때 게임의 실행/UI 종료와 함께 `EndSession()`을 호출합니다. 로더의 세션 종료는 이미 실행 중인 Runner나 게임 UI를 대신 종료하지 않습니다.

로드 중 새 요청은 이전 요청을 무효화하고 자신이 소유한 로드 핸들만 취소합니다. 같은 에셋을 다른 시스템이 사용하더라도 전역 unload하지 않습니다. `CancelPending()`은 로드/선정만 취소하고 세션 완료 이력은 유지합니다. 조건은 요청 순간 평가하며 로드 완료 때 다시 평가하지 않습니다. 게임은 연타·이동·대화 입력 제한을 자체 정책으로 제어해야 합니다.

이벤트 처리 중 새 요청을 시작할 수 있습니다. 이전 결과로 UI/실행을 변경하기 직전에 `IsCurrentRequest`를 검사하십시오. Native 구독자가 새 요청을 시작한 경우 이전 결과의 Blueprint 이벤트는 생략됩니다. 게임 연동은 한 이벤트 경로를 정본으로 사용하고, 여러 Native 구독자를 사용하면 각 구독자가 요청 ID를 검사해야 합니다.

### 공통 인사 에셋 설정

에디터의 `Project Settings → DialogueKit Settings → Fallback → Common Greeting Block` 에셋 선택기에서 개발자가 만든 공통 인사 Block을 지정합니다. 설정 검색에서 `Common Greeting Block`으로 찾을 수도 있습니다. 새 로더는 이 설정을 기본값으로 사용하며, 게임이 로더 인스턴스의 `CommonGreetingBlock`을 명시적으로 교체할 수 있습니다. 설정 변경 후 이미 생성된 로더에는 다시 지정해야 합니다.

적격 일반/기본 후보가 없을 때만 공통 인사를 시도합니다. 공통 인사는 정상 Block ID를 가진 `Active` 에셋이어야 하며, 일반 후보의 대상/진입 조건을 다시 적용하지 않는 최종 fallback입니다. 기본 대화 누락 진단을 제작사 로그 전송에 연결하고 출시 전 수정하십시오. 공통 인사 파일을 자동 생성하지 않습니다. 프로젝트 AssetManager/cook 설정에 해당 콘텐츠가 포함되어야 합니다.

### 확장과 검증 범위

게임의 별도 카탈로그는 `RequestCandidates`에 같은 메타데이터를 제공합니다. C++ 커스텀 스트리밍은 `LoadOverride`와 `CompleteRequest(RequestId, Block)`을 사용합니다. 예상 에셋 경로와 Block ID·버전이 일치해야 합니다. 에셋 이름/경로 변경 후 오래된 색인은 재생성해야 합니다. 오류·성공 반환에 따른 UI/재시도 정책은 게임 책임입니다.

`DialogueKit.Loading`은 이전 완료 무시, 취소, 최신 실패, 즉시 완료, 요청 시점 조건 고정, 공통 인사, 조건 진단 재진입을 검사합니다. `DialogueKit.Selection.UnloadedRegistry`는 임시 에셋을 실제 언로드한 뒤 Registry 평가와 비동기 로드, 같은 에셋의 다른 핸들 취소, 이미 로드된 에셋 재요청을 검사합니다. 저장 위치 복원·구버전 매핑·스토리 초기화와 Subsystem의 자동 세션 연결은 아직 후속 S3/S4 항목입니다.

## 23. 구조체 기본값과 초기화 오류 검사

새로 생성한 `FPortraitInitData`의 기본값은 Speaker=`TestNPC`, EmoteType=`None`(Default), PortraitSide=`Left`입니다. `FPortraitActionData.ActionTargetSpeakerID`는 `TestNPC`, `FItemRow.ItemType`은 `Consumable`, `Value`는 `0`입니다. 기존 enum의 0 값 의미를 유지하고 명시적 C++ 초기값을 추가했습니다.

작가는 기존 초상화/연출 Details 및 아이템 DataTable에서 필요한 화자·표정·위치·종류·수치를 지정합니다. 이미 에셋에 저장된 값을 일괄 덮어쓰거나 enum 순서를 변경하지 않습니다. 게임 개발자는 구조체 생성 후 필요한 필드를 대입할 수 있으며 별도 초기화 함수를 호출할 필요가 없습니다. `TestNPC`는 미지정 전용 값이 아니므로 실제 화자를 지정하십시오.

`Invoke-UE.ps1`은 기존 6개 필드의 초기화 진단을 더 이상 허용 목록으로 분리하지 않습니다. 같은 오류가 재발하면 테스트 실패로 처리합니다. 테스트 결과의 `StartupClean`, `LogErrorLines`, `UnexpectedLogErrors`를 확인하십시오. 이 변경은 체크포인트 스키마나 Block 교체 정책을 변경하지 않습니다.

## 24. 누적 열람과 실행별 저장 증거

`UDialogueCheckpointState`는 게임이 `UPROPERTY`로 보유하는 대화 캐시입니다. 일반 게임 연동은 아래 25절의 `UDialogueCheckpointSession`을 사용합니다. 상태기를 직접 연결할 때는 `PrepareBlock` → `BeginBlock`(Runner의 실행 ID) → `RunPreparedBlock` 순서를 지켜야 합니다. `StartBlock` 반환 후 초기화하면 첫 자동 효과를 놓칩니다. 아래는 상태기의 낮은 수준 호출 계약이며 세션을 사용하면 자동 처리됩니다.

1. 실행 시작 시 `BeginBlock(BlockReference, TargetTag, BlockRunId)`를 호출합니다. Runner 경계의 `Context.BlockRunId`와 같은 ID여야 합니다.
2. 실제 Line 표시 시 `VisitLine(NodeGuid, ReadVersion)`을 호출합니다. 표시 중 커서는 갱신하지만 누적 열람과 저장 재개점은 아직 변경하지 않습니다.
3. 다음 흐름으로 진행할 때, 후속 효과 실행 **전에** `PassLine(NodeGuid)`를 호출합니다. 텍스트 즉시 출력·보이스 중단만으로 호출하지 않습니다. 같은 Line 통과는 중복 저장하지 않습니다.
4. 실제 효과 성공/선택/정상 완료 경계에서 `ApplyBoundary`를 호출합니다. `bSaveRequested`인 경우 반환 Snapshot과 같은 시점의 게임 상태·효과 원장을 한 합본으로 FIFO에 제출합니다. 반환 성공만으로 디스크 저장이 끝난 것은 아닙니다. End 저장이 꺼져 있으면 완료·열람을 캐시에만 반영합니다.
5. 일반 게임 저장은 `CaptureGameSave`를 사용합니다. 마지막 저장 경계가 없다면 Block 처음부터 재개하며, 선택/효과 경계가 있다면 해당 후속 위치를 유지합니다. 실제 디스크 성공 이후 `AcknowledgeSavedSnapshot`에 성공한 자료를 전달합니다. 이전 End 저장 완료가 늦게 도착해도 이후 선택은 지우지 않습니다.
6. 게임 상태와 합본으로 읽어온 자료만 `RestoreSnapshot`에 전달합니다. 실패는 기존 캐시를 유지합니다. `ResetStory(false)`는 누적 읽음을 보존하고 활성 실행을 초기화하며, 완전 새 게임은 `true`로 전체 읽음을 초기화합니다.

새 캐시는 스키마 2를 생성합니다. 스키마 1은 `UpgradeToLatest`로 알려진 마지막 선택/효과만 이전하며 `RunEvidence.bComplete=false`로 유지합니다. 빈 배열을 과거 효과가 없었다는 증거로 해석해서는 안 됩니다. 선택적 롤백 소비 기능은 아직 미구현이며, 불완전 증거를 근거로 지급 취소를 허용하지 않습니다. 알 수 없는 스키마는 거부합니다.

누적 열람은 Block ID별 Line ID와 열람 버전, 완료 여부를 저장합니다. 현재 실행의 `PassedLines`는 순서·반복 횟수 기록이 아닙니다. `Effects`는 성공한 ExecutionId, 실제 발생 Node, Choice ID와 관련 Line을 보관합니다. Line 없이 실행한 StateChange는 관련 Line이 비어 있을 수 있습니다. 실제 지급 여부는 효과 원장과 함께 확인합니다.

샘플 합본 저장은 완전한 실행 증거의 Block·Node·BlockRunId·ChoiceId·ExecutionId를 효과 원장과 대조합니다. 다른 게임의 제공자도 같은 관계를 검증해야 합니다. 같은 실행 ID를 다른 실행/선택의 요청에 재사용하면 안 됩니다. 정상적인 반복 선택에는 새 실행 ID를 사용합니다. 현재 자동화 테스트에는 이전 저장 이전, End 임시 기록 정리, 중복 통과/효과, 다른 실행 원장 거부를 추가했습니다. 해상도 복구 후 전체 DialogueKit 자동화 37개가 통과했고 오류 로그는 0건입니다(`Saved/AI/20261015_142642_c7d18338`). 이 검증은 독립 상태기와 기존 기능 범위이며 아직 구현되지 않은 자동 실행/저장 연결의 검증을 뜻하지 않습니다.

## 25. 게임 저장 세션과 실행 재개

게임은 저장 제공자를 생성·보유하고 `UDialogueCheckpointSession::Initialize(SaveProvider, StateProvider, EffectProvider, Router, ExistingRunner)`를 호출합니다. ExistingRunner가 없으면 세션이 생성합니다. 게임의 공통 큐가 `UDialogueSaveQueue`라면 시작 전에 `AttachSaveQueue`를 한 번 호출합니다. 이후 다른 큐로 교체하지 않습니다. `GetRunner()`의 표시 이벤트와 `AdvanceLine`/`SelectChoice`를 UI에 연결합니다.

- 신규 실행: `StartBlock(Block, TargetTag)`. 준비 단계에서 실행 ID와 캐시를 먼저 만든 뒤 첫 노드를 실행합니다. 후보 조건 검사는 앞선 로더 책임입니다.
- 기존 Subsystem UI: `ConfigureBlockProviders` → `ConfigureCheckpointSession(SaveProvider, SaveQueue)` → `StartDialogueBlock`. 저장 세션 생성 후 제공자 객체 교체는 거부합니다. 기존 객체의 게임 상태 값은 게임이 계속 갱신합니다.
- 일반 저장: `SaveCurrentState()`. 게임 상태도 같은 제공자가 캡처합니다. 성공 반환은 메모리 접수이며 파일 성공이 아닙니다.
- 복원: 게임 제공자가 최신 합본의 게임 상태를 복원하고 같은 Snapshot을 반환 → 정확 ID/버전의 Block 확보 → `ResumeBlock(Block, Snapshot)` 또는 Subsystem `ResumeDialogueBlock`. 게임 조건으로 다시 후보를 고르지 않습니다. 저장된 실행 ID와 후속 노드에서 재개하며 이전 지급은 실행하지 않습니다. 완료 Snapshot은 완료 상태만 표시합니다.
- 사망/스토리 초기화: `ResetStory(false)`. 새 게임은 `true`. 게임 상태도 초기화한 뒤 `SaveCurrentState`로 같은 합본에 저장합니다. 로더의 이전 요청/세션 종료는 S4 연동을 사용합니다. 초기화 저장 전 종료는 마지막 성공 저장으로 복원됩니다.

`IDialogueCheckpointProvider` 구현은 `CanCaptureDialogueCheckpoint`와 `CaptureDialogueCheckpoint(Snapshot)`을 제공합니다. 미구현 기본값은 실패입니다. Capture에서는 게임 공통 순서/세대/요청 ID를 발급하고 최초 게임 상태와 Snapshot을 불변 복사합니다. `UDialogueSampleSaveCoordinator`는 이 구현 예제입니다. 실제 게임은 모든 시스템 저장을 동일 조율자로 보내야 합니다.

메모리 접수 실패 시 후속 노드 실행을 중단하고 `HasPendingCapture`와 오류값을 반환합니다. 제공자는 최초 게임 복사본을 유지하고 다른 저장의 추월을 막아야 합니다. 세션의 `RetryCapture`는 같은 자료만 재접수하고 성공하면 같은 실행의 후속 노드에서 이어갑니다. 자동 전체 상태 롤백은 하지 않습니다. 캡처 자체에 실패해 자료가 없는 게임 구현은 안전한 재시도를 제공할 수 없으므로 오류 처리에서 게임 진행을 중단해야 합니다.

샘플은 구조화 게임 복사본과 Queue의 캡처 예약으로 순서를 보장합니다. 직접 `RequestDialogueCheckpoint`를 사용하면 유효한 요청을 전달해야 하며, 실패한 최초 요청을 버리고 새 CheckpointId를 제출하지 않습니다. 파일 쓰기 실패는 큐의 실패 head에 보존되므로 `RetryFailedWrite()`를 사용합니다. 이 경우 대화는 이미 접수된 자료를 유지하며 계속 진행할 수 있습니다. 저장 이미지/실패 안내는 `OnSaveStateChanged`에 연결합니다.

`OnExecutionBoundary`는 단일 세션 기록 이후의 관찰 이벤트입니다. 여기서 같은 저장을 다시 제출하지 않습니다. 접수 실패한 경계는 방송되지 않으며 재시도도 원 경계를 재방송하지 않습니다. 복구 UI는 `RetryCapture` 결과와 표시 이벤트를 사용합니다. 저장 성공 정리는 Queue의 실제 성공 Snapshot에만 적용됩니다. 게임 자체 큐라면 `AcknowledgeSaved`를 성공 시 호출합니다.

작가는 기존 Choice의 SaveOnSelection과 End의 SaveOnEnd를 사용합니다. 효과가 있으면 저장 경계가 필수이며, End 저장을 끄면 완료/열람은 캐시에만 남습니다. 이 절은 실행/저장 연동이며 별도 에디터 화면은 추가하지 않습니다.

## 26. 정확 콘텐츠 버전 복원과 버전 작성

게임은 최신 합본 Snapshot의 `Resume.Block`을 `UDialogueBlockLoader::RequestExactReference`에 전달합니다. 호출 전 완료 이벤트를 연결하고 결과의 요청 ID와 `bLoaded`를 확인한 뒤, 동일 Snapshot으로 CheckpointSession 또는 Subsystem의 Resume 함수를 호출합니다. 현재 GameplayTag·레벨·아이템 조건은 다시 검사하지 않습니다. 소속 없음/RestoreOnly 상태여도 정확 참조를 복원할 수 있으며 내부 Branch·Choice는 실행 시 정상 평가합니다.

`FindExactReference`는 로드 없이 Registry에서 ID와 콘텐츠 버전을 찾는 C++ 확장점입니다. 다른 버전을 대신 선택하지 않습니다. 누락은 `RestoreBlockNotFound`, 중복은 `AmbiguousRestoreReference`로 반환합니다. 실패 시 일반 후보나 공통 인사로 대체하지 않습니다. 게임이 별도 카탈로그 또는 명시적 매핑에서 경로를 확보했다면 `RequestExactPath(SoftBlock, ExactReference, Router)`를 사용합니다. 완료 객체의 경로와 정확 참조도 대조합니다. 복원 요청은 신규 후보 소진으로 기록하지 않습니다.

출시한 Graph를 구조적으로 바꿀 때는 Editor 모듈의 `UDialogueVersionLibrary::CreateContentVersion(Source, PackageName, AssetName, NewVersion, OutError)`로 별도 에셋을 만듭니다. 예: `/Game/Dialogue/QuestA_V2`, `QuestA_V2`, 버전 2. 동일 Block ID와 Node ID를 보존한 복제본을 만들고 콘텐츠 버전만 올립니다. 기존 목적지나 중복 버전에는 쓰지 않습니다. 저장은 자동 수행하지 않습니다. 이 함수는 Editor Blueprint/C++ 도구에서 호출할 수 있으며 Content Browser 전용 메뉴는 아직 추가하지 않았습니다.

작가는 새 버전을 편집·저장하고, 구 에셋 Details의 Availability를 RestoreOnly로 설정·저장합니다. 원본은 함수가 자동 변경하지 않습니다. 일반 에셋 Duplicate는 기존대로 새 Block ID를 만드는 기능입니다. 새 버전 작성과 혼동하지 마십시오. 두 버전이 모두 Active이면 신규 후보 선정은 모호한 동일 ID로 거부할 수 있으므로 구 버전을 명시적으로 신규 노출 중단해야 합니다.

지원하는 구버전과 이미지/음향 의존 리소스를 실제 cook에 포함해야 합니다. 프로젝트 AssetManager의 해당 DialogueGraph scan 경로와 AlwaysCook 설정 또는 프로젝트의 명시적 cook 포함 정책을 사용하십시오. Git 보관만으로는 실행 중 복원되지 않습니다. 자동 테스트는 버전별 조회, RestoreOnly 로드, 소속/대상 없는 복원, 중복·누락·목적지 충돌을 검사합니다. 명시적 대체/게임 보정은 다음 기능입니다.

## 27. 명시적 버전 교체와 게임 보정

### 작가와 개발자의 교체 설정

Content Browser → Miscellaneous → Data Asset → `DialogueReplacementMap`을 생성하고 Details의 `Rules`에 항목을 추가한다. 신규 항목의 RuleId는 Details 편집 시 생성된다. C++에서 작성할 때는 `FGuid::NewGuid()`로 명시한다. 원본 `SourceBlockId`, 양 끝 포함 `FirstVersion`/`LastVersion`, 파일 선택기로 `Target`, 그 에셋의 `TargetReference`를 지정한다. 출시된 규칙의 의미를 바꾸면 새 RuleId를 사용한다. 중복 ID/범위 중첩/교체 순환은 오류다. 검증은 공개 `Validate(OutError)`를 사용한다.

| Mode | 처리 |
|---|---|
| None | 게임 보정 없이 대체 Block 처음부터 시작 |
| GameCorrection | 게임 제공자의 `GameRule` 보정 성공 후 처음부터 시작 |
| SelectiveRollback | 완전한 현재 실행 증거와 정확 원본 Graph를 확인하고 지정 효과만 게임에 전달 |

SelectiveRollback의 `RequiredPassedLine`은 선택 사항이다. 지정하면 그 Line을 이번 실행에서 실제 통과했을 때만 보정한다. `EffectNodeIds`에는 회수할 효과가 있는 Choice/StateChange의 ID를 지정한다. ID가 잘못됐으면 미통과로 간주하지 않고 실패한다. 원본 Graph를 보존·cook해야 한다. 누적 열람과 구 스키마의 불완전 증거만으로는 회수하지 않는다. 일반 게임은 지급 아이템 소비·태그 중복 출처·상태 덮어쓰기 처리 정책을 직접 구현한다.

### 게임의 공개 연결 순서

1. 게임 상태와 대화 Snapshot을 같은 최신 합본에서 복원한다. 샘플은 `UDialogueSampleSaveCoordinator::RestoreLatest`를 사용한다.
2. `UDialogueCheckpointSession::Initialize`와 `AttachSaveQueue`로 동일 게임 저장 조율자를 연결한다.
3. 게임이 보유할 `UDialogueRestoreController`를 생성하고 `Initialize(Session, CorrectionProvider, Router, CandidateLoader)`를 호출한다. 마지막 인자는 게임이 사용하는 신규 후보 로더이며 선택 사항이다.
4. `Restore(Snapshot, ReplacementMap)`을 호출한다. 규칙이 없으면 현재 GameplayTags를 재평가하지 않고 정확 BlockId/버전으로 복원한다. 명시한 범위만 교체한다. 완료/초기화 저장은 과거 Block을 실행하지 않는다.
5. `OnRestored`의 성공/오류 코드를 받아 게임이 UI·재시도·로그 전송을 결정한다. 반환 true는 요청 대기 또는 동기 성공이며, false는 접수/동기 복원 실패다. 최종 결과는 이벤트로 확인한다. 저장 디스크 완료는 별도 FIFO 이벤트다.

이미 메모리에 확보한 에셋은 `Session->ReplaceLoadedBlock(Target, Source, Rule, SourceGraph, CorrectionProvider)`로 연결할 수도 있다. 낮은 수준 API는 게임이 Source와 같은 게임 상태를 먼저 복원했음을 전제로 한다. 저장한 원본이 사라지고 명시 규칙도 없으면 오류를 반환하며 임의 대화로 보내지 않는다.

### 보정과 저장의 경계

`IDialogueCorrectionProvider::CorrectDialogueState`를 C++/Blueprint로 구현한다. 요청은 규칙 ID에서 계산한 CorrectionId, 규칙 fingerprint, 원본 Snapshot, 이번 실행의 실제 성공 Effects를 제공한다. 게임은 원장과 대조하여 **다른 경로의 변경을 보존하는 전체 성공 또는 실패 무변경** 트랜잭션으로 처리한다. 같은 ID/내용 재전달은 AlreadyApplied, 같은 ID/다른 내용은 오류여야 한다. 결과·보정 영수증·대화 자료를 동일 합본에 저장한다.

보정 성공 후 대체 BlockStart 스냅샷을 **첫 노드 실행 전** 캡처한다. 메모리 접수 성공 후에는 비동기 파일 저장을 기다리지 않고 대화를 실행한다. 접수 실패 시 원본 효과를 다시 보정하지 않고 동일 자료를 `RetryCapture()`로 재시도한다. 디스크 실패는 큐의 `RetryFailedWrite()`다. 임시 원본 선택 기록은 교체 저장의 실제 성공까지 유지한다. 자동 저장 전 종료 시 마지막 온전한 저장 전체를 복원한다.

샘플 `UDialogueSampleEffectProvider`는 초기 수량 0에서 해당 제공자만 기록한 **Count Add**를 원장으로 입증할 때만 회수한다. 다른 경로 Add는 보존한다. 소비·Set·Tag·Value·외부 변경·혼합 batch는 자동 추정하지 않고 실패한다. 이것은 게임 전용 보정 구현 예제이며 범용 인벤토리 롤백 엔진이 아니다.

### 사망과 새 게임

게임 상태 초기화 정책은 게임이 소유한다. `Controller->ResetStoryAndSave(false)`는 누적 열람을 보존하고 활성 복원 위치/실행 증거/세션 후보를 초기화해 같은 조율자에 저장한다. true는 누적 이력도 지운다. 제공자가 캡처 불가이면 대화 캐시는 변경하지 않는다. 초기화 뒤 접수 실패면 같은 초기화 자료가 pending으로 남으므로 `Session->RetryCapture()`한다. 등록한 후보 로더와 복원 로더의 늦은 완료는 무효화된다. 다음 후보 선정은 별도 Start/Refresh 호출로 Active 신규 버전을 사용한다. 등록하지 않은 외부 로더는 게임이 `EndSession()`해야 한다.

테스트: `DialogueKit.Restore.SelectiveReplacement`, `VersionRangeRules`. 샘플 Sentry 전송과 실제 연출 화면 검증을 이 테스트가 대신하지 않는다.

## 28. 실제 재시작과 버전 cook 검증

개발자는 에디터를 정상 종료한 뒤 프로젝트 루트 PowerShell에서 실행한다.

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Build
.\Scripts\AI\Invoke-DialogueRestoreValidation.ps1 -IncludePackage
```

`IncludePackage`를 생략하면 실제 두 UE 프로세스의 쓰기/복원만 검증한다. Write 프로세스가 선택 효과·재개 위치를 비동기로 저장한 뒤 종료하고, 다른 PID의 Read 프로세스가 합본을 복원한다. 원본 RestoreOnly 버전의 정확한 다음 Line, 보상 중복 없음, 열람 유지, 사망 초기화 후 Active 신버전 선정과 이전 후보 요청 취소를 검사한다. 패키징 포함 실행은 `ProjectCook.DialogueRegistry`와 `ProjectCook.DialogueVersions`를 추가 실행해 실제 cooked Registry의 Block ID/버전/신규 사용 상태를 확인한다.

결과는 `Saved/AI/RestoreValidation/<GUID>/result.json`과 각 Invoke-UE/Package 실행 폴더에 기록한다. 테스트가 만든 `/Game/DialogueValidation/<GUID>` 에셋은 Git ignore이며 이번 package의 `-CookDir`로만 명시 포함한다. 일반 배포 시 해당 폴더를 추가 cook하거나 참조하지 않는다. 전용 `DialogueRestart_<GUID>` 슬롯·에셋·manifest는 증거로 남고 자동 삭제하지 않는다. 기존 게임 슬롯/샘플 에셋은 수정하지 않는다. 이 검증용 바이너리는 Git에 올리지 않고 생성 코드·문서·스크립트를 관리한다.

두 내부 저장 슬롯은 중단에 대비한 구현 세부사항이다. 플레이어에게 과거 세이브 선택 UI를 제공하지 않는다. 패키지 생성/Registry 검사가 실제 패키지 실행·이미지·음성·입력 검증을 뜻하지 않는다. 실제 화면과 저장 표시 이미지 연결은 S6, 전체 게임·출시 업데이트·강제 종료 회귀와 배포 실행은 S7에서 수행한다.

## 29. 노드 색상과 우클릭 삭제

Graph에서 Line은 기존 파란색, Choice는 보라색, 상태 변경(StateChange)은 청록색으로 표시된다. 각 노드의 Input/Output 핀도 같은 색을 사용하며 미연결 테두리와 연결된 핀 채움 모두 적용된다. 기존 에셋은 재작성할 필요가 없다. 이미 실행 중인 에디터는 새 Editor 모듈을 반영하도록 정상 종료 후 다시 연다. Branch/Start/End의 기존 색상은 유지한다.

작가는 **노드 본문 우클릭 → 대화 노드 작업 → 노드 삭제… → 확인창에서 예** 순서로 해당 노드와 연결선을 삭제한다. 아니요를 누르면 변경하지 않는다. 핀 우클릭에는 노드 전체 삭제 메뉴를 추가하지 않았으므로 연결 작업과 구분된다. 이 메뉴는 Line/Choice/StateChange에 적용하며 기존 Branch/End 메뉴와 Start 삭제 보호는 유지한다. 빈 공간 우클릭은 기존 노드 생성 메뉴다.

새 삭제 동작은 UE transaction에 노드·핀 연결을 함께 기록한다. Ctrl+Z/Undo로 되돌리고 Ctrl+Y/Redo로 다시 삭제할 수 있다. 연결을 자동으로 다른 노드에 우회 연결하지 않는다. 삭제 후 필요한 연결을 작성하고 에셋을 저장한다. 여러 노드 일괄 삭제와 키보드 Delete 명령 통합은 이 보완의 범위가 아니다.

개발자 확장: Flow 계열 노드의 `GetNodeTitleColor()`를 override하면 전용 핀 위젯도 같은 색을 사용한다. 저장된 PinSubCategory 변경은 필요 없다. 에디터 명령에서 `CanDeleteFromGraph()`로 확인하고 `DeleteFromGraph()`를 호출하면 연결 정리/transaction을 공유한다. 이 낮은 수준 함수 자체는 확인창을 띄우지 않으며 메뉴가 확인 UI를 담당한다. Runtime/게임 API는 변경하지 않았다.

검증은 `DialogueKit.Editor.NodeDeleteUndo`의 메뉴 구분·읽기전용 거부·링크 제거·Undo/Redo·Graph export와 관련 Graph 회귀를 사용한다. 실제 GUI에서 색감·확인창 클릭을 관찰한 결과와 비시각 자동화 결과는 구분한다.


## 30. 연속 화자와 Line 저작 설정

### 작가의 작업 순서

1. Line/Branch/Choice의 출력 핀을 빈 공간으로 끌어 놓습니다. `Line 생성`은 기본값으로 만들고 `연속된 화자의 Line 생성`은 가장 가까운 앞선 Line의 표현 설정을 가져옵니다.
2. 같은 거리의 원본이 여럿이면 검색 가능한 원본 선택창에서 화자·본문·이미지·표정·위치·방향·시간을 확인하고 항목을 누릅니다. 창을 닫으면 생성하지 않습니다. 앞선 Line이 없으면 사용 불가 이유가 메뉴에 표시되고 생성되지 않습니다.
3. 복사 항목은 화자, 이미지, 표정, 위치, 방향, 글자당/전체 출력 시간입니다. 본문·액션·효과·열람 기록은 복사하지 않습니다. 기존 출력 연결이 있으면 새 연결로 교체하며 기존 목적 노드는 보존합니다. Undo 한 번으로 생성과 연결 교체를 되돌립니다.
4. Line Details에서 `Seconds Per Character` 또는 `Line Duration`을 바꾸면 반대 항목은 0으로 변경됩니다. 두 항목 기본값은 0(즉시 출력)입니다. 툴바 `출력 시간 일괄 변경`에서 현재 Block의 모든 Line에 적용할 수 있습니다. 일괄 편집도 Undo 한 번으로 복구합니다.
5. 본문 수정 후 기존 플레이어에게 미열람으로 처리하려면 Line Details의 `미열람 갱신`을 누릅니다. Node ID는 유지하고 ReadVersion만 증가합니다. 본문 편집 자체는 열람 버전을 변경하지 않습니다.

### 개발자 연동과 확장

- `UDialogueLineNodeInfo::SetTextTiming(bool bWholeLine, float Seconds)`는 두 모드 중 하나만 설정합니다. 음수/비정상 수는 0으로 정규화합니다. 편집 일괄 API `FDialogueLineAuthoring::ApplyTiming`은 잘못된 입력이나 읽기 전용 Graph에 false를 반환합니다.
- `CopyContinuationDefaultsFrom`은 표현 설정 전용 복사 함수입니다. 향후 새 표현 설정을 추가할 때 이 whitelist와 테스트를 함께 확장하고 게임 효과/이력을 복사하지 않습니다.
- `FDialogueLineAuthoring::FindNearestLines`와 `CreateContinuation`은 Editor 전용입니다. 실행 중 게임 상태를 변경하는 API가 아닙니다.
- `PortraitImage`가 비어 있으면 기존 화자/표정 조회를 사용하는 계약입니다. 순차 출력·이미지 적용·음성 중단은 37~38절의 기본 화면에서 사용합니다. 저작값·저장 왕복과 실제 화면 관찰 검증은 구분합니다.
- 자동화 `DialogueKit.Editor.LineAuthoring`은 상속·새 ID·연결 교체·Undo·일괄 시간·왕복을 검사합니다. 실제 팝업 클릭/레이아웃은 GUI 도구 부재로 별도 확인이 필요합니다.


## 31. 경유점과 영역 주석

경유점의 핀은 미연결 상태의 테두리와 연결 상태의 채움 모두 파란색으로 표시됩니다. 엔진 기본 경유점처럼 입출력이 한 지점에 겹쳐 보이며, 드래그 방향으로 연결 방향을 판단합니다. 별도 설정 없이 기존 경유점에도 적용됩니다.

- 연결선을 더블클릭하면 그 위치에 블루프린트와 같은 경유점이 삽입됩니다. 경유점은 실행 노드가 아니며 위치를 옮겨 긴 복귀 연결선을 정리할 때 사용합니다. 생성과 연결 교체는 한 번의 Undo/Redo로 복구합니다.
- 빈 공간 생성 메뉴의 `경유점`도 사용할 수 있습니다. 출력은 한 개, 입력은 여러 연결을 허용합니다. 저장 전 실제 실행 노드까지 연결해야 합니다. 끊긴 경유점 또는 경유점만의 순환은 저장 변환이 실패하며 이전 실행 자료를 보존합니다.
- 빈 공간 우클릭 → `주석`으로 영역을 생성합니다. 제목을 편집하고 Details에서 Description(설명)·색상·글꼴·Move Mode를 지정합니다. `Group Movement`는 내부 노드와 함께 이동하는 언리얼 표준 주석 동작입니다.
- 개발자는 `FDialogueGraphCodec::ExportGraph/ImportGraph`를 사용합니다. `UDialogueGraph::EditorLayout`은 편집 전용이며 경유점/주석·핀 연결만 저장합니다. 실행 본문은 `Graph`가 정본이고 런타임에는 경유점/주석이 없습니다. cook에서는 편집 전용 property가 제외됩니다.
- 재열기 때 편집 경로의 최종 목적지와 실행 자료가 다르면 임의 수정하지 않고 false를 반환합니다. 새 노드 종류를 확장할 때 저장 변환과 입력 검증을 함께 추가해야 합니다.
- `DialogueKit.Editor.LayoutRoundtrip`은 삽입 Undo/Redo·배치 왕복·순환 거부를, `LayoutFileRoundtrip`은 격리 파일 저장/언로드/재로드를 검사합니다. 실제 드래그·주석 그룹 이동의 화면 확인과 cooked 산출물 검사는 별도 검증 항목입니다.


## 32. 선택 노드 복제

- Graph에서 복제할 노드를 선택하고 **Ctrl+D** 또는 툴바 `복제`를 누릅니다. 선택 집합 안의 연결만 유지하며 외부 입력/출력 연결은 복사하지 않습니다. 단일 노드를 복제하면 연결 없이 새 노드만 만듭니다.
- Start는 복제하지 않습니다. Line/Choice/Branch/StateChange/End·기존 Dialogue 노드·경유점·주석을 지원합니다. 새 노드는 약간 이동된 위치에 생성하고 선택합니다.
- Node ID·Pin ID·Choice ID는 새로 발급합니다. 원본 본문·조건·표현 설정은 복사하지만 Line 열람 버전은 1, 구형 shown 값은 false로 시작합니다. 원본과 복제본의 편집은 독립적입니다.
- Undo 한 번으로 전체 복제를 취소하고 Redo 한 번으로 동일한 새 ID와 내부 연결을 복원합니다.
- 개발자용 `FDialogueGraphEditing::Duplicate(Graph, Selection)`은 Editor 전용이며 새 노드 배열을 반환합니다. 읽기 전용·지원하지 않는 구조·손상된 그래프에서는 빈 배열을 반환합니다. 복제 전에 구조 검증을 수행하므로 미완성 경유점은 먼저 연결해야 합니다.
- 단축키는 Graph Editor 명령 목록에 연결했습니다. 실제 Details 텍스트 입력 포커스와의 충돌 여부는 GUI 검증 대상이며, 자동화는 생성/연결/ID/Undo/Redo를 검증합니다.


## 33. 현재 Block 검색

1. Graph에서 **Ctrl+F** 또는 상단 `DialogueMaker → 검색`으로 하단 검색 패널을 엽니다.
2. `화자`와 `대사`를 함께 입력하면 두 조건에 모두 맞는 Line만 찾습니다. `선택지`는 단독 검색입니다. 선택지와 다른 칸을 섞으면 안내 팝업을 띄우고 실행하지 않으며 입력과 기존 결과를 유지합니다.
3. `검색`을 누르면 현재 열린 Block의 미저장 내용까지 검색합니다. 입력이 모두 비었거나 결과가 없으면 안내합니다. 결과가 없는 유효한 검색은 이전 결과 목록을 비웁니다.
4. 결과의 노드 종류·화자·본문 일부를 확인하고 클릭하면 Graph에서 해당 노드를 선택·이동해 Details를 표시합니다. Choice 결과는 현재 Choice ID에 맞는 항목을 찾아 Details에서 스크롤·강조합니다. 결과 목록은 유지됩니다.
5. 검색 후 노드를 삭제한 경우 클릭 시 다시 검색하라는 안내를 제공합니다. `닫기`는 패널만 숨기며 입력을 지우지 않습니다.

개발자용 `FDialogueGraphSearch::Search`는 순수 저작 텍스트 검색입니다. false는 잘못된 입력이고 결과 배열을 유지합니다. true이면서 빈 배열이면 결과 없음이며 안내 문구를 반환합니다. 게임 조건을 평가하거나 저장/열람 이력을 변경하지 않습니다. 검색 대상의 추가는 이 모델과 결과 이동 코드를 함께 확장합니다. 실제 패널 배치·포커스·Details 강조의 시각 검증은 GUI 확인 항목입니다.


## 34. 그래프 검사와 저장 방어

- 툴바 또는 `DialogueMaker → 그래프 검사`를 누르면 현재 미저장 Graph를 검사합니다. 일반 저장/다른 이름 저장도 같은 검사를 먼저 수행합니다.
- 경고 목록은 스크롤할 수 있습니다. 노드가 있는 항목을 누르면 해당 노드를 선택하고 Details로 이동합니다. `저장 취소`는 저장 없이 닫습니다. `그래도 저장`은 경고를 허용하며, 저장 차단 오류가 하나라도 있으면 비활성화됩니다.
- 기본 선택지만 있으면 경고하되 저장합니다. 기본 선택지가 없어도 조건 없는 일반 선택지가 있으면 해당 누락 경고는 없습니다. 모든 일반 선택지에 조건이 있고 기본 선택지가 없으면 경고하되 저장합니다. 선택지가 전혀 없는 Choice는 저장을 차단합니다.
- 빈 Branch와 불완전한 조건은 경고 후 저장할 수 있습니다. 런타임의 오류 기록과 True 대체 정책은 유지합니다. 미연결 출력·자동 순환 가능성도 검사합니다. 식별자/핀/경유점의 손상이나 Start 개수 오류는 저장할 수 없습니다.
- Save All·자동 저장은 패키지 저장 단계에서도 미저장 작업 그래프의 **차단 오류**를 검사합니다. 이 경로는 저장 내부에서 모달을 열지 않으며 로그와 엔진 저장 실패로 알립니다. 상세 경고 목록은 Graph의 수동 검사 또는 개별 저장에서 확인합니다. cook은 엔진의 패키지 veto 경로에서 제외되므로 배포 전 수동/자동화 검사를 별도로 통과해야 합니다.
- 개발자 API `FDialogueGraphValidation::Inspect`는 위치·코드·설명·저장 차단 여부를 반환합니다. `CanSave`로 검사 결과를 판정하고 `CanSavePackage`는 열린 작업 그래프를 우선 검사합니다. 게임 상태·보상·이력은 조회하거나 변경하지 않습니다.
- 저장 guard는 엔진의 기존 `IsPackageOKToSaveDelegate`를 먼저 호출합니다. 다른 플러그인이 같은 단일 delegate를 덮어쓰는 경우 통합 검증이 필요합니다. 모듈 종료 시 자신이 마지막 바인딩일 때만 기존 delegate를 복원합니다.


## 35. 스토리보드와 Block 소속·대상 편집

### 작가 작업 순서

1. 콘텐츠 브라우저에서 **Data Asset → DialogueStoryDefinition**으로 게임 콘텐츠 폴더에 정의 에셋을 만듭니다. 에디터 상단 **DialogueMaker → 스토리보드**에서 위 에셋 선택기로 지정합니다. 같은 설정은 Project Settings의 **DialogueKit Settings → Story → Story Definition**입니다.
2. 왼쪽 `+ Chapter`, 가운데 `+ Scene`으로 등록합니다. 오른쪽에서 이름·설명·정렬 순서를 편집합니다. ID는 이름 수정이나 이동으로 변경되지 않습니다. 정렬/외부 편집 후 `새로고침`으로 목록을 갱신합니다.
3. Chapter를 선택하면 가운데 위에 Scene 카드와 Block 수가 표시됩니다. Scene을 선택하면 가운데 아래에 Block 목록, 오른쪽에 Scene 설정이 표시됩니다. Block 목록은 이름/대상 태그 검색과 스크롤을 지원하며 우선순위·기본/최우선 구분·신규 사용/복원 전용을 보여줍니다.
4. Block 한 번 클릭은 오른쪽 설정, 두 번 클릭 또는 `열기`는 Graph 편집기를 엽니다. Block 소속 드롭다운에서 소속 없음·제한 없음·Chapter 전체 Scene·특정 Scene을 선택합니다. 목록은 정의 에셋에서 가져오며 게임의 현재 진행 상태를 변경하지 않습니다.
5. Scene의 오른쪽 `Scene을 다른 Chapter로 이동`으로 부모를 바꿉니다. Block은 체크박스로 복수 선택한 뒤 목적 Chapter/Scene을 클릭하고 `현재 소속으로 이동`을 누릅니다. **선택은 카테고리 전환에도 유지**되므로 확인창의 전체 경로 목록을 확인하세요. 취소하면 변경하지 않습니다. 이동은 하나의 Undo로 복구합니다.
6. Chapter/Scene `제거`는 영향 항목을 표시하는 두 확인을 거칩니다. Block 에셋은 삭제되지 않습니다. 사라진 Scene ID는 Undo 복원을 위해 보존하며 `소속 없음 / 삭제된 소속`에서 재배치합니다. 소속 없는 Scene의 Block도 신규 실행 후보에서는 제외됩니다.
7. 불필요한 **미출시** Block은 소속 없음에서 선택하고 삭제합니다. 첫 확인에 정확한 경로, 두 번째 확인에 모든 대상의 미출시 여부를 명시적으로 확인합니다. RestoreOnly·미저장·열린 Graph·소속 있는 Block은 거부합니다. 실제 삭제는 Undo 대상이 아니며 참조로 삭제하지 못한 에셋을 결과에 표시합니다. 출시 에셋은 삭제하지 말고 RestoreOnly로 보존하세요.
8. 정의와 각 Block 변경은 해당 에셋에서 저장합니다. `정의 에셋 열기/저장`은 기본 에셋 편집기를 열며 자동 저장 버튼은 아닙니다.

### 게임 개발자 설정과 공개 확장 지점

- `UDialogueSettings::StoryDefinition`은 저작 원본입니다. 게임은 동일 정의의 ID를 사용해 `FDialogueStoryProgress`를 제공하며 현재 진행/전환 책임을 유지합니다. 런타임 자동 로드나 cook 포함을 보장하는 설정은 아니므로 게임이 정의 에셋을 참조하거나 cook 포함 규칙을 지정하고 로더에 전달해야 합니다.
- `TargetFieldLabel`로 대화 NPC·대화 장소 등 필드 표시명을, `TargetDisplayNames`로 등록 GameplayTag별 게임 표시명을 지정합니다. Project Settings에서 설정하거나 게임 설정 코드/Config로 공급합니다. 표시명은 식별자가 아니며 바꿔도 저장된 Tag는 그대로입니다.
- Block Details 대상 드롭다운의 최상위 분류는 등록된 GameplayTag에서 동적으로 수집합니다. 선택 목록은 태그 원문과 게임 표시명 모두 검색합니다. 필터/검색만 바꾸면 Block 값은 바뀌지 않습니다. 실제 선택한 **태그 하나**만 저장하고 런타임은 정확 일치합니다.
- Editor 전용 `FDialogueStoryEditor::CollectBlocks`, `MoveScene`, `MoveBlocks`, `RemoveChapter`, `RemoveScene`은 메타데이터 기반 조회 및 Undo 가능한 저작 변경입니다. `DeleteUnassigned`는 UI의 두 확인과 미출시 확인 이후에만 호출하며 반환 bool/결과 메시지를 처리해야 합니다. 게임 런타임 API로 호출하지 마세요.
- Block 목록은 Asset Registry에서 수집하고, 이미 로드된 Block은 현재 편집값을 사용합니다. 목록만 보기 위해 모든 Graph를 로드하지 않으며 상세 편집/이동/삭제 대상만 로드합니다. 읽을 수 없는 메타데이터는 확인 필요 항목으로 표시하고 삭제 대상에서는 제외합니다.
- 실제 UI 배치·클릭과 삭제 확인 팝업은 GUI 미검증입니다. 자동화는 임시 객체의 소속/안정 ID/Undo/삭제 거부와 Saved/AI 격리 에셋의 선택 삭제·비선택 파일 보존을 검증했습니다. 기존 프로젝트 에셋은 삭제하지 않았습니다.


## 36. 신규 조건과 구형 조건 구분

### GameplayTag 선택 및 복수 조건 작성

1. Branch 노드를 선택하고 Details의 `DialogueKit → Branch → Conditions`를 펼칩니다. Choice는 `Options`에서 일반 선택지의 `Conditions`를 펼칩니다.
2. 내부 `Conditions` 배열의 `+` 버튼으로 항목을 추가하고 해당 항목의 `Key`를 펼칩니다.
3. `Kind`를 `Tag`로 설정한 뒤 **`Key → Tag`의 태그 선택 버튼/드롭다운**에서 프로젝트에 등록된 GameplayTag를 선택합니다. `Kind=Tag`는 종류 지정일 뿐 실제 검사할 태그 선택이 아닙니다.
4. 보유 여부는 `Comparison=Equal`, `Tag Operand=true`로 검사합니다. 없는 태그를 요구하려면 `Equal`, `Tag Operand=false`로 설정합니다.
5. 복수 태그는 태그마다 배열 항목을 추가합니다. 바깥 `Combine=All`이면 전부 충족, `Any`이면 하나 이상 충족해야 합니다. 예: `Quest.Accepted`, `Item.KeyOwned`를 각각 지정하고 All이면 두 태그가 모두 필요합니다. 예시 태그는 게임이 등록해야 합니다.

태그 목록이 비어 있다면 프로젝트 설정의 Gameplay Tags에서 사용할 태그를 먼저 등록합니다. 기본 선택지(`Default=true`)는 조건을 평가하지 않아 Conditions가 숨겨집니다. `Legacy → 구형 조건`은 읽기 전용이므로 그곳에서 태그를 수정할 수 없습니다. 신규 `Key → Tag` 경로에도 선택기가 없다면 정상적인 상태가 아니며 해당 화면을 기준으로 추가 확인이 필요합니다.

복수 태그는 한 드롭다운에서 일괄 선택하는 방식이 아니라 **복수 조건 행**으로 지원합니다. 기존 Value/Count와 섞어 같은 All/Any로 평가할 수 있습니다. 게임 개발자는 `IDialogueStateProvider`의 태그 조회 구현을 제공하며, 이번 안내는 기존 저장 형식과 평가 로직을 변경하지 않습니다.

- 새 Branch는 `DialogueKit → Branch → Conditions`를 편집합니다. `Legacy → 구형 조건 (신규 실행기 미사용)`은 기존 값 확인용 읽기 전용입니다. 자동 변환하지 않습니다.
- Block 진입 조건은 `Entry Conditions`, 선택지 표시 조건은 각 `Options → Conditions`입니다. 조건 배열의 추가/삭제와 All/Any 조합을 사용하며, Tag/Value/Count 항목과 비교 기준을 지정합니다. 게임은 기존 `IDialogueStateProvider`로 값을 제공해야 합니다.
- Block Details의 `Legacy 안내`를 확인하세요. 구형 Required All/Any/Blocked Tags가 남으면 신규 후보에서 제외됩니다. 신규 조건에 필요한 내용을 수동 작성한 다음 구형 태그를 명시적으로 비우세요. 구형 `SpeakerID`/`ChapterID`는 새 `DialogueTarget`/`StoryScope`를 대신하지 않습니다.
- 기존 직렬화 필드와 호환 함수는 유지합니다. 읽기 전용 표시는 신규 작가의 혼동을 줄이기 위한 에디터 제약이며 외부 게임 코드를 자동 변환하지 않습니다.


## 37. 텍스트 표시 상태와 안전한 입력

`UDialogueTextPresentation`은 게임 상태를 변경하지 않는 공개 표시 도구입니다. 기본 UMG 또는 게임의 커스텀 UI에서 사용합니다.

1. 지역화된 `FDialoguePresentation`을 `SetPresentation`으로 전달합니다. 0초는 즉시 완료, 전체 시간 또는 글자당 시간은 Line 설정을 따릅니다. 글자 경계를 사용해 결합 문자/emoji 중간에서 자르지 않습니다.
2. 출력 중 실제 경과 시간을 `AdvanceTime`에 전달하고 `GetVisibleText`, `GetVisibleChoices`로 화면을 갱신합니다. 텍스트가 완료되면 직접 연결된 Choice가 노출됩니다.
3. 입력 당시 `GetRevision`과 물리 입력 token을 `RequestAdvance`/`RequestChoice`에 전달합니다. 기본 화면은 프레임 번호를 token으로 사용합니다. 동일 token은 새 Line에서도 다시 수락하지 않습니다.
4. `Reveal`이면 실제 음성을 중지하고 표시만 갱신합니다. `Advance`에서만 Runner의 `AdvanceLine`, `Select`에서만 `SelectChoice`를 호출합니다. `None`은 무시합니다. 표시 객체가 효과/저장/Line 통과를 직접 수행하지 않습니다.
5. `SetVoicePlaying(상태, 예상 revision)`으로 실제 재생 상태를 전달합니다. 이전 Line의 지연 콜백은 현재 상태를 바꿀 수 없습니다. bool 변경 자체가 음성을 중지하지 않으므로 실제 컴포넌트의 Stop 책임은 화면에 있습니다.
6. 게임 상태가 변경되면 `UDialogueBlockRunner::RefreshChoices`를 호출합니다. Choice-only 이벤트는 현재 출력률을 유지합니다. 실제 재진입/복원은 새 revision으로 처음부터 표시합니다. `CancelBlock`은 Idle을 게시하여 화면이 수명을 정리하도록 합니다.

기존 Subsystem의 `ProgressNextDialogue`/`SelectBlockChoice`와 Runner 호출은 저수준 호환 API로 남습니다. 새 화면의 물리 입력에 직접 연결하면 펼치기 방어를 우회하므로 위 입력 판정 또는 기본 화면의 Submit API를 사용하세요. 이 절의 자동화는 비시각 상태 검사이며 실제 화면/음향 검증은 별도입니다.


## 38. 기본 대화 화면과 이미지·음향

### 게임 개발자 연결

`UDialoguePresentationWidget`은 별도 바이너리 UI 에셋 없이 사용하는 고정 기본 화면입니다. 기존 `DialogueWidgetClass`나 샘플 Widget을 자동 대체하지 않습니다. Blueprint Designer 상속용 화면이 아니며, 커스텀 UI는 37절의 표시 객체와 Subsystem 이벤트를 사용합니다.

1. 플레이어를 소유자로 `CreateWidget<UDialoguePresentationWidget>` → `AddToViewport` → `BindSubsystem` 순서로 연결합니다. 단독 Runner는 `BindRunner`를 사용합니다. 두 구독을 동시에 사용하지 않습니다.
2. 게임의 PlayerController에서 UI 입력 모드를 지정하고 `SetWidgetToFocus(View->TakeWidget())` 또는 `SetUserFocus`로 포커스를 줍니다. 이동 제한·커서·종료 후 입력 복원은 게임 책임입니다.
3. 연결 후 `StartDialogueBlock`을 호출합니다. 출력 중 배경 클릭/Space/Enter는 전체 표시와 음성 중지만 합니다. 동일 프레임의 추가 입력과 키 반복은 진행하지 않습니다. Choice 버튼은 생성 당시 ID·화면 revision을 사용합니다.
4. 종료/취소 시 화면은 숨겨지고 자신이 만든 음향 컴포넌트를 정리합니다. `DetachPresentation`/`RemoveFromParent`는 저장 큐나 게임 상태를 취소하지 않습니다. 제거한 Widget을 재사용하면 `AddToViewport` 이후 반드시 다시 Bind하세요.

### 작가 설정

- Line Details에서 직접 `Portrait Image`를 지정하거나 비워 두고 화자+표정이 모두 일치하는 `DialoguePortraitData`를 사용합니다. `Portrait Offset`, 좌/우/중앙 위치, `Mirror Portrait`를 반영합니다. 직접 이미지가 지정되어 있으면 표정 에셋보다 우선합니다.
- `Line Audio`의 `Background Music`, `Sound Effect`, `Voice`에 Sound 에셋을 지정합니다. 기본값은 무음입니다. BGM은 각 Line에 명시하며 빈 값은 정지, 동일 BGM은 이어집니다. 반복 BGM은 Sound 에셋에서 loop를 설정하세요. 효과음과 음성은 새 Line마다 시작합니다.
- 출력 중 입력하면 음성만 중지하며 BGM/효과음은 유지합니다. 새 Line/취소/화면 제거에서는 이전 Line 음성·효과음을 정리합니다. Choice 조건만 갱신하면 텍스트와 음향을 재시작하지 않습니다.
- 연속 화자 생성은 기존 합의된 이미지·시간 설정만 계승하며 음향은 복사하지 않습니다. 게임의 기존 ActionSequencer를 자동 실행하는 화면은 아닙니다.

이미지·음향은 표시할 때 동기 로드하므로 큰 리소스는 게임에서 미리 로드해 두세요. 실제 프레임·비율·포커스·소리 확인은 PIE 관찰 대상이며 화면 없는 자동화 성공으로 대체하지 않습니다. 게임별 커스텀 화면은 공개 `UDialogueTextPresentation`과 지역화된 `OnBlockPresentationChanged`를 사용하고 물리 입력을 37절의 판정으로 처리합니다.


## 39. 비동기 저장 표시

게임은 `UDialogueSavingWidget`을 대화 화면과 별도로 한 번 생성하여 viewport에 추가하고, 공통 저장 조율자의 `UDialogueSaveQueue`를 `BindQueue`로 연결합니다. 대화 화면을 닫아도 이 Widget은 유지하세요. 생성 전에 `SavingImage`를 지정하면 해당 이미지를, 없으면 기본 원형 애니메이션을 우측 하단에 표시합니다.

- 대기 요청이 있으면 저장 중, 실패하면 저장 실패, 마지막 요청까지 성공하면 숨김 상태입니다. 요청 사이의 일시적인 writing=false 때문에 깜빡이지 않습니다.
- 화면은 입력을 차단하지 않고 진행·선택·효과·저장을 직접 실행하지 않습니다. 게임의 비동기 저장 어댑터가 실제 쓰기를 처리하고 `CompleteWrite`를 호출하는 계약은 그대로입니다.
- 실패 후 `RetryFailedWrite`를 호출할지 등의 판단은 게임 책임입니다. Widget은 실패를 성공으로 처리하거나 자동으로 재시도하지 않습니다.
- 사용자 UI는 `OnSaveStateChanged`를 구독하고 `GetPendingCount`/`HasFailedWrite`로 같은 표시를 구현할 수 있습니다. 기본 Widget의 `IsSavePending`/`IsSaveFailed`도 공개 조회 함수입니다.
- Widget 제거는 큐를 취소하지 않습니다. 재사용하면 `BindQueue`를 다시 호출합니다. Block PIE에는 실제 저장 큐를 연결하지 않습니다.

자동 검증은 실제 UMG 객체에서 두 요청의 순차 완료·실패·재시도·최종 숨김을 확인합니다. 표시 위치와 애니메이션의 시각적 확인은 별도 PIE 관찰 대상입니다.


## 40. Block 테스트의 게임 상태 격리

`UDialoguePreviewPolicy::IsPreviewWorld(WorldContextObject)`는 Block 테스트가 활성화된 PIE 월드인지 알려줍니다. 게임의 `UGameInstanceSubsystem`/`UWorldSubsystem` 등 자동 생성 시스템은 `ShouldCreateSubsystem(Outer)`에서 이 값을 검사해 생성을 생략하거나, `Initialize`에서 격리 여부를 보관하고 해당 인스턴스의 모든 파일·네트워크·영속 효과를 차단하세요. 전역 활성 여부만 검사해 편집기 월드의 작업까지 차단하지 마세요.

DialogueKit의 기존 대화 이력·PlayerProgress·Quest Subsystem은 초기화부터 실제 슬롯 읽기/쓰기를 막고 메모리 SaveData를 사용합니다. 격리 여부는 인스턴스에 남아 테스트 종료 과정에서도 쓰기가 다시 허용되지 않습니다. 일반 게임 인스턴스의 동작은 유지합니다.

`UDialoguePreviewStateProvider`는 샘플 조건 값을 조회하고 유효 효과의 흐름만 통과시킵니다. 보상/소모·태그 변경을 실제로 적용하지 않으며 저장 큐도 만들지 않습니다. 따라서 효과 이후의 조건까지 시험하려면 해당 초기 상태를 별도 프리셋으로 지정하세요. 이 제공자를 게임의 실제 효과 제공자로 등록하지 마세요.

플러그인은 외부 게임 시스템의 임의 파일/네트워크 코드를 가로채지 않습니다. 외부 Subsystem은 위 공개 격리 API를 적용해야 합니다. 배포 빌드에서는 미리보기 활성화 API가 없고 조회 결과는 항상 false입니다. 자동화는 PIE 문맥에서 세 Subsystem의 초기화·메모리 변경·종료 후 저장 호출 전후로 기존 SaveGames 파일 집합과 내용을 비교합니다.


## 41. 현재 Block 단독 PIE 테스트

### 시나리오 작가

1. Block Graph 편집기 상단 **DialogueMaker → 테스트 → 프리셋 설정**을 엽니다. `Presets` 배열에 이름 있는 항목을 추가하고 `Selected Preset`에 사용할 번호를 지정합니다(첫 항목은 0).
2. 프리셋 `Definitions`에 게임 상태 키와 표시명을 등록하거나 `Definition Table`에 공통 `FDialogueStateDefinition` 행 DataTable을 지정합니다. 레벨/능력치는 `Value`, 보유 아이템 수량은 `Count`, 퀘스트/진행도는 `Tag`입니다. `States`에 같은 키와 현재 값을 입력합니다. 정의의 중복, 정의 없는 현재 값, 잘못된 값은 시작 시 거부됩니다. 정의만 있고 현재 값이 없으면 샘플 기본값 0/false입니다.
3. 예: `Value / Player / Level` 정의와 현재 값 3을 지정하고 Block 진입 조건을 `Level >= 5`로 만들면 일반 테스트는 시작하지 않고 현재 값·기준·False 판정을 안내합니다. 현재 값을 5로 바꾸거나 **진입 조건 무시하고 테스트**를 사용합니다. 강제 테스트에서도 Branch와 각 Choice 조건은 정상 평가합니다. 조건 조회 오류는 기존 런타임 정책대로 로그 후 True 대체되므로 정의 누락 경고를 먼저 확인하세요.
4. **현재 Block 테스트**는 Graph를 검사한 뒤 미저장 내용을 임시 복제하여 엔진의 빈 맵에서 실제 PIE를 시작합니다. 경고가 있으면 확인 후 실행할 수 있고 구조상 차단 오류는 먼저 수정해야 합니다. 게임의 원래 맵/GameMode/GameInstance 대신 빈 맵과 기본 클래스를 사용합니다.
5. 텍스트·선택지·초상화·음향을 확인하고 클릭/Space/Enter로 진행합니다. 테스트 중에는 Graph·핀·Details·복제·출력시간 변경·편집 Undo/Redo·에셋 저장을 잠급니다. 화면 이동·확대·선택은 유지합니다. 편집기를 닫으려면 먼저 테스트를 종료합니다.
6. **DialogueMaker → 테스트 → 테스트 종료** 또는 UE의 PIE 종료 버튼으로 종료합니다. 다음 실행은 프리셋 초기 값부터 시작합니다. 완료된 Block 화면이 사라져도 PIE는 종료 버튼을 누를 때까지 유지됩니다.
7. 프리셋을 다음 에디터 실행에도 보존하려면 **프리셋 명시적 저장**을 누릅니다. 창 닫기와 테스트 시작/종료는 프리셋을 저장하지 않습니다. 이는 로컬 사용자 설정이며 Block 에셋/실제 게임 저장과 별개입니다.

### 개발자 확장과 검증

- 외부 자동 초기화 Subsystem은 반드시 40절의 격리 API를 적용하세요. 이 도구는 임의 외부 시스템의 파일/네트워크 부수 효과를 차단하는 운영체제 sandbox가 아닙니다.
- Editor 전용 `FDialogueBlockPreview::CopyWorkingBlock`, `MakeProvider`, `CheckEntry`를 자동 검증/개발 도구에 사용할 수 있습니다. `Start`의 선택적 프리셋 인자는 저장 설정을 바꾸지 않고 특정 값을 시험하는 경로입니다. `GetRunner`는 현재 테스트의 실행 상태 조회용입니다.
- 선택 결과의 아이템 지급/소모/태그 변경은 시뮬레이션하지 않습니다. 변화 이후 조건은 별도 프리셋으로 시험합니다. 실제 게임 효과·체크포인트 세션·저장 큐는 연결하지 않습니다.
- `Invoke-UE.ps1 -Action Test -TestFilter DialogueKit.Preview`는 복제·조건·격리를, `-TestFilter ProjectPreview.BlockPIELifecycle`는 실제 엔진 PIE 시작/종료 경로를 검사합니다. 기본 래퍼의 NullRHI/nosound 실행은 화면·소리의 관찰이 아닙니다.
- 수동 확인: 0초/전체1초/글자당시간, 출력·음성 중 입력, Choice 등장, 동일 BGM 유지/빈 값 정지, 화자 표정·위치·반전, 종료 후 편집 복구를 확인하세요. 실제 키 포커스·렌더링·음향 청취 결과는 별도 기록해야 합니다.

## 42. 안정 선택 ID와 하위 폴더 지역화

작가는 기존 DialogueGraph CSV 내보내기/번역 가져오기 메뉴를 그대로 사용한다. 새 CSV에는 기존 `NodeGuid,PinId,Key,Value`에 `ChoiceId` 열이 추가된다. 번역자는 ID 열을 보존하고 Value만 수정한다. 예전 4열 CSV도 가져올 수 있다. 신규 Choice는 ChoiceId를 먼저 조회하고 예전 번역은 PinId로 대체하므로 핀 재생성 뒤에도 신규 번역이 유지된다. Node/Choice 자체를 새로 만들면 해당 ID의 번역도 다시 작성한다.

게임 개발자는 `UDialogueLocalizationSubsystem::SetCurrentLanguage` 또는 `SetCurrentCultureCode`를 호출한다. 언어 전용 `en`, `ko`는 지원 코드 `en-US`, `ko-KR`로 정규화하고 명시적인 `zh-Hant` 같은 문화권은 그대로 둔다. 빈 문자열/공백은 false다. `ResolveDialogueText`는 NodeGuid, `ResolveChoiceText`는 NodeGuid·ChoiceId와 구형 fallback용 PinId를 받는다. 기존 `ResolveResponseText`도 유지된다. Subsystem 표시 이벤트에는 번역 복사본이 전달되며 원본 에셋은 변경하지 않는다.

하위 폴더 번역은 AssetManager의 `DialogueLocalization` PrimaryAsset 등록 경로로 검색한다. 기본 `/Game/DialogueLocalization`은 recursive scan/cook 대상이다. 커스텀 생성 경로를 쓰는 게임은 AssetManager에 해당 경로를 등록하고 cook에 포함해야 한다. 등록되지 않은 경우 기존 루트 경로 조회를 시도하고 번역 누락 시 원문을 반환한다. 같은 Graph·문화권의 번역 에셋을 중복 생성하지 않는다.

검증: `DialogueKit.Localization`은 언어 목록과 CSV→DataAsset→런타임 조회, 하위 폴더, 핀 변경, 구형 CSV를 검사한다. 이는 실제 패키지 UI의 번역 표시 관찰을 대신하지 않는다.

## 43. 샘플 게임 호스트로 선정과 저장 연결

`Source/DialogueMaker/DialogueSampleHost.h`의 `UDialogueSampleHost`는 게임 모듈의 컴파일 가능한 연결 예제다. 플러그인에 게임 시스템을 넣거나 기존 BeginDialogue UI를 자동 교체하지 않는다. GameInstance 등에서 UPROPERTY로 보유하고 상태·효과·저장·보정 제공자를 먼저 준비한다.

```cpp
Host = NewObject<UDialogueSampleHost>(GameInstance);
bReady = Host->Initialize(DialogueSubsystem, StateProvider, EffectProvider,
    SaveProvider, SaveQueue, CorrectionProvider, DiagnosticRouter);
// OnStarted 구독은 요청보다 먼저 한다. 이미 로드된 에셋은 동기 완료할 수 있다.
Host->RequestDialogue(TargetTag, StoryDefinition, RestoredStoryProgress);
```

Initialize 실패 시 시작하지 않는다. RequestDialogue/RequestCandidates의 유효 GUID는 요청 접수이며 실행 성공은 `OnStarted(bool, ErrorCode)`로 확인한다. 로드 대기 중 최신 요청으로 교체하고, 표시/효과/저장 접수 중 재진입은 거부한다. 게임은 연타·이동·상호작용 정책을 입력 계층에서 추가한다. 기본 공통 인사는 `GetLoader()->CommonGreetingBlock`으로 지정한다. `StartDialogueBlockForTarget(Block, RequestTarget)`은 인사 에셋에 대상이 없더라도 실제 요청 대상을 저장한다. 기존 `StartDialogueBlock(Block)`은 에셋의 대상 사용을 유지한다.

호스트는 실행 성공 뒤에 노출을 기록하고 정상 End에만 후보 소진을 기록한다. 첫 자동 효과의 캡처가 실패하면 같은 요청을 보존한다. 이 샘플에서는 **Host의 RetryCapture**를 호출해 접수와 노출/완료 기록을 함께 재개한다. 디스크 실패는 `SaveQueue->RetryFailedWrite()`로 재시도한다. 사용자 안내와 Sentry 전송은 결과/진단 구독자가 담당한다.

게임 저장 조율자가 동일 합본의 게임 상태를 먼저 복원한 뒤 `RestoreDialogue(Snapshot, ReplacementMap)`을 호출한다. 최종 결과는 `GetRestoreController()->OnRestored`다. 복원 시작은 늦은 신규 후보를 무효화한다. 대기 중 쓰기/미접수 캡처가 있으면 복원은 거부된다. `EndConversation()`은 실행·화면·세션 순회를 닫지만 미완료 저장 재개점을 폐기하지 않는다. 다음 신규 시작은 거부되므로 해당 대화를 복원하거나 게임의 명시적 사망/새게임 초기화 흐름을 사용한다. Runner callback 내부의 종료는 false이므로 UI에서 다음 프레임에 종료 요청을 처리한다.

`ShowPresentation(LocalPlayerController)`은 기본 대화 위젯과 별도 저장 표시 위젯을 명시적으로 생성/바인딩한다. 입력 모드·포커스·게임 이동 정책은 게임이 정한다. EndConversation 뒤에도 저장 표시는 유지된다. 레벨 이동/플레이어 교체에서는 호스트와 위젯 수명을 게임에서 다시 구성한다. 커스텀 UI는 같은 Subsystem 이벤트에 연결하면 된다.

샘플 `UDialogueSampleSaveCoordinator`는 예제 상태/효과 원장과 대화 snapshot을 저장한다. **Chapter·Scene, 퀘스트 본체 등 실제 게임 전체 상태를 저장하는 제품 저장 시스템이 아니다.** 게임 개발자는 같은 캡처 호출 안에서 진행도와 게임 상태를 합본에 넣고 함께 복원해야 한다. 호스트는 복원된 `FDialogueStoryProgress`를 요청 때 받아 필터링한다. 기존 샘플 저장 형식을 임의로 바꾸지는 않았다. Prototype2 통합은 별도 작업이다.

검증은 `DialogueKit.Integration`: Subsystem 선택/효과·합본·재개, 공통 인사 대상, 종료와 재시도, 초기 자동 효과의 중복 방지, 복원 시 지연 후보 취소를 검사한다. 실제 디스크/프로세스 및 패키지 실행은 별도 검증 결과를 따른다.

## 44. 실제 패키지 저장과 강제 종료 복원 검사

개발자는 다음 순서로 실행한다. 모든 산출물은 Git에서 제외되는 `Saved/AI`의 새 폴더에 남기며 스크립트/테스트/본 안내서는 Git에 포함한다.

```powershell
# 고유 GUID의 보존 v1/활성 v2 fixture와 저장을 만든다.
.\Scripts\AI\Invoke-DialogueRestoreValidation.ps1 -IncludePackage
# 위 실행의 RestoreValidation/<GUID> 및 대응 Package_*/result.json을 사용한다.
.\Scripts\AI\Invoke-PackagedDialogueValidation.ps1 `
  -PackageResult "<대응 Development 패키지 result.json 절대 경로>" `
  -DialogueValidationId "<위 fixture의 32자리 GUID>"
```

후자는 bootstrap이 아닌 실제 게임 exe를 세 번 실행한다. Write는 v1 선택 효과와 재개점을 실제 비동기 저장한다. Crash는 동일 저장을 호스트로 복원하고 추가 효과를 적용하되 디스크 writer 직전 큐를 보류한다. 대기 marker의 PID가 래퍼의 실행 PID와 같은 것을 확인한 뒤 그 프로세스만 강제 종료한다. Read는 마지막 저장의 수량 10, 동일 Block ID·v1·실행 ID·재개 노드, 효과 비중복을 검사하고 정상 End 이후 활성 v2 후보를 실행한다.

`PackagedWriteCrashRestorePassed`는 세 단계와 기존 프로젝트 슬롯 해시 불변을 모두 통과했다는 뜻이다. 실행마다 절대 `userdir`를 지정해 저장·로그를 격리하며 Crash 전후 durable 파일 해시도 같아야 한다. 실패/timeout/로그 Error/결과 파일 없음은 실패다. 래퍼는 기존 사용자 프로세스를 종료하지 않는다. 검증 함수를 일반 게임 실행에서 호출할 필요는 없다. Game 모듈의 검증 경로는 명시적 GUID·phase·절대 userdir 인자가 있는 Development에서만 활성화되고 Shipping에서는 제외된다.

이 검사는 **저장 큐가 대기 중인 실제 프로세스 종료**다. OS가 파일의 일부 바이트를 기록하는 순간의 전원 차단을 재현하지 않는다. 손상된 내부 슬롯의 fallback은 기존 저장 자동화 검사와 구분한다. NullRHI/nosound이므로 화면·실제 키/마우스·음향 청취를 검증하지 않는다.

Shipping은 테스트 fixture를 지정하지 않고 별도로 패키징한다.

```powershell
.\Scripts\AI\Invoke-PackageValidation.ps1 -Action Package -Configuration Shipping
# Shipping cook 직후, 다른 cook으로 덮기 전에 실행한다.
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter ProjectShipping -TimeoutMinutes 5
```

Shipping에 DialogueValidationId를 함께 주면 스크립트가 거부한다. ProjectShipping 검사는 현재 cooked Registry에서 테스트 에셋/태그가 없는지 확인하므로 직전 cook의 Configuration은 패키지 result.json으로 함께 확인한다. Shipping의 일반 실행/종료는 별도 검증이다. Development 자동 종료 인자나 `-ExecCmds`를 Shipping 정상 종료 검증으로 간주하지 않는다.

## 45. 초상화 없는 기존 대화와 Shipping 샘플

기존 `BeginDialogue`/개발용 `PlayDialogueGraph` 경로에서 필요한 초상화 에셋이 0개이면 사전 로드 없이 대화 UI를 생성한다. 별도 작가 설정은 필요 없다. 종료 시 preload handle이 없더라도 안전하게 처리한다. 존재하는 초상화의 정상 비동기 로드는 기존 경로를 유지한다.

샘플 `ATEST` 액터의 직접 대화 실행은 Development 전용이며 Shipping에서는 실행하지 않는다. 테스트 Graph를 지정하지 않은 경우도 호출하지 않는다. 실제 게임은 샘플 액터의 테스트 동작에 의존하지 않고 공개 게임 호스트/Subsystem 진입점을 연결한다. 기존 테스트 에셋이나 샘플 맵 바이너리는 변경하지 않았다.

## 46. Line 노드 내용 표시

Line을 선택하고 Details의 `Title`, `Speaker ID`, `Dialogue Text`를 편집합니다. 제목은 노드 제목 막대에, 화자와 대사는 핀 아래에 표시됩니다. 빈 제목은 `Line`이며 긴 본문은 폭 300 기준으로 줄바꿈합니다. 내용은 읽기 전용 미리보기이며 수정은 Details에서 합니다. Undo 또는 저장 후 재열기에도 같은 노드 정보를 읽습니다.

게임 개발자의 추가 호출은 필요 없습니다. 기존 `UDialogueNodeInfo`의 `GetTitle()`, `GetSpeakerEmotePair()`, `GetDialogueText()`를 읽으며 런타임/저장 형식은 바꾸지 않습니다. Choice·StateChange·경유점은 기존 전용 표현을 유지합니다. 실제 화면의 색감·배치는 GUI 인수 확인 대상입니다.
