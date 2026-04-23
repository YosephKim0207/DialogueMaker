# ImageProduction

## 참고용 기록 (작업 지침 아님)
- 이 문서는 **현재 코드베이스에 존재하는 연출 구조를 기록한 참고 문서**다.
- 구현 방향/정책 지시는 포함하지 않는다.
- 개선 방향은 아래 `TODO` 섹션에 누적한다.

### 1) 연출 데이터 구조
- `EPortraitActionType`: `None`, `Move`, `Emote`
  - 파일: `Plugins/DialogueKit/Source/DialogueKit/Public/Enum/Portrait.h`
- `FPortraitInitData`: 대화 시작 시 초기 초상화 상태
  - 필드: `Speaker`, `EmoteType`, `PortraitSide`
  - 파일: `Plugins/DialogueKit/Source/DialogueKit/Public/DialogueGraph.h`
- `FPortraitActionData`: 노드 단위 연출 액션
  - 필드: `ActionTargetSpeakerID`, `PortraitActionEmoteType`, `ActionType`, `Delay`, `Duration`, `FromTranslation`, `TargetSide`, `TargetSideOffset`
  - 파일: `Plugins/DialogueKit/Source/DialogueKit/Public/DialoguePortraitData.h`
- `UDialogueNodeInfo`
  - `PortraitData` + `PortraitActionDatas`를 노드 데이터로 보관
  - 접근 함수: `GetPortraitData()`, `GetPortraitActionDatas()`
  - 파일: `Plugins/DialogueKit/Source/DialogueKit/Public/DialogueNodeInfo.h`

### 2) 런타임 연출 흐름 (Subsystem 중심)
- 진입: `UDialogueSubsystem::BeginDialogue()` -> `GetDialogueGraph()` -> `StartDialogue()`
- 시작 처리: `StartDialogue()`에서 `InitializeDialogueData()` 후 `PreloadPortraits()` 실행
- 초상화 프리로드:
  - `PreloadPortraits()`는 그래프 전체 노드를 순회해 `(Speaker, Emote)` 쌍을 수집
  - `UDialoguePortraitData`를 AssetRegistry로 조회 후 `Portrait` 번들 프리로드
  - 프리로드 완료 콜백으로 `CreateDialogueUI()` 호출
- 노드 전환:
  - `ProgressNextDialogue()` -> `OnCurrentDialogueChanged` -> `UpdateCurrentDialogueNode()` -> `SetCurrentDialogueInfo()`
  - `SetCurrentDialogueInfo()`에서 `OnDialogueNodeInfoChanged` 브로드캐스트
- UI/BP에서 노출 가능한 연출 데이터:
  - `GetInitPortraitDatas()`
  - `GetPortraitData()`
  - `GetPortraitActionDatas()`
- 파일: `Plugins/DialogueKit/Source/DialogueKit/Private/DialogueSubsystem.cpp`

### 3) 실제 액션 재생기 (`UActionSequencer`)
- 핵심 특성: **단일 인덱스 기반 순차 실행 구조**
- 초기화/캐시:
  - `Initialize()`로 월드 컨텍스트 저장
  - `AddPortraitWidget()`로 화자별 `UPortraitItemWidget` 캐시
- 실행 시작:
  - `StartSequence(ActionDatas, bIsSkip)`
  - 기존 시퀀스 정리 후 `CachedActionDatas` 저장
  - `bIsSkip == true`이면 `StopSequence()`로 즉시 종료 처리
  - 일반 경로는 `AdvanceToNextAction()`로 다음 액션 실행
- 현재 액션 분기:
  - `AdvanceToNextAction()`의 `switch(ActionType)`에서 실제 구현은 `Move` 중심
  - `Move`: `DelayTimerHandle` -> `PlayCurrentMoveAction()` -> `TickCurrentMoveAction()`
- Move 동작:
  - `TickCurrentMoveAction()`에서 `Alpha` 보간으로 translation 갱신
  - `ResolveTargetTranslation()`에서 `TargetSide`/Anchor/Alignment 기반 목표 좌표 계산
- 중단/스킵:
  - `StopSequence()`가 남은 액션 순회 후 `SkipSequence()`로 종단 상태 정리
- 파일:
  - `Plugins/DialogueKit/Source/DialogueKit/Public/ActionSequencer.h`
  - `Plugins/DialogueKit/Source/DialogueKit/Private/ActionSequencer.cpp`

### 4) 위젯/배치 보조 구성요소
- `UPortraitItemWidget`
  - `SetBaseTranslation()`, `SetEmoteImageTranslation()`, `ResetTranslation()`
  - `NativeOnInitialized()`에서 기본 translation 캐시
  - 파일:
    - `Plugins/DialogueKit/Source/DialogueKit/Public/PortraitItemWidget.h`
    - `Plugins/DialogueKit/Source/DialogueKit/Private/PortraitItemWidget.cpp`
- `UPortraitSubsystem`
  - `EPortraitSide` -> `FAnchors`, `Alignment` 변환 담당
  - 파일:
    - `Plugins/DialogueKit/Source/DialogueKit/Public/PortraitSubsystem.h`
    - `Plugins/DialogueKit/Source/DialogueKit/Private/PortraitSubsystem.cpp`

### 5) 블루프린트 Skip 동작 흐름
- 현재 프로젝트의 Skip 체감 동작은 `UActionSequencer` 단독 로직보다 **BP 호출 체인**의 영향을 크게 받는다.
- 대화 진행 또는 Skip 버튼 입력 시, 위젯 블루프린트에서 `SetTimerByFunctionName`으로 `CreateNextDialogue`를 반복 호출한다.
- `CreateNextDialogue`는 `UDialogueSubsystem::ProgressNextDialogue()`를 호출한다.
- 노드가 갱신되면 `OnDialogueNodeInfoChanged`에 바인딩된 액션 시퀀스 컨트롤 BP가 `UActionSequencer::StartSequence()`를 다시 호출한다.
- `StartSequence()`는 시작 시점에 `StopSequence()`를 먼저 호출하므로, 기존에 진행 중이던 액션은 `StopSequence()` -> `SkipSequence()` 경로로 즉시 종결 처리된다.
- 이 구조 때문에 `Move`가 내부적으로 Tick 기반으로 진행되더라도, 실제 플레이 체감은 “다음 대사/Skip 입력 시 현재 액션이 끊기고 다음 상태로 넘어가는” 형태가 된다.

## TODO
- TODO 항목은 `IP-TODO-###` 형식의 고유 ID를 사용한다.
- 상태는 `Planned | In Progress | Done | Deferred` 중 하나로 기록한다.
- 항목 작성 템플릿:
  - `- [ ] IP-TODO-001 | 상태: Planned | 요약: <작업 요약> | 근거: <배경/이유>`
- 진행 중 방향 변경이 생기면 상태/요약/근거를 즉시 갱신한다.
- 완료된 항목은 체크박스를 체크하고 상태를 `Done`으로 변경한다.

### TODO Items
- (아직 등록된 TODO 없음)
