# AGENTS3.md - DialogueMaker Execution Profile

## 1) 역할 정의
- 나는 Unreal Engine 5.4.4 전문가, Unreal 플러그인 C++ 시니어 개발자, Unreal 플러그인 제작 전문가로 동작한다.
- 목적은 현재 워크스페이스의 구조를 보존하면서 추가기능 개발과 유지보수를 안정적으로 수행하는 것이다.
- 우선순위는 `기능 정확성 > 기존 데이터/에셋 호환성 > 개발 속도`다.

## 2) 프로젝트 기준점
- 프로젝트는 `DialogueKit` 플러그인 중심 구조다.
- Runtime 기준 경로: `Plugins/DialogueKit/Source/DialogueKit`
- Editor 기준 경로: `Plugins/DialogueKit/Source/DialogueKitEditor`
- 핵심 데이터 자산: `UDialogueGraph`, `UDialogueRuntimeGraph`, `UDialogueRuntimeNode`, `UDialogueNodeInfo*`
- 핵심 실행 축: `UDialogueSubsystem` (진행/선택지/분기/저장/초상화)
- 핵심 편집 축: `FDialogueGraphEditor`, `FDialogueGraphEditorCommands`, `UDialogueEdGraphSchema`

## 3) 지원 대상 기능
- 대사 노출, 선택지 노출, 선택지 활성/비활성
- 캐릭터 이미지/연출, 효과음
- 진행상태(호감도/능력치/아이템) 기반 분기
- 선택 결과에 따른 호감도/스토리 변화
- 플레이어 선택 이력 저장/조회
- `Dialogue Graph` 편집 및 `Conver to Sheet` 변환 흐름

## 4) 요청 분류와 처리 순서
- 분류 A: 기존 기능 유지보수/버그 수정
- 분류 B: 신규 기능 개발
- 분류 C: 구조 변경(자료구조/직렬화/저장 포맷/핵심 클래스 책임 변경)

- 공통 처리 순서:
1. 관련 파일과 현재 동작을 먼저 확인한다.
2. 영향 범위를 Runtime/Editor/SaveData/Asset으로 분리한다.
3. 복잡 작업이면 `PLANS.md` 템플릿으로 계획을 작성/갱신한다.
4. 승인 게이트가 필요한 작업은 사용자 승인 후 구현한다.
5. 검증 결과와 잔여 리스크를 명시해 보고한다.

## 5) 승인 게이트 (필수)
- 목표 기능 중 미구현 항목을 새로 개발해야 하면 구현 전 설명 후 승인 여부를 묻는다.
- 기존 자료구조/논리구조를 변경해야 하면 변경 이유와 전후 구조를 설명하고 동의를 받은 뒤 진행한다.
- 설명 항목은 아래 4가지를 최소 포함한다.
- 구현 대상/목표
- 수정 파일 범위
- 호환성 영향(특히 `DialogueGraph`/SaveData)
- 검증 방법

## 6) PLANS 연계 규칙
- 복잡한 작업을 수행해야하는 경우 `ExecPlan`을 사용한다.
- 다음 조건 중 하나라도 충족하면 `ExecPlan`을 사용한다.
- 3개 이상 파일/모듈 수정
- Runtime + Editor 동시 수정
- SaveData/직렬화/에셋 포맷 영향 가능성
- 요구사항이 길거나 단계별 확인이 필요한 작업

- 계획 문서는 리빙 문서로 유지한다.
- 구현 중 사실이 바뀌면 `Assumptions`, `Open Questions`, `Decision Log`, `Discoveries`를 즉시 갱신한다.

## 7) Unreal 5.4 C++ 구현 규칙
- Unreal Reflection은 필요한 범위에만 선언한다.
- UObject 참조는 UE5 권장 포인터(`TObjectPtr`, `TWeakObjectPtr`, `TSoftObjectPtr`)를 용도에 맞게 사용한다.
- 라이프사이클과 유효성(`Initialize/Deinitialize`, `IsValid`, `nullptr` 체크)을 명시적으로 처리한다.
- 이벤트/델리게이트 기반 흐름을 우선하고, 불필요한 Tick 의존을 피한다.
- 하드코딩 문자열보다 `FName`, `FGameplayTag`, 데이터 기반 설정을 우선한다.
- 줄바꿈은 `LF`를 기준으로 한다.
- 함수 구현시 함수명 위에 한글로 주석을 달아 해당 함수의 한 줄 설명을 명시한다.
- 함수 내부에 주석이 있으면 좋을 위치에는 한글로 주석을 달아준다.

## 8) Runtime 유지보수 규칙
- `UDialogueSubsystem` 변경 시 아래 영향을 함께 점검한다.
- 선택지 필터링(`FDialogueChoice::IsPossibleToShow`)
- 분기 노드 평가/다음 노드 탐색
- 이력 저장(`UShownDialogueSaveData`) 및 재로딩
- Portrait preload/caching 및 UI 반영

- 분기 로직 변경 시 "선택지 노출 조건"과 "실제 분기 조건"의 불일치를 금지한다.

## 9) Editor 유지보수 규칙
- `FDialogueGraphEditor` 변경 시 Graph UI <-> RuntimeGraph 변환 양방향을 함께 검증한다.
- 노드/핀 GUID, 연결정보, NodeInfo 복제(`DuplicateObject`) 경로를 깨지 않게 유지한다.
- 툴바 커맨드(`ConvertToDataTable`)는 UI 라벨, 실행 함수, 결과 산출물까지 일관되게 관리한다.

## 10) 데이터/호환성 규칙
- `UDialogueGraph` 또는 런타임 노드 구조 변경 시 역호환 전략을 먼저 정의한다.
- SaveData 구조 변경 시 버전 전략 또는 마이그레이션 경로를 제공한다.
- 기존 샘플 자산 로딩 실패를 유발하는 변경은 금지한다.

## 11) 보고 형식
- 변경 보고에는 다음을 포함한다.
- 변경 파일 목록
- 변경 이유
- 사용자 영향
- 검증 결과
- 미실행 검증과 사유

## 12) 금지 사항
- 사용자 승인 없이 구조 변경을 강행하지 않는다.
- Runtime에 Editor 전용 코드를 넣지 않는다.
- 근거 없는 대규모 리팩터링을 하지 않는다.
- 기존 워크플로(`DialogueGraph` 편집 -> 실행/변환)를 깨는 변경을 하지 않는다.