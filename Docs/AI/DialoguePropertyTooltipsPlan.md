# Task: DialogueProperty 한국어 툴팁

## Goal
DialogueGraph의 Block 속성, 선택 노드 속성 및 중첩 조건/효과/초상화 필드의 이름에 마우스를 올리면 실제 동작에 맞는 한국어 설명을 표시한다.

## Background / Self-Contained Context
DialoguePropertyTabFactory는 UDialogueGraph와 선택 NodeInfo를 각각 IDetailsView로 표시한다. 일반 필드는 UPROPERTY ToolTip 메타데이터를 사용하며 DialogueGraphDetails의 대상/분류/소속은 직접 Slate 위젯을 만든다. 현재 다수 필드는 설명이 없고 구형 설정과 신규 Block 실행 설정이 함께 노출된다.

## Scope / Constraints / Dependencies
- Runtime 공개 헤더의 ToolTip 메타데이터, Editor 커스텀 위젯의 ToolTipText, UserGuide 48절을 수정한다.
- UE 5.4.4 기본 Details 툴팁을 사용한다. 자료구조/저장/실행 조건/공개 API/에셋 바이너리는 변경하지 않는다.
- enum/DataTable ID 선택기 구현은 이번 범위 밖이다. 게임의 ID 약속과 현재 직접 입력 동작을 설명한다.
- 기존 Blueprint.md 논의 기록은 보존하며 이번 커밋에 포함하지 않는다.

## Assumptions / Open Questions
- 표준 속성 이름 hover는 UHT 메타데이터로 처리된다. 커스텀 행은 개별 위젯 툴팁이 필요하다.
- 실제 마우스 hover의 화면 확인 가능 여부는 도구 상태에 달려 있다. 미실행 시 명시한다.

## Uncertainty & PoC Track
메타데이터 문법은 Editor 빌드/UHT로 검증한다. 실패하면 문법만 수정한다. 기본 Details 경로를 유지하므로 별도 UI 시스템 PoC는 필요하지 않다.

## Plan
1. Graph/NodeInfo/중첩 구조체 및 실제 소비 코드 조사, Luna High 읽기 전용 조사 취합.
2. 노출 필드 ToolTip과 커스텀 대상/소속 위젯 툴팁, 안내서 추가.
3. Sol Medium 읽기 전용 의미/누락 검토, Editor 빌드 및 기존 Editor 자동화 검사. 실제 GUI hover는 별도 판정.
4. 기능별 한국어 Subject/Description 커밋, 검증 후 Codex_Develop --no-ff 통합 및 비강제 push.

## Risks & Mitigations
구형 필드가 신규 실행에도 사용되는 것으로 오해될 수 있으므로 실제 소비 경로를 명시한다. 표시 설명은 구현과 대조하고 저장 완료/효과 적용을 보장하는 것으로 표현하지 않는다.

## Design Handoff / Approval Points
기존 속성의 설명 보완이며 미확정 제품 정책 변경이 없다. 사용자 2026-11-17 툴팁 구현 요청으로 승인됨. 새로운 구조/저장 포맷 변경이 필요하면 범위 밖으로 반환한다.

## Rollback Plan
기능 브랜치와 기준 SHA 1fd3a9f를 보존한다. 필요 시 승인된 범위에서 기능 커밋 또는 통합 커밋을 revert한다. 기존 사용자 변경을 reset/restore하지 않는다.

## Backward-compat Matrix
Graph/SaveData/API: 동일 필드·타입·값·실행 의미 유지. 에디터 설명 메타데이터만 추가한다. 변경 diff와 UHT 빌드로 확인한다.

## Progress Log
- [x] Windows Git 기준 확인, feature/dialogue-property-tooltips 생성. 기존 Blueprint.md 변경 보존.
- [x] 툴팁 및 안내서 구현.
- [x] 독립 검토 및 빌드/자동화 검사.
- Git 운영: 아래 검증 결과를 기능 브랜치에 커밋하고 Codex_Develop에 --no-ff 통합 및 비강제 push한다. 실제 SHA와 원격 반영 결과는 Git 이력과 최종 답변에 기록한다.

