# Task: 경유점과 Line 표시 및 조건 태그 선택 안내

## Goal / Scope
2026-10-16 사용자 요청: 경유점 연결/미연결 핀 색, Line 제목·화자·대사 표시, Branch/Choice 복수 태그 설정 방법 확인. 기존 데이터와 런타임 평가 의미를 보존한다.

## Self-Contained Context
기준 Codex_Develop 45cb599. DialogueKitEditor의 경유점은 SGraphNodeKnot를 사용하여 일반 핀 factory를 우회하며, 기본 schema 색은 검정이다. UDialogueFlowEdGraphNode 제목은 Line 상수이고 본문 위젯이 없다. 조건은 FDialogueConditionSet.Conditions 배열의 Key.Tag를 사용하며 FGameplayTag 표준 선택 UI가 적용된다. Branch의 구형 조건은 읽기 전용이다.

## Plan
1. fix/dialogue-reroute-pin-color: schema 실행 핀 색 제공, 기존 knot 드래그 유지. Editor 빌드와 LayoutRoundtrip 검증 후 작은 커밋/통합.
2. fix/dialogue-line-node-content: Line 전용 Slate 본문과 실제 제목 반영. 파생 Choice/StateChange 표현 보존. Editor 빌드와 관련 회귀 검증 후 통합.
3. 기존 태그 선택 경로 및 All/Any 복수 조건을 사용자 문서에 설명한다. 기존 선택기가 없다는 추가 근거가 있으면 원인을 재검토한다.

## Constraints / Compatibility
기존 에셋·저장·ID·실행 자료구조 및 CSV 변경 없음. 편집 표시만 수정. 게임 상태 평가와 태그 등록은 기존 게임 책임. 기존 줄바꿈과 사용자 코드 스타일 유지.

## Assumptions / Open Questions / PoC
기존 표준 태그 UI가 등록되어 있는지 읽기 전용 검토로 확인한다. 실제 GUI 자동 조작 수단이 없어 화면 관찰은 미검증으로 명시한다. 엔진 로컬 코드·빌드·자동화로 표시 바인딩과 연결/저장 왕복을 확인한다.

## Design Handoff / Approval
사용자가 직접 요청한 기존 저작 기능의 표시 결함 수정. 신규 미확정 제품 설계 없음. Sol Medium 2명 읽기 전용 검토 후 메인만 구현한다.

## Risks / Rollback
긴 대사로 가로폭이 커지지 않도록 본문 폭과 줄바꿈을 제한한다. Line 전용 위젯을 다른 파생 노드에 적용하지 않는다. 각 기능 merge를 git revert -m 1로 복구하며 이력과 기존 파일을 폐기하지 않는다.

## Progress / Verification
- 조사 진행. 빌드·자동화 결과와 사용자 문서는 기능 완료 시 기록한다.

- 경유점: Editor 빌드 20261016_171749_5f381d8e 성공, LayoutRoundtrip 20261016_171834_9a0ef154 1/1·Error 0. 이전 축약 필터 Layout 실행은 두 테스트 성공·Error 0이나 UnexpectedTests로 래퍼 실패(171814_5d80f9c6), 정확 이름으로 수정. 실제 색감 GUI 미검증.

## Result / Evidence
- Line: 제목 막대에 Title(빈 경우 Line), 본문에 Speaker ID와 Dialogue Text를 동적 표시. 본문 폭 300에서 줄바꿈. Choice/StateChange 기본 위젯과 Reroute 전용 위젯 유지.
- 태그: Sol Medium 검토에서 UE GameplayTagsEditor의 FGameplayTag customization과 Key.Tag 필드를 확인. 기존 선택 경로를 UserGuide 36절에 명시했으며 자료형/평가 코드는 변경하지 않았다. 기본 선택지와 Legacy에서는 조건 선택이 없는 이유도 안내했다.
- Editor Build: Saved/AI/20261016_172053_b5afd7b9/Build.log 성공.
- DialogueKit.Editor: Saved/AI/20261016_172118_f4ad099f/result.json 10/10·Error 0. 신규 LineNodeContent는 실제 Slate 트리에서 기존 위젯의 화자/본문 갱신과 파생 노드 분리, 경유점 색 조회를 확인. 기존 배치 파일 왕복·Undo·검색·검사 회귀도 통과.
- 실제 GUI 색감/긴 본문 배치/태그 picker 클릭은 도구 미연결로 미검증. 이번 Editor 전용 수정에서 PIE/패키징은 재실행하지 않았다. 기존 EnhancedInput 의존성 선언·XGE·Git safe.directory 경고는 변경하지 않았다.
- 전체 계획: S0~S7 구현 상태는 유지. S5~S7 실제 GUI/화면/입력/음향·Shipping 실행과 최종 제품 인수, Prototype2 통합은 별도 잔여 항목이다.
