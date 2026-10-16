# Task: 스토리 정의 에셋의 안전한 직접 편집

## Goal / Context
2026-10-16 사용자 요청: DialogueStoryDefinition에서 Chapter/Scene 추가 시 0 ID 문제와 Scene 부모 ID 직접 입력을 수정한다. 기준 Codex_Develop 9fc86cf. 원인: 기본 DataAsset Details 배열 +는 구조체 기본값을 추가하며 AddChapter/AddScene API를 호출하지 않는다. 기존 스토리보드 버튼만 GUID 발급 경로를 사용한다.

## Scope / Plan
1. Editor 전용 StoryDefinition Details를 등록하여 기본 배열 원시 편집 대신 + Chapter/+ Scene 버튼과 항목별 속성 행 제공. 기존 API로 생성하여 새 GUID 보장.
2. Scene 소속 Chapter는 현재 편집 중인 정의의 이름+ID 드롭다운 및 소속 없음으로 선택. 다른 프로젝트 설정 정의를 참조하지 않는다.
3. 과거 잘못 생성된 0 ID는 명시적인 누락 ID 생성 버튼으로만 발급. 유효 ID·이름·순서·부모·Block 참조는 유지. 누락 부모를 추측하여 연결하지 않는다.
4. Editor 빌드, 생성/복구/부모 선택/Undo 및 직렬화 회귀, 사용자 문서와 한국어 작은 커밋 후 --no-ff 통합/push.

## Constraints / Compatibility
Editor만 변경. 저장 자료형/런타임 정책/기존 에셋 바이너리 미변경. 생성자·로드·저장 시 자동 ID 발급 없음. 기존 유효 중복 ID는 임의 수정하지 않고 진단. 기본 배열 삭제/복제 대신 기존 스토리보드의 참조 보호·이중 확인 경로로 이동/제거 안내. 다중 에셋 직접 편집은 거부해 잘못된 부모를 선택하지 않는다.

## Assumptions / Risks / PoC
일반 DataAsset editor의 class customization 등록을 사용한다. PropertyHandle로 일반 필드를 유지하고 약한 객체 참조/ID로 메뉴 실행시 재검사한다. 화면 리빌드는 deferred RequestRefresh로 위젯 수명 문제 방지. 기존 에디터 조작 도구가 없어 실제 GUI 표시/클릭은 미검증으로 명시.

## Approval / Review
사용자의 직접 수정 요청 범위. 미확정 제품 설계나 저장 스키마 변경 없음. Sol Medium 읽기 전용 검토 후 메인만 구현/Git/빌드/검증.

## Rollback
fix/dialogue-story-definition-details 브랜치 보존, 기능 merge는 git revert -m 1. 기존 이력 및 사용자 작업 폐기 금지. 사용자가 새 ID를 발급해 저장한 데이터는 코드 롤백으로 되돌리지 않는다.

## Progress / Evidence
- 원인 조사와 검토 진행.

## Result / Verification
- Editor 전용 Details 등록/해제 완료. 일반 DataAsset 에디터에서 생성 API를 호출하며 원시 배열 추가·복제·삭제/부모 GUID 입력을 노출하지 않는다. ID 읽기 전용, 이름·설명·정렬은 PropertyHandle 편집을 유지한다.
- 누락 ID 생성은 0 자체 ID만 변경하고 부모·기존 ID·유효 중복 ID는 보존한다. 같은 이름의 Chapter도 메뉴와 현재 표시의 전체 GUID로 구별한다. 해당 Scene의 잘못된 부모만 있는 경우 명시적 재선택을 허용하고 다른 오류는 거부한다.
- Sol Medium 읽기 전용 사전/후속 검토 수행. 수명/파생 자료/Undo 경로에 차단 문제 없음. 닫힌 콤보의 ID 표시 권고를 반영했고 다른 Scene 부모 오류를 우회하지 않는 회귀를 추가했다.
- 최종 Editor Build: Saved/AI/20261016_174240_7e98454f/Build.log 성공.
- 최종 DialogueKit.Editor: Saved/AI/20261016_174253_f676bc7f/result.json 11/11 통과, Error 0. 새 StoryDefinitionDetails는 생성/복구/부모 변경/Undo/Redo/중복 보존, 실제 Details 테스트 창의 버튼 생성, GUID 격리 에셋 파일 저장·언로드·재열기를 검사한다.
- 실패 이력: 173811_80818a0b는 조건부 포맷 문자열의 UE static_assert를 고정 포맷으로 수정. 174010_ae463dc0는 테스트의 TArray 내부 원소 자기 Add로 assertion 발생하여 별도 복사본으로 수정. 174105_f2aa4409는 Details 가상 목록의 배치 전 버튼 검사 실패로 테스트 창 배치/Slate Tick 후 검사하도록 수정. 각 실패는 보존하고 최종 성공과 구분한다. 새 테스트 소스 인식용 UBT 갱신으로 Build.cs 타임스탬프만 갱신했으며 내용 변경은 없다.
- 실제 GUI 가독성/마우스 조작은 미검증. 이번 Editor 수정에서 PIE/패키징은 미실행. 기존 EnhancedInput·XGE·Git safe.directory 경고는 전역 설정 변경 없이 유지.
- 기존 에셋 바이너리/사용자 저장/Prototype2 변경 없음. 사용자 기존 0 ID 에셋은 에디터의 명시적인 복구 버튼으로 처리해야 한다.
- 전체 계획: S0~S7 구현 상태 유지, 이번 S5 저작 UI 결함 보완 완료. S5~S7 실제 화면/입력/음향 및 Shipping 실제 실행/제품 인수, Prototype2 통합은 별도 잔여.
