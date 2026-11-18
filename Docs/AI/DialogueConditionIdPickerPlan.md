# Task: 게임 정의 기반 조건 ID 선택기

## Goal / Background
NPC DataAsset과 Item/Skill DataTable을 공통 정의 제공자로 읽어 DialogueGraph의 Target ID와 Item ID를 검색·선택한다. 이전 샘플 구현은 원본과 어댑터만 제공했고 ID 입력 UI가 연결되지 않았다.

## Self-Contained Context
- 기준 Codex_Develop d4fa6d0, 작업 feature/dialogue-condition-id-picker.
- Runtime/Public/DialogueStateProvider.h의 IDialogueDefinitionProvider가 FDialogueStateDefinition(Key, DisplayName) 배열을 반환한다.
- /DialogueKit/Sample/GameDefinitions/DA_SampleDefinitionCatalog는 NPC 2개, Item 6개, Skill 6개를 14개 키로 변환한다.
- 기존 FDialogueStateKey는 Kind, TargetId, ItemId, Tag를 저장한다. 이 형식과 런타임 평가 코드는 유지한다.

## Scope / Constraints / Dependencies
- In: UDialogueSettings의 선택 원본 설정, Editor 공통 Key Details, 프로젝트 샘플 설정, 자동 검사와 가이드.
- Out: 게임 정책, 현재 상태 제공자 자동 교체, 기존 에셋 변경, 스키마 변경, 프로젝트 전체 원본 자동 수집.
- UE5.4.4, 기존 PropertyEditor/Slate 모듈 사용. 플러그인 기본 원본은 비움. 게임은 에셋으로 읽을 수 있는 IDialogueDefinitionProvider 어댑터를 구현한다.

## Assumptions / Open Questions
- Target 표시 이름은 공통 계약에 없으므로 raw ID를 표시한다. Item은 DisplayName과 ID를 함께 표시한다.
- 정의 원본은 에디터에서 월드 없이 읽을 수 있어야 한다. Blueprint 제공자도 interface execute로 호출한다.
- 실제 GUI 입력 관찰은 사용 가능한 도구와 구분하여 보고한다.

## Uncertainty & PoC Track
- UE property handle 편집과 자동 transaction/undo, MustImplement 필터를 엔진 소스로 확인하고 실제 Details 자동 검사로 검증한다.
- 실패 시 에셋 저장을 진행하지 않고 편집 경로를 수정한다. 빌드와 실제 handle 검사가 통과하면 본 구현으로 유지한다.

## Plan
1. Runtime/Public/DialogueSettings.h와 Config/DefaultDialogueKit.ini에 원본 선택을 추가한다. 빌드와 설정 로드로 검증한다.
2. Editor의 DialogueStateKeyCustomization 헤더/구현과 모듈 등록을 추가한다. Kind별 Target, Kind+Target별 Item, 검색, 새로고침, 오류/미등록 안내, 고급 직접 입력을 구현한다. 기존 키는 명시적 선택 외에 변경하지 않는다.
3. Editor/Private/Tests에 원본/필터/실제 Details 편집·Undo/Redo·파일 왕복 검사를 추가한다. Editor 빌드와 DialogueKit 전체 테스트를 실행한다.
4. UserGuide 50절 및 Blueprint에 실제 사용 절차·검증 한계를 기록하고 기능 커밋 후 Codex_Develop에 --no-ff 통합·push한다.

## Risks & Mitigations
동기 로드는 메뉴 열기/새로고침/선택 직전 재검증 때만 수행한다. 초기 Details에서는 미확인 안내와 저장된 ID를 표시한다. 원본 객체는 호출 중 strong 참조로 보호하고 값 배열만 캐시한다. 오류·중복은 목록 전체를 거부하며 기존 키를 보존한다. 편집 가능 여부는 위젯과 선택 콜백 모두 검사한다. 공통 Key 사용처(Entry/Branch/Choice/Effects/샘플/정의 편집)의 고급 직접 입력도 유지한다.

