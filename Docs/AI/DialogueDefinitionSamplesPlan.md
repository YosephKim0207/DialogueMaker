# Task: 게임 정의 연동 Sample DataAsset·DataTable

## Goal / Background
게임과 DialogueMaker의 책임 분리를 게임 개발자가 실제 에셋과 코드로 확인할 수 있게 한다. NPC 정의 DataAsset, 아이템·스킬 DataTable, 이를 공통 조건 정의로 바꾸는 샘플 연결 에셋을 /DialogueKit/Sample/GameDefinitions에 제공한다. 사용자 2026-11-18 요청은 샘플 구현 승인이다. 게임의 스킬 동작·성장 정책을 정하지 않는다.

## Self-Contained Context
기존 IDialogueDefinitionProvider는 GetDialogueDefinitions로 FDialogueStateDefinition 목록을 반환한다. 상태는 IDialogueStateProvider가 별도로 조회한다. UDialogueSampleStateProvider는 정의를 AddDefinition하고 SetSampleState로 메모리 상태만 설정하는 기존 테스트 구현이다. 신규 샘플은 이 계약을 재사용하며 기존 FDialogueStateKey(Kind/TargetId/ItemId/Tag)를 변경하지 않는다.

## Scope / Constraints / Dependencies
- Runtime Public/Samples 및 Private/Samples: 샘플 전용 NPC DataAsset, 아이템·스킬 행과 카탈로그/정의 변환 예제. 다른 게임이 플러그인만 가져가도 호스트 모듈 의존 없이 로드 가능해야 한다.
- Editor 전용 생성 commandlet: 신규 샘플 패키지만 생성하며 기존 파일/패키지 존재 시 덮어쓰지 않는다.
- 실제 .uasset 5개: NPC 2개, 아이템/스킬 표 각 1개, 원본 참조 카탈로그 1개. 아이템·스킬 항목은 카탈로그에 중복 수기 등록하지 않는다.
- 자동 검증, UserGuide 49절, Blueprint 승인/결과 기록.
- 제외: Conditions 선택 UI, 실제 게임 프로젝트 수정, 코어 계약·직렬화·SaveData 변경, 스킬 실행 시스템, 새 외부 플러그인.

## Assumptions / Open Questions
샘플의 Player 대상·NPC 호감도·스킬 레벨 키는 예시 규약이며 게임에 강제하지 않는다. 샘플 숫자는 밸런스 결정이 아니다. NPC 원본 연결은 명시적 참조이며 전 프로젝트 자동 수집/누락 보장은 제공하지 않는다.

## Uncertainty & PoC Track
UE 5.4.4 Editor commandlet으로 신규 UDataTable/UDataAsset을 생성·SavePackage하고 별도 프로세스 자동화에서 다시 로드한다. 실패 시 오류를 수정하고 재실행한다. 부분 생성 결과가 있으면 기존 에셋을 덮어쓰지 않고 존재하는 파일을 검증한 뒤 누락 생성만 명시적으로 수행하는 방법을 결정한다.

## Plan
1. Luna 조사와 Sol 구조 검토, Director의 샘플 범위 인계를 취합한다.
2. 샘플 타입/검증·정의 변환/새 메모리 제공자 생성 API를 구현한다.
3. 생성 commandlet과 자동 검증을 작성하고 Editor 빌드 후 실제 에셋을 생성한다.
4. 다른 프로세스에서 에셋 로드·변환·현재 상태 분리·실패 시 부분 출력 없음·중복 ID/잘못된 표 검사를 실행한다.
5. 가이드와 결과를 기록하고 기능별 한국어 커밋 및 Codex_Develop --no-ff 통합/push한다.

