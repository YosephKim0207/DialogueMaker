# Task: 필수·대안·금지 조건과 대화 유형 통합

## Goal / Background
2026-11-19 사용자 요청으로 새 조건 시스템에 필수·대안·금지 동시 적용과 스토리/퀘스트/일상 분류, 각 항목 한국어 툴팁을 제공한다. 이전 단일 All/Any는 A AND (B OR C) AND NOT D를 표현하지 못했다.

## Self-Contained Context
- 기준 Codex_Develop e2009ca. 기능 브랜치 feature/dialogue-condition-groups.
- 기존 Blueprint.md 미커밋 11줄은 직전 대화에서 총괄이 작성한 관련 논의 기록이며 보존한다.
- Runtime의 FDialogueConditionSet.Combine/Conditions는 Block Entry, Branch, Choice와 메타데이터에 공통 사용된다. EvaluateConditions가 공통 평가기다.
- Editor의 DialogueGraphValidation과 DialogueBlockPreview는 Conditions 배열을 직접 소비하므로 세 그룹을 함께 처리해야 한다.
- DialogueGraphType은 기존 Quest=0/Story=10/DailyChat=100 enum 필드다. 기존 값/이름을 유지하고 새 Selection 분류로 노출한다.

## Scope / Constraints / Dependencies
In: 빈 배열 기본의 RequiredConditions/AnyConditions/BlockedConditions 추가, 기존 그룹 AND 새 세 그룹 평가, 호환 조건을 명시하는 Details, 메타데이터 버전 호환, 검사/미리보기, 유형 표시/툴팁, 테스트·가이드.
Out: 구형 BeginDialogue 제거, 기존 태그 자동 변환·삭제, 임의 중첩 그룹, 유형별 게임 정책, 기존 에셋/SaveGame 재작성.
UE5.4.4 및 기존 모듈을 사용하며 메인만 수정·검증·Git을 실행한다.

## Design Handoff / Approval
사용자가 두 기능과 설명 툴팁의 구현을 승인했다. GameDirector 인계: 세 그룹은 동시에 적용하고 빈 그룹은 제한하지 않는다. 기존 Combine/Conditions는 의미를 유지하며 값이 있으면 함께 적용됨을 UI에 알린다. 필수/대안 오류는 기존 True 대체, 금지 오류는 차단하지 않으며 정상 true인 다른 금지는 여전히 차단한다. Branch 전체 빈 조건의 진단 유지. 유형은 새 우선순위와 독립이다. 기존 중첩 보류는 임의 중첩에 대해서 유지한다.

## Assumptions / Open Questions
- 기존 enum 숫자/필드 유지로 유형 데이터 이관은 필요 없다.
- 새 그룹 에셋은 이전 플러그인 바이너리로 실행하지 않는다. 새 바이너리는 기존 자산을 읽는다.
- 실제 Windows GUI 조작 도구가 없어 Slate 자동 검사와 실제 사용자 화면 검증은 구분한다.

## Uncertainty & PoC Track
프로퍼티 Details에서 기존 그룹 노출과 중첩 Key 선택기, UObject 파일/Registry 텍스트 호환을 실제 자동 검사한다. 빌드·검사가 실패하면 구현을 수정하고 기능 완료로 표시하지 않는다. 위험한 기존 에셋 변환은 시도하지 않는다.

## Plan
1. Runtime 조건 구조/공통 평가기/메타데이터 버전 및 유형 툴팁: 조합·오류·구자료 테스트.
2. Editor 조건 Details와 미리보기·validation: 기존 조건 안내, 새 그룹 인식, Undo/Redo 및 저장 왕복 검사.
3. 전체 DialogueKit 회귀, UserGuide51·Blueprint·검증 기록, 목적별 한국어 커밋과 --no-ff 통합/push.