## Design Handoff / Approval Points
- BP-116, 2026-11-18 사용자 요청: DataAsset NPC ID와 DataTable Item ID를 DialogueGraph 드롭다운에서 선택하도록 구현.
- GameDirector 인계 완료: 필터/검색/기존 값 보존/Tag 기존 선택기/오류 안내/Undo·저장/가이드가 완료 기준이다.
- 저장 계약·런타임 의미·게임 정책 변경이 필요해지면 해당 부분만 반환한다. 현재 범위는 이미 구현 요청됨.

## Backward-compat Matrix
| 표면 | 기존 / 이후 | 검증 |
|---|---|---|
| Graph/Key/SaveData | 직렬화와 FName 그대로, UI만 목록 선택 | 기존 전체 테스트와 새 저장 왕복 |
| 런타임 평가 | 동일 게임 키를 동일 제공자에 전달 | 기존 조건 테스트 |
| 미등록 ID/비활성 필드 | 자동 삭제/정규화 없음 | Kind/Target 변경 및 원본 실패 검사 |
| 배포 플러그인 | 원본 기본 null, 샘플 지정은 호스트 config | 설정/소스 검사 |

## Rollback Plan
기능 커밋을 보존하고 명시적 revert로 코드·문서·프로젝트 설정을 함께 복구한다. 기존 에셋은 수정하지 않는다. 데이터 소실 작업은 별도 승인 정책을 따른다.

## Progress Log
- [x] 조사·제품 인계, 기능 브랜치 생성.
- [x] 구현·빌드.
- [x] 자동 검사·가이드·검토.
- [x] 기능 커밋 및 통합 대상 확정: 6988061. 아래 통합 절차를 적용한다.

## Decision Log
- 공통 인터페이스 원본 1개를 설정한다. 임의 게임 DataTable 행 구조를 추측하지 않고 Catalog/게임 어댑터가 변환한다.
- 직접 입력은 UE 기본 접기 그룹의 고급 행으로 제공하여 소스가 없어도 접근 가능하게 한다.

## Discoveries
- 엔진 IPropertyUtilities가 Details 편집 잠금 여부를 제공한다.
- 기존 전역 Key 사용처 전체에 공통 UI가 적용된다. Block 대화 대상 GameplayTag와 조건 Target ID는 별도다.

## Result / Verification / Evidence
- UI 구현 완료, Editor Win64 Development 빌드 성공. 기존 XGE 초기화 문제를 피한 실행 한정 -NoXGE, 신규 소스 목록 갱신 -gather를 사용했으며 엔진/전역 설정은 바꾸지 않았다.
- 최종 빌드: Saved/AI/ConditionPickerBuild_20261118_173353/Build.log.
- 최종 전체 DialogueKit 66/66, 종료 0, LogErrorLines=0: Saved/AI/20261118_173417_d114a95a/Report/index.json 및 result.json.
- 신규 2개 검사: 실제 설정 원본 로드·native catalog 필터 메타데이터·14개 목록·Kind/Target 필터·표시 이름/ID 검색·전체 목록 오류 원자성, 실제 Details 생성·고급 그룹·선택·1회 Undo/Redo·미등록 값 보존·편집 delegate 잠금·SIE 플래그 잠금·다중 Kind/Target 거부·실제 새 Graph 파일 저장/언로드/재로드.
- SIE 플래그 검사는 실제 PIE 생명주기/게임 실행 검증이 아니다. 엔진 내장 테스트는 이번 별도 미실행. 전체 회귀에는 기존 조건·Graph/노드·샘플·저장 검사가 포함된다.
- 실제 GUI 마우스/키보드·화면 배치·PIE·패키징·패키지 실행·실게임/Prototype2 연결은 이번 미검증. 현재 로컬 Windows GUI 조작 도구가 없어 NullRHI Slate 검사와 구분한다. 미로드 Blueprint 전용 인터페이스 자산 필터는 엔진 제한을 UserGuide에 안내했고 실사용 검증은 남는다.
- ue_research_scout(Luna High) 엔진 API 조사, ue_senior_reviewer(Sol Medium) 구현 검토, game_director(Astra Medium) 제품 일치 검토를 읽기 전용으로 취합했다. AllowedClasses 인터페이스 미지원이라는 초기 검토 가설은 엔진 코드 및 실제 helper 검사로 철회했다. 최종 차단 결함 없음. Director가 지적한 UserGuide48의 직접 입력 전용 설명도 수정했다.
- 최초 실패 보존: DefinitionSamplesBuild_20261118_172733(children builder advanced 인자), 172927(키 해시 미export 링크), ConditionPickerBuild_20261118_173212(SIE bitfield/SetObjects 테스트 타입)에서 빌드 실패 후 수정. 첫 테스트 20261118_173037_bd0b6a14는 Details 검사 성공 뒤 Options 테스트 데이터의 TArray 자기 참조 Add로 assert/보고서 없음; 복사본 추가로 수정했다. 최종 전체 성공이 이 실패들을 대체하며 실패 로그는 보존한다.
- 고유 시험 마운트는 엔진 지연 검사 수명 때문에 프로세스 종료까지 유지하고 Saved/AI 아래 시험 파일을 증거로 보존한다. 기존 에셋/사용자 저장 파일은 변경하지 않았다.

