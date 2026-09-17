# AGENTS3.md - DialogueMaker Execution Profile

## 자동화 작업 연결 (2026-09-15)
- 환경과 기준 빌드 결과: `Docs/AI/Environment.md`.
- 빌드/테스트/Git/Fork 절차: `Docs/AI/Build.md`, 실행 파일: `Scripts/AI/Invoke-UE.ps1`, `Scripts/AI/Invoke-Git.ps1`.
- Git 상태와 변경 관리는 Windows Git을 사용한다. WSL Git의 줄바꿈/LFS 차이를 사용자 변경으로 단정하거나 일괄 정리하지 않는다.
- 사용자가 요청한 기능과 승인한 구현 범위 안의 코드 작성, 로컬 빌드/검증/오류 수정은 단계마다 재승인받지 않고 진행한다. 아래 승인 게이트는 이미 부여된 승인을 반복 요청하는 근거로 사용하지 않는다.
- 저장 형식 변경, 기존 에셋 삭제/이름 변경, 엔진 변경/새 외부 플러그인, 승인 범위 밖 재설계는 중단 후 사용자 확인을 받는다.
- 기존 미커밋 변경을 보존하고 전체 stage를 하지 않는다. Git 명령의 확인 기준은 아래 「Git 실행 확인 정책」을 따른다.

### Git 실행 확인 정책 (2026-09-17)
- 사용자 요청을 수행하는 데 필요한 Git 명령은 데이터 소실 우려가 없으면 별도 사용자 확인(confirm) 없이 실행한다. 이 지시는 이후 작업에도 적용되는 상시 승인이다.
- 조회(status/log/diff/show 등), fetch, 새 브랜치 생성, 변경을 보존하는 브랜치 전환·이름 변경, upstream 설정, 명시적 파일 stage, 일반 commit, 비강제 push, 기존 이력과 변경을 보존하는 merge/pull/cherry-pick 등은 손실 우려가 없는 경우 재확인하지 않는다. 예시는 허용 명령의 한정 목록이 아니며 실제 옵션과 대상, 결과로 판단한다.
- 특정 커밋을 브랜치 이력에서 제외하는 reset/rebase, amend 등 이력 재작성, 작업 파일·미커밋 변경의 삭제/덮어쓰기(restore/checkout의 파일 복원, reset --hard, clean, rm 등), 로컬·원격 브랜치/태그 삭제, 강제 push(--force-with-lease 포함), stash drop/clear/pop, 복구 데이터를 제거하는 prune/gc 등 데이터 소실 우려가 있는 작업은 대상과 영향을 설명하고 실행 전 확인받는다. 이미 해당 대상과 영향까지 명시적으로 승인받은 작업은 같은 승인을 반복 요청하지 않는다.
- 실행 전 상태·대상·옵션을 직접 확인하고 기존 사용자 변경을 보존한다. 충돌이나 예상하지 못한 변경이 생겨도 자동으로 폐기하거나 덮어쓰지 않는다. 명령 실행에 대한 상시 승인은 사용자 요청 범위 밖 작업을 시작하라는 뜻이 아니다.
- 이 정책은 프로젝트의 Git 관련 재확인 문구보다 우선한다. 시스템/관리자/샌드박스가 강제하는 실행 권한 승인은 별개이며, 이 문서로 우회하거나 비활성화하지 않는다.

### 자동화 작업 운영 계속
- 메인은 작업 시작 시 Docs/AI/Agents.md를 읽고 기본 절차로 적용한다. 필요한 독립 조사·검토의 실제 하위 위임은 사용자의 상시 지시이므로, 매 요청에 위임 문구나 위임 자체의 재승인을 요구하지 않는다. 메인이 역할·질문·인원을 정해 실제 하위를 생성하고 관련 결과를 취합한 뒤 메인만 수정한다.
- 이번 요청의 명시적인 하위 사용 금지·역할 지정·수정 금지 등은 우선한다. 사소한 문서/오탈자 또는 독립적으로 맡길 작업이 없으면 생략 이유를 짧게 보고한다. 조사/설명/리뷰만 요청하면 취합·보고에서 끝낸다. 기존 사용자 확인 조건은 유지한다.
- 역할은 Sol Medium 검토자, Luna High 검색자, 어려운 버그만 Sol High를 사용한다. 하위는 읽기 전용이며 재위임하지 않는다. 메인 제외 하위 동시 3개 한도를 지키고, 메인만 Git·빌드·테스트·에디터 조작을 담당한다. 같은 에디터/빌드를 동시에 조작하지 않는다.
- 테스트 0개/보고서 없음은 성공이 아니다. 엔진 내장 테스트, 기능 테스트, PIE, 패키징을 구분해서 보고한다.

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