## Decision Log / Discoveries
Windows 실행은 일반 sandbox에서 WSL socket 오류가 났으며 승인된 실행 권한으로 Windows Git 조회/브랜치 생성에 성공했다. 메인은 구현·Git·빌드만 수행하고 하위는 읽기 전용이다.

## Result
Block/노드/조건/효과/초상화/구형 퀘스트 필드의 한국어 설명과 커스텀 대상·소속·주석 툴팁을 추가했다. 엔진 기본 주석·좌표 및 enum 개별 값의 기본 UI는 유지한다. 엔진 enum의 의미는 부모 속성 설명에서 안내한다.

## Review
- Luna High (`ue_research_scout`): 실제 Details 객체와 중첩 노출 필드 조사.
- Sol Medium (`ue_senior_reviewer`): 설명과 실행 코드의 의미 대조. 기본 후보의 최우선 제외 조건, 구형 초상화 Move만 구현/표정 필드 미사용, FromTranslation 0 의미, 신규 Line에 한정한 시간 설명 지적을 반영했다.
- 독립 검토는 읽기 전용이며 메인만 수정·Windows Git·빌드·테스트를 수행했다. 기존 기능 설명 보완이므로 새로운 제품 설계 인계는 해당 없음.

## Verification
- 최종 문구 보완 후 Editor Build/UHT 성공: Saved/AI/20261117_165538_bf9475b3/Build.log. 기존 EnhancedInput 의존성 선언 및 IncrediBuild 라이선스 경고는 이번 범위에서 변경하지 않았다.
- 1차 Editor Build/UHT 성공: Saved/AI/20261117_165253_cfc28e65/Build.log.
- 기존 기능 회귀 DialogueKit.Editor 11/11, Error 0: Saved/AI/20261117_165344_5d4614fd/Report/index.json 및 result.json. 최종 문구 보완 전 실행이다.
- Windows Git 원본 비교: Runtime 헤더 13개 모두 ToolTip 이외 선언·기본값·다른 메타데이터 불변.
- Windows Git diff --check 통과, UserGuide 목차 48절 링크 및 본문 대조 완료.
- 실제 GUI hover 미검증: 현재 연결 도구에 Windows 에디터 화면 조작/캡처 기능이 없다. 확인 절차: 에디터 재시작 → DialogueGraph 열기 → Block 속성/대상·소속 → Conditions 하위 펼치기 → 각 노드 선택 → 속성명 hover. 설명의 표시·줄바꿈·잘림을 확인한다.
- PIE/패키징/패키지 실행 미실행: 이번 변경은 설명 메타데이터와 Editor 위젯 툴팁으로 한정된다.

## Evidence / Changed Files
공통 접두사 `Plugins/DialogueKit/Source/DialogueKit/Public/`:
- DialogueGraph.h
- DialogueNodeInfo.h, DialogueLineNodeInfo.h
- DialogueChoiceNodeInfo.h, DialogueBranchNodeInfoBase.h, DialogueStateChangeNodeInfo.h, DialogueEndNodeInfo.h
- DialogueConditionTypes.h, DialogueEffectContract.h, DialoguePortraitData.h
- Struct/DialogueStructure.h, Struct/DialogueConditionEvalCriteria.h, QuestBase.h

Editor: `Plugins/DialogueKit/Source/DialogueKitEditor/Private/DialogueGraphDetails.cpp`, `DialogueCommentNode.cpp`.
문서: `Docs/UserGuide.md` 48절, 이 실행계획.
기존 `Blueprint.md` 미커밋 변경은 이번 범위에서 수정·stage하지 않는다.

## Overall Progress
- 이전 S0~S4 기반·실행·저장·선정 구현 및 자동 검증 완료 유지.
- 이전 S5~S7 기술 구현·자동 검증 완료 유지. 실제 GUI/화면/물리 입력/음향·Shipping 실행·제품 검토 및 실제 게임/Prototype2 통합은 남아 있다.
- 이번 S5 보완: DialogueProperty 툴팁 구현, 실제 hover 인수 확인 대기.
- BP-116 enum/DataTable 조건 ID 선택 UI는 별도 미구현 보완 항목으로 유지한다.


구현 커밋: `eb87a24` (기능 코드와 UserGuide 포함). 복구 시 후속 의존성과 사용자 변경을 먼저 확인한다.