## Risks & Mitigations
샘플을 게임 시스템 완성으로 오인하지 않도록 클래스/에셋 명칭과 한국어 툴팁/설명에 Sample을 표시한다. ID와 표시명은 분리하고 행 이름은 ID를 대체하지 않는다. 목록 변환 실패는 false/빈 결과/원인 문자열로 반환한다. 현재 값은 원본 에셋에 저장하지 않는다. soft 참조는 명시적 로드 시 읽으며 대화 플러그인 기본 설정에 자동 등록하지 않는다.

## Design Handoff / Approval Points
BP-116. GameDirector 인계 준비됨(2026-11-18). 실제 샘플 로드/공통 정의 대응/개발자 교체 지점 안내가 완료 기준이다. 사용자 요청으로 신규 샘플 생성 승인됨. 기존 에셋 삭제·변경/코어 저장 포맷 변경은 승인 범위 밖이다.

## Rollback Plan / Backward-compat Matrix
기준 2fb1775와 feature/dialogue-definition-samples 보존. 기존 에셋/공개 계약/저장 필드는 불변이고 신규 샘플만 추가한다. 복구는 이후 의존성을 확인한 뒤 기능 커밋 또는 merge 첫 부모 revert를 검토하며 기존 사용자 변경은 보존한다.

## Progress Log
- [x] 기존 Blueprint.md 미커밋 논의 보존, 작업 브랜치 생성.
- [x] 샘플 코드·에셋 생성.
- [x] 검토/빌드/실제 에셋 로드/기능 검사.
- [x] 안내서/결과 기록. Git 통합/push SHA는 Git 이력과 최종 답변으로 확인한다.

## Decision Log / Discoveries
실제 게임의 스킬 동작 질문은 이번 구현의 선행 조건이 아니다. 정의 샘플은 플러그인 내 Samples로 제공하되 게임이 자신의 원본/제공자로 교체하는 예제다.

## Verification / Evidence / Result
- 최초 빌드 실패: Saved/AI/20261118_162729_90cafd9f/Build.log. IncrediBuild가 cl.exe 실행 전에 early hooks/VE/IM injection 오류로 중단했다. C++ 오류가 아니며 엔진/전역 설정을 바꾸지 않고 이번 실행만 -NoXGE로 재시도했다.
- 로컬 Editor 빌드 성공: Saved/AI/DefinitionSamplesBuild_20261118_162821/Build.log. 검토 테스트 보완 후 163033 빌드도 성공.
- 실제 에셋 생성 성공: Saved/AI/DefinitionSamples_20261118_162959_38649bb0/result.json. 5개 .uasset와 SHA256을 기록했다.
- 생성기와 별도 프로세스의 샘플 자동화 3/3 통과, Error 0: Saved/AI/20261118_163050_4ea46353/Report/index.json.
- 기존 대상 재생성 거부(의도된 실패): Saved/AI/DefinitionSamples_20261118_163144_7b6df20b/result.json. 재실행 전후 5개 파일 SHA256 불변: Saved/AI/DefinitionSamplesOverwriteCheck.json.
- 최초 생성의 기존 Python 노출 이름 중복 경고(DialogueStoryScope)는 이번 변경과 무관하며 수정하지 않았다.
- 최종 Editor/UHT 빌드 성공: Saved/AI/DefinitionSamplesBuild_20261118_163210/Build.log. -NoXGE는 이번 작업 실행 인자만 사용하고 엔진/빌드 전역 설정은 불변이다.
- 최종 DialogueKit 전체 자동화 64/64, Error 0: Saved/AI/20261118_163306_d247a3ef/Report/index.json 및 result.json. 새 샘플 3개와 Blueprint 실행 핀 reflection 검사 포함. 전체 테스트 후에도 생성 당시 5개 샘플 SHA256 불변.
- GameDirector(Astra medium) 제품 검토: 샘플 승인 범위 일치, 필수 누락 없음. 정적 코드/안내서와 전달받은 자동 검증 증거에 기반하며 GUI 직접 관찰은 아니다.
- Windows Git diff --check, UserGuide 49절 목차/실제 에셋 이름·공개 API·예제 대응 확인 완료.