## Backward-compat Matrix
| 표면 | 전략 | 검증 |
|---|---|---|
| 기존 Combine/Conditions | 이름·기본값·종전 의미 유지, 새 배열 빈 기본 | 기존 테스트·구형 텍스트 역직렬화 |
| 신규 세 그룹 | 새 필드 추가, 기존 그룹 결과와 AND | 조합/오류/왕복 |
| Registry | 새 writer 버전2, reader 1/2 허용, Registry 키 이름 유지 | 양 버전 로드·정확 복원 ID |
| SaveData | 형식 변경 없음 | 전체 저장 회귀 |
| DialogueGraphType | 기존 enum/값/필드 재사용(선언은 Enum/DialogueGraphType.h), 분류와 선정 독립 | 유형별 우선순위 불변·파일 왕복 |

## Risks & Mitigations
금지 조건의 오류를 단순 True로 반전하면 부당 차단되므로 정상 평가 여부를 구분한다. 기존 invalid Combine 오류는 기존 그룹만 통과시키고 정상 새 그룹의 차단을 무시하지 않는다. 새 그룹만 있는 Branch/Choice를 빈 조건으로 오판하지 않도록 공통 IsEmpty를 사용한다. 메타데이터 v2는 구 바이너리에서 조용히 그룹을 무시하는 대신 거부한다.

## Rollback Plan
복구 기준 e2009ca 및 기능 브랜치를 보존한다. 기능 merge를 명시적 revert할 때 신규 그룹으로 작성된 자산을 이전 실행기로 실행하면 안 되므로 관련 콘텐츠도 호환 버전으로 함께 관리한다. 기존 파일 삭제/덮어쓰기는 자동 수행하지 않는다.

## Progress Log
- [x] 요청·제품 인계·기능 브랜치.
- [x] 구현·빌드.
- [x] 검증·가이드·최종 검토.
- [x] 커밋·통합 대상 확정: Runtime 72f5dea, Editor·UserGuide 2d77714.
- 통합 절차: 이 검증 기록을 기능 브랜치에 커밋한 뒤 Codex_Develop에 --no-ff merge하고 두 브랜치를 비강제 push한다. 복구 기준 e2009ca와 기능 브랜치를 보존한다.

## Decision Log / Discoveries
- 호환 그룹은 세 그룹과 별도의 옛 필터가 아니라 동일 평가기 안에서 보존되는 기존 식이다.
- 현재 유형 정렬은 구형 BeginDialogue에만 남으며 이번 새 분류 도입은 그 API의 동작을 변경하지 않는다.

## Result / Verification / Evidence
- Editor 최종 Build 성공: Saved/AI/ConditionPickerBuild_20261119_155301/Build.log. 실행 한정 -NoXGE 및 -gather 사용, 엔진/전역 설정 변경 없음.
- DialogueKit 69/69 성공, Error 0, 종료 0: Saved/AI/20261119_155411_781c29f6/Report/index.json 및 result.json.
- Win64 Development build/cook/stage/package/archive 성공(실행 안 함): Saved/AI/Package_20261119_155457_9401f969/result.json. 기존 래퍼를 Saved/AI에 복사한 실행 한정 -noxge 인자로 실행. 412개 패키지 cook, 새 exe/pak 산출물 확인. 기존 Python 노출 이름 DialogueStoryScope 경고 1개와 EnhancedInput 의존 선언 경고는 기존 항목이며 변경하지 않았다.
- 실제 cooked Registry 검사 ProjectCook.DialogueRegistry 1/1·Error 0: Saved/AI/20261119_155558_851e04ac/Report/index.json.
- 신규 검사: 16개 진리 조합, 빈/오류/기존 조건 병용, 새 필드 없는 v1 fixture, v2 그룹·유형 왕복, v1 잘못된 신규 그룹 거부, v2 손상 유형 거부, v1 유형 3종/누락/손상, v2 정확 복원 ID, 서로 다른 유형보다 Priority가 우선함, 기존 실제 에셋 로드, 실제 Details 그룹/호환 안내·툴팁·ID 선택·Undo/Redo·새 파일 저장/언로드/재로드·미리보기 모든 그룹 설명.
- 기존 Branch/Choice 실행 테스트의 조건 fixture를 새 Required/Any 그룹으로 바꾸어 실행 중 재평가/기본 선택지/빈 Branch를 검증했다. 기존 단일 All/Any와 직렬화의 호환 테스트는 그대로 통과한다.
- Luna High의 직접 소비처 조사, Sol Medium의 설계·최종 검토, GameDirector(Astra Medium)의 인계/제품 검토를 취합했다. 지적된 버전 오표기·유형 fallback·이전 가이드 잔여 문구를 수정했다. 최종 추가 수정 요구 없음.
- 실제 GUI hover/마우스·키보드 가독성, 실제 PIE, 패키지 실행, Shipping, 게임별 전체 Blueprint 연동/Prototype2는 이번 미검증. Cook에 포함된 콘텐츠 로드는 통과했지만 모든 게임 Blueprint를 재컴파일한 검사는 아니다. 엔진 내장 테스트는 이번 별도 실행하지 않았다.
- 기존 .uasset 및 게임 저장 파일은 변경하지 않았다. 새 구조를 작성한 콘텐츠의 구버전 플러그인 downgrade는 지원하지 않는다.

