# Task: 대화 필터 단일화 정정

## Goal / 승인
2026-11-19 사용자 정정: 기존/새 필터 병용이 아니라 하나로 통합한다. 후속 답변: 기존 에셋은 재사용하지 않는다. 이전 8b2a8a6의 그룹 추가는 유지하되 구형 독립 필터와 호환 조건 병용을 제거한다.

## 범위 및 결정
- 기준 Codex_Develop 8b2a8a6, 기능 브랜치 feature/unified-dialogue-filter.
- 저작: EntryConditions/Branch/Choice는 RequiredConditions, AnyConditions, BlockedConditions만 제공한다. Combine/Conditions, 구형 Graph 태그/화자/Chapter 검색 필드를 제거한다.
- 실행: 기존 UDialogueBlockLoader의 RequestFromRegistry → 공통 후보 선정/조건 평가 → StartDialogueBlockForTarget을 사용한다. BeginDialogue와 독립 검색/태그 검사/유형 정렬을 제거한다. 기존 재생기 전체 삭제는 하지 않는다.
- 유형은 분류이며 새 Priority/Top/Default 정책과 독립이다. DialoguePriorityWeight는 신규 Priority 원본이므로 Selection에 유지한다.
- 기존 에셋 파일은 삭제하지 않고 이관 도구를 만들지 않는다. Graph custom version이 없는 로드 자료는 영속 retired 표식을 붙여 후보/직접 Block 시작/공통 인사/정확 복원 로드를 차단한다. 재저장과 복제도 표식을 유지한다. 기존 독립 ID 판독은 저장 식별 목적에 한해 남되 실제 로드/실행에서 차단한다.
- Registry payload 3만 조건 자료로 읽으며 1/2는 거부한다. SaveData 스키마 자체는 바꾸지 않는다. 기존 에셋의 의미 보존이나 구버전 플러그인 실행은 지원하지 않는다.
- 삭제 대상은 코드 필드와 실행 경로다. 사용자 콘텐츠 파일·엔진·전역 설정은 수정하지 않는다.

## 진행
- [x] 사용자 의도 정정 및 비재사용 확정, GameDirector 인계·Luna 조사·Sol 검토.
- [x] Runtime/Editor 단일화 및 관련 테스트 갱신.
- [x] Editor 빌드·69개 회귀 테스트·Development 패키징·cooked Registry 1개 검사 완료.
- [x] UserGuide/Blueprint·최종 검토.
- [x] 구현 커밋 확정: Runtime a27ac8b, Editor·UserGuide 0b34130.
- 통합 절차: 검증 기록 커밋 후 Codex_Develop에 --no-ff merge하고 두 브랜치를 비강제 push한다.

## 검증 계획
세 그룹 진리표/오류/빈 목록, 단일 Reflection UI·툴팁·ID 선택·Undo/Redo, 새 에셋 저장 왕복, 구형 Registry 거부, 기존 실제 UAsset의 제외와 재저장/복제 제외 유지, 로더/직접 시작 거부를 확인한다. 전체 DialogueKit 및 cooked Registry 검사를 수행한다. GUI/PIE/패키지 실행 검증은 별도 보고한다.

## 복구
기준 8b2a8a6과 기능 브랜치를 보존한다. 개별 목적 커밋과 --no-ff merge를 사용한다. 이력 재작성·사용자 에셋 복원/삭제는 수행하지 않는다.

## 전체 계획
이전 S0~S4 완료, S5~S7 기술 구현/자동 검증 완료 상태. 이번 단일 필터 정정 진행. 잔여 실제 GUI·물리 입력·음향·Shipping 실행·게임/Prototype2 연결은 별도다.

## 발견 및 수정 이력
- 1차 회귀 Saved/AI/20261119_175523_c18dc7e7: 68/69. 기존 BeginDialogue 문구가 있어야 한다는 오래된 툴팁 기대를 단일 분류 설명으로 수정했다.
- 2차 회귀 Saved/AI/20261119_175739_f8983101: 68/69, 엔진 Error 27. 추가 테스트가 Editor 모듈에서 native tag를 등록해 ensure가 발생했다. Runtime에 등록된 테스트 태그 조회로 수정했다. 실제 구형 에셋 복제본 저장은 구형 노드의 식별 검증에 막혔으므로, 저장 지속성 검사는 격리 복제본의 Graph만 비운 뒤 제외 표식이 보존되는지 검사하도록 분리했다. 원본 파일은 변경하지 않았다.
- Sol 최종 검토에서 개발용 PlayDialogueGraph → StartDialogue 구형 재생 우회를 발견해 동일 retired guard와 테스트를 추가했다.
- 새 형식의 복원용 독립 ID 조회는 유지한다. 구형 ID가 식별되더라도 Loader/Runner가 실제 객체를 거부한다. 프로젝트 cook 경로는 바꾸지 않아 기존 파일이 패키지에 들어갈 수 있으나 실행 대상에서 제외된다.

## 최종 검증 근거
- Editor 빌드 성공: Saved/AI/ConditionPickerBuild_20261119_175854/Build.log. 이후 코드 변경은 설명 주석뿐이다.
- 전체 DialogueKit 69/69, Error 0, 종료 0: Saved/AI/20261119_175919_3a2a775d/Report/index.json 및 result.json.
- Win64 Development build/cook/stage/package/archive 성공, 실행 안 함: Saved/AI/Package_20261119_175956_fb81a060/result.json. 실행 한정 NoXGE 사용, 엔진/전역 설정 변경 없음.
- GameDirector(Astra Medium) 제품 검토, Luna High 소비처 조사, Sol Medium 최종 검토 취합. 마지막 지적(개발 직접 재생 우회/가이드 잔여 문구)을 수정했고 추가 중요 결함 없음.
- 신규 테스트는 세 그룹 단일 UI·삭제 필드 부재·툴팁·Undo/Redo·새 파일 저장 왕복, 실제 old UAsset 식별, 구형 복제/재저장 제외 지속성, 직접 Runner/Subsystem/개발 재생 및 Loader 일반/정확 경로/공통 인사 차단을 검사한다. 표식 저장 시험의 격리 구형 복제본은 원본 노드 식별 검증과 분리하기 위해 Graph만 비웠다.
- 실제 GUI hover, PIE, 물리 입력/음향, 패키지 실행, Shipping 및 모든 게임 Blueprint 재컴파일은 이번 미실행. cook의 콘텐츠 로드 검증과 구분한다. 기존 콘텐츠 .uasset는 변경하지 않았다.

## 변경 파일 범위
- Runtime: DialogueConditionTypes/Library, DialogueGraph, DialogueSubsystem, DialogueBlockMetadata/Loader/Runner, 구형 Registry GameplayTags 선언/정의, 조건/메타데이터/로더/계약 테스트.
- Editor: ConditionSetCustomization, GraphDetails/Validation, BlockPreview, StoryEditor, 조건 그룹/ID 선택/샘플/분기/미리보기/Registry 테스트.
- 문서: UserGuide51 및 관련 안내, Blueprint 정정, 이전 그룹 계획의 정정 표식, 이 실행계획.

- 실제 cooked Registry ProjectCook.DialogueRegistry 1/1, Error 0, 종료 0: Saved/AI/20261119_180053_e7c942a7/Report/index.json.
- Windows Git diff --check 통과. 변경 목록에 .uasset 없음.