## Changes
- Plugins/DialogueKit/Source/DialogueKit/Public/Samples/DialogueSampleDefinitions.h
- Plugins/DialogueKit/Source/DialogueKit/Private/Samples/DialogueSampleDefinitions.cpp
- Plugins/DialogueKit/Source/DialogueKitEditor/Public/DialogueCreateDefinitionSamplesCommandlet.h
- Plugins/DialogueKit/Source/DialogueKitEditor/Private/DialogueCreateDefinitionSamplesCommandlet.cpp
- Plugins/DialogueKit/Source/DialogueKitEditor/Private/Tests/DialogueDefinitionSamplesTests.cpp
- Plugins/DialogueKit/Content/Sample/GameDefinitions: DA_SampleNPC_Merchant, DA_SampleNPC_Guard, DT_SampleItems, DT_SampleSkills, DA_SampleDefinitionCatalog (.uasset)
- Scripts/AI/Invoke-DefinitionSamples.ps1, Docs/UserGuide.md 49절, Blueprint.md 이번 승인/결과 절, 이 실행계획.

## Review / Discoveries
- Luna High: 기존 SavePackage·독립 프로세스 재로드·호스트 의존 없는 배치 근거 조사. 기존 UDialoguePortraitData/ItemRow는 호환성 보존하고 별도 Sample 타입을 추가했다.
- Sol Medium: 원자 출력, strict row type, 복합키 중복, 수치 범위, 정렬, soft 이미지 미로드, 새 provider 수명·독립성 검토. NPC 이름 누락/사용자 지정 매핑/인터페이스 실패 출력 정리 검사도 반영했다.
- 메인: const BlueprintCallable의 자동 Pure 변환을 피하도록 새 BuildDefinitions/CreateMemoryProvider에 BlueprintPure=false를 지정하고 reflection 검사로 실행 핀 계약을 확인한다. 기존 공통 인터페이스 계약은 수정하지 않는다.
- 생성 중 일부 저장 실패 시 기존/부분 파일을 자동 정리하지 않으며 재실행은 충돌로 거부한다. 안내서에 복구 판단이 필요함을 명시했다.

## Remaining Verification
실제 GUI에서 에셋 더블클릭·Blueprint 노드 배선 관찰은 미실행이다. 에셋 로드 및 API 자동화와 구분한다. PIE, cook/패키징, 패키지 실행, 별도 실제 게임 통합은 이번 샘플 범위에서 실행하지 않는다. 이미지/스킬 실행 예제는 구현하지 않으며 아이콘/초상화는 의도적으로 비어 있다.

## Overall Progress
- 이전 S0~S4 공통 계약·Graph 실행·저장/복원·후보 선정 완료 유지.
- 이전 S5~S7 기술 구현/자동 검증 완료 유지, 실제 GUI·화면·입력·음향·Shipping 실행·제품 인수 및 실제 게임/Prototype2 통합은 별도 남음.
- 이번 BP-116 보완: 개발자 참고용 실제 정의 샘플과 변환·상태 분리 예제.
- 남은 BP-116: Conditions 정의 목록 선택 UI 및 게임별 실제 원본/상태 연결. 샘플 제공 완료를 그 완료로 표시하지 않는다.


## Final Result
NPC 2개 DataAsset, 아이템/스킬 각 6행 DataTable, 원본 참조 카탈로그를 구현·생성하고 총 14개 공통 정의로 연결했다. BP-116 샘플 제공 항목은 구현 및 자동 검증 완료다. 이전 대화에서 직접 작성한 BP-116 논의 기록도 관련 설계 이력으로 검토 후 이번 문서 커밋에 포함한다. 실제 게임 설계 확정이나 드롭다운 완료로 확대하지 않는다.

구현·에셋·UserGuide 커밋: `66c1878`. 생성/검증 도구와 BP-116 논의·승인·결과는 별도 커밋으로 기록한다. 기능 브랜치와 기준 SHA를 유지한다.