- PoC 빌드에서 IDetailChildrenBuilder는 AddCustomRow의 advanced 인자를 지원하지 않음을 확인했다. 기본 AddGroup으로 고급 직접 입력을 제공한다. 목록 필터에 AllowedClasses 인터페이스 메타데이터를 사용하고 값 설정 검증의 MustImplement도 함께 유지한다.

- 기존 FDialogueStateKey의 GetTypeHash에 DIALOGUEKIT_API export만 추가해 Editor 모듈의 중복 집합 검증을 연결했다. 해시 계산/키 필드/직렬화 변경은 없다.

## 변경 파일
- Config/DefaultDialogueKit.ini
- Plugins/DialogueKit/Source/DialogueKit/Public/DialogueSettings.h
- Plugins/DialogueKit/Source/DialogueKit/Public/DialogueConditionTypes.h
- Plugins/DialogueKit/Source/DialogueKitEditor/Public/DialogueStateKeyCustomization.h
- Plugins/DialogueKit/Source/DialogueKitEditor/Private/DialogueStateKeyCustomization.cpp
- Plugins/DialogueKit/Source/DialogueKitEditor/Private/DialogueKitEditor.cpp
- Plugins/DialogueKit/Source/DialogueKitEditor/Private/Tests/DialogueConditionIdPickerTests.cpp
- Docs/UserGuide.md, Blueprint.md, 이 계획서.

## 전체 진행 기준
- 이전 완료: S0~S4, 툴팁, 정의 샘플. S5~S7 기술 구현·자동 검증 완료는 유지하되 전체 Exit Verified는 아니다.
- 이번 완료: BP-116 정의 목록 기반 ID 선택 UI와 샘플 연결·자동 검사·가이드.
- 잔여: S5 실제 GUI, S6/S7 실제 화면·물리 입력·음향, Shipping 실행, 실제 게임/Prototype2 통합. 이번 선택기의 실제 화면/입력과 Blueprint 전용 소스 확인도 남는다.

## Git 통합 절차
기능 코드·테스트·UserGuide 커밋은 6988061이다. 이 결과 기록을 별도 한국어 커밋으로 남긴 뒤 Codex_Develop에 --no-ff merge하고 기능 브랜치와 Codex_Develop을 비강제 push한다. 최종 Git 결과와 merge SHA는 채팅 완료 보고에 남긴다. 기능 브랜치와 복구 기준 d4fa6d0은 보존한다.
