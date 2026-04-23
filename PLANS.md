# PLANS.md

복잡한 작업(다중 시스템 수정, 저장 포맷 영향, 그래프/런타임 동시 수정, 디버깅 장기화)은 구현 전에 실행계획(Execution Plan)을 작성한다.

참고: https://developers.openai.com/cookbook/articles/codex_exec_plans/

## 언제 계획을 작성하는가
- 3개 이상 파일/모듈을 수정할 때
- 데이터 구조 또는 저장 포맷이 바뀔 가능성이 있을 때
- `DialogueKit` 런타임과 `DialogueKitEditor`를 동시에 수정할 때
- 요구사항이 길거나 모호하여 단계적 검증이 필요할 때

## 작성 원칙
- 계획은 구현 가능한 최소 단계로 쪼갠다.
- 각 단계는 검증 가능해야 하며 완료 기준을 명시한다.
- "무엇을 왜 바꾸는지"를 먼저 쓰고 "어떻게 바꿀지"를 뒤에 쓴다.
- 진행 중 새로운 사실이 나오면 계획을 즉시 갱신한다.
- 큰 변경은 중간 결과를 사용자에게 공유하고 승인 포인트를 둔다.
- 계획 문서는 작업 중 계속 갱신하는 리빙 문서로 유지한다.
- 불확실성은 숨기지 말고 `Open Questions`에 즉시 기록한다.
- 단계별 검증 명령 또는 검증 절차를 실행 가능한 형태로 작성한다.
- 실패 시 복구 방법(롤백/우회)을 `Rollback Plan`에 반드시 기록한다.
- 계획 문서는 완전 자급자족(self-contained) 상태를 유지한다.
- 외부 문서를 열지 않아도 실행 가능하도록 파일 경로, 현재 상태, 의사결정 근거, 검증 방법을 계획 내부에 명시한다.
- 불확실성이 남아있으면 본 구현 전에 PoC/토이 구현 단계를 Plan에 기본으로 포함한다.
- PoC 단계는 성공/실패 기준과 폐기 조건(언제 본 구현으로 넘어가는지)을 반드시 갖는다.
- 구조/저장 포맷/조건 분기 변경이 포함되면 `Backward-compat Matrix`를 필수로 작성한다.
- 연출 관련 작업이면 `ImageProduction.md`의 `TODO` 섹션을 반드시 참조한다.
- 연출 관련 의사결정은 `Decision Log`에 TODO ID와 함께 기록한다.

## 실행계획 템플릿
아래 형식을 복사해 작업 시작 전에 채운다.

```md
# Task: <작업명>

## Goal
- <최종 산출물/성공 조건>

## Background
- <요청 배경/현재 동작>

## Self-Contained Context
- <현재 코드/에셋 상태 요약>
- <핵심 파일 경로와 현재 동작>
- <이 계획만 읽어도 작업 가능한 수준의 전제/용어 정의>

## Scope
- In: <수정 범위>
- Out: <이번 작업에서 제외>

## Constraints
- <엔진/플러그인/호환성/성능 제약>

## Dependencies
- <선행 조건/의존 모듈/필요 에셋>

## Assumptions
- <현재 가정>

## Open Questions
- <확인 필요 사항>

## ImageProduction TODO Link (연출 작업인 경우 필수)
- Referenced TODO IDs: <참조한 TODO ID 목록>
- Applied in this task: <이번 작업에서 반영한 TODO ID 목록>
- Not applied + reason: <미반영 TODO 및 사유>
- New TODO candidates: <새로 발견한 TODO 후보>

## Uncertainty & PoC Track
- Uncertainty: <무엇이 불확실한가>
- PoC Scope: <검증만을 위한 최소 구현 범위>
- PoC Steps: <짧은 실험 단계>
- PoC Success Criteria: <성공 기준>
- PoC Failure Signal: <실패 판단 기준>
- Exit Rule: <본 구현으로 전환/중단 조건>

## Plan
1. <단계 1>
   - Files: <파일 경로>
   - Change: <변경 내용>
   - Validation: <검증 방법>
2. <단계 2>
   - Files: <파일 경로>
   - Change: <변경 내용>
   - Validation: <검증 방법>
3. <단계 3>
   - Files: <파일 경로>
   - Change: <변경 내용>
   - Validation: <검증 방법>

## Risks & Mitigations
- Risk: <위험>
- Mitigation: <완화 방법>

## Approval Points
- <구조 변경/데이터 포맷 변경 전 사용자 확인 항목>

## Rollback Plan
- <문제 발생 시 되돌리는 절차>

## Backward-compat Matrix
- Surface: <예: DialogueGraph Asset 직렬화>
- Old Behavior/Data: <기존 동작/포맷>
- New Behavior/Data: <변경 동작/포맷>
- Compatibility Strategy: <버전 필드/마이그레이션/기본값/가드>
- Verification: <구버전 자산/세이브 재현 검증 방법>

## Progress Log
- [ ] Step 1
- [ ] Step 2
- [ ] Step 3
- [ ] ImageProduction TODO 정합성 확인(연출 작업인 경우)

## Decision Log
- <중요 의사결정과 이유>

## Discoveries
- <진행 중 새로 확인된 사실>

## Result
- <완료 후 요약>
- <남은 이슈/후속 작업>

## Verification
- <실행한 테스트/검증 결과>

## Evidence
- <로그/스크린샷/재현 절차/관련 커밋 또는 변경 파일>
```

## DialogueMaker 특화 체크포인트
- `DialogueGraph` 에셋 직렬화/역직렬화 영향 확인
- `Conver to Sheet` CSV 변환 경로/포맷 영향 확인
- 선택 이력/호감도/능력치/아이템 조건 분기의 역호환 확인
- 에디터 커맨드(`DialogueGraphEditorCommands`)와 런타임(`DialogueSubsystem`) 연결 영향 확인
- 연출 작업인 경우 `ImageProduction.md TODO` 참조/반영 여부 확인

## 완성도 점검 체크리스트
- Goal/Scope/Out-of-Scope가 분리되어 있는가
- Approval Points가 구조 변경 지점을 명시하는가
- Plan 각 단계에 Files/Change/Validation이 모두 있는가
- Self-Contained Context가 외부 문서 없이 실행 가능 수준인가
- 불확실성 항목마다 PoC/토이 구현 단계와 종료 조건이 정의되어 있는가
- Rollback Plan이 실제 수행 가능한 절차인가
- Backward-compat Matrix가 영향 표면별로 작성되어 있는가
- Verification/Evidence가 추적 가능한 형태인가