- 조사 후 추가: Storyboard의 별도 metadata import도 v1/v2 판독과 version 누락 거부로 갱신했다. v1 분류는 원래 DialogueGraphType Registry 태그에서 보완하고 missing/invalid는 unknown으로 표시한다. v2의 손상 유형과 v1에 신규 그룹이 잘못 포함된 자료는 후보 유효성에서 거부한다.
- 최초 전체 회귀 20261119_155023_a1799d82: 69개 중 68 성공, ChoiceFilteringAndLoop 1 실패. 새 Any 그룹 테스트로 변경한 시나리오에서 옵션 삭제 후 index0 초기화가 옛 Conditions에 남은 테스트 fixture 문제였으며 해당 시나리오의 모든 초기화/추가 대상을 Any로 맞췄다. 제품 평가 실패가 아님을 원본 호출/상태로 확인했다.

## 변경 파일 범위
- Runtime: Public/DialogueConditionTypes.h, Private/DialogueConditionTypes.cpp, Private/DialogueConditionLibrary.cpp, Public/DialogueBlockMetadata.h, Private/DialogueBlockMetadata.cpp, Public/DialogueGraph.h, Private/DialogueGraph.cpp, 신규 Public/Enum/DialogueGraphType.h, 신규 Private/Tests/DialogueConditionGroupsTests.cpp.
- Editor: 신규 Public/DialogueConditionSetCustomization.h 및 Private/DialogueConditionSetCustomization.cpp, Private/DialogueKitEditor.cpp, DialogueBlockPreview.cpp, DialogueGraphValidation.cpp, DialogueStoryEditor.cpp, DialogueStoryboard.cpp, 신규 Tests/DialogueConditionGroupsEditorTests.cpp, 기존 Tests/DialogueGraphFlowTests.cpp·DialogueValidationTests.cpp·DialogueCookRegistryTests.cpp.
- 문서: UserGuide 51절/관련 이전 안내, Blueprint 승인·완료 기록, 이 계획.

## 전체 계획 진행
- 이전 완료: S0~S4, 툴팁·정의 샘플·ID 검색 선택기. S5~S7 기술 구현 및 자동 검증 상태 유지.
- 이번 완료: 필수·대안·금지 그룹과 기존 조건 호환 UI, 유형 표시·스토리보드 검색·툴팁, 자동 테스트와 Development 패키징.
- 잔여: S5 실제 GUI, S6/S7 실제 화면·물리 입력·음향, Shipping 실행 및 실제 게임/Prototype2 연결. 구형 시작 API/태그 필터 자동 이관 또는 제거는 이번 승인 범위 밖이며 별도 통합 작업이다.
