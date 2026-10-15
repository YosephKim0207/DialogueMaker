# DialogueMaker 구현 실행계획과 자동 진행 지침

작성: 2026-10-13. 상태: S0~S2 완료 / S3·S4 부분 구현 / 실행 증거 구현·빌드 완료/자동화 실행 환경 차단·롤백 연동 대기 / S5~S7 미착수.
이 계획은 PLANS.md의 실행계획이며 제품 기준은 Blueprint.md이다. 단계의 작업·검증·Git 운영 기준을 한곳에서 관리한다. 최신 사용자 지시가 우선하며 계획이 제품 결정을 임의 변경하지 않는다.

## Goal

- UE 5.4.4 DialogueKit을 확정된 Graph 저작·실행·게임 연동·저장 복원 계약에 맞춰 구현한다.
- 각 기능에서 재현 가능한 검증 증거를 확보하고 기능별 Branch와 작은 커밋을 남긴다. `Codex_Develop`에 검증된 기능을 merge하여 누적 통합한다. S0~S7은 의존성과 검증 마일스톤이며 Branch 단위가 아니다.
- 승인된 구현 실행 중에는 필요한 조사→구현→검토→검증→명시적 Stage→작은 Commit→기능 Branch Push→검증된 기능 merge→Codex_Develop Push→다음 기능 진행을 반복 확인 없이 수행한다.
- 최초 요청은 실행계획 작성이었으며 이후 사용자가 실행을 승인했다. 최신 승인은 S3·S4 잔여 구현 재개이며 스키마 1/2·교체·선택적 롤백·증거 저장이 후속 사용자 지시로 승인됐다. 과거 자동 승인 거절 기록은 당시 이력이다.

## Background / Self-Contained Context

- 기준 저장소: C:/Users/kjs94/Documents/Unreal Projects/DialogueMaker. 작성 시 Branch DiagnoseGraphSystem, HEAD d1b1937c60bfc6534536e8757751fc626418945a. 실행 착수 때 최신 상태를 다시 기록한다.
- Windows Git: C:/Program Files/Git/cmd/git.exe. 승인된 origin: git@github.com:YosephKim0207/DialogueMaker.git. 다른 목적지로 자동 변경하지 않는다.
- 엔진 UE 5.4.4, Editor 타깃 DialogueMakerEditor Win64 Development. 빌드는 Scripts/AI/Invoke-UE.ps1을 사용한다.
- Runtime 루트 R = Plugins/DialogueKit/Source/DialogueKit. Editor 루트 E = Plugins/DialogueKit/Source/DialogueKitEditor. 이하 R/E는 이 경로의 축약이다.
- UDialogueGraph 한 에셋이 Block이다. 현재 DialogueNode에 선택지가 내장되어 있고 Editor Graph와 RuntimeGraph가 서로 변환된다. 신규 Choice·StateChange 및 버전/ID를 양쪽에 함께 반영해야 한다.
- 기존 실행/저장 소비처의 Graph 키는 이름 기반 PrimaryAssetId이며 열람 저장은 Graph별 NodeGuid 목록이다. S1에서 안정 Block ID·ChoiceId·열람 버전·체크포인트 계약을 추가했으며 기존 소비처 전환은 S2~S4다.
- 현재 후보 전체를 비동기 로드한 뒤 첫 객체를 사용한다. 신규 방식은 Registry 메타데이터 조건 평가→후보 선정→선정 Block만 로드한다.
- 현재 저장은 여러 SaveGame 자료의 동기 저장 경로다. 신규 정책은 게임이 단일 FIFO 조율자를 소유하고 게임 상태와 대화 상태를 동일 스냅샷으로 저장하는 것이다.
- 기존 샘플 UMG 내부는 정적 검토로 확인하지 못했다. 최초 조사에는 자체 C++ 테스트가 없었고 S0/S1에서 DialogueKit.Baseline/Contracts 테스트를 추가했다. 엔진 내장 테스트만으로 기능 검증을 대체하지 않는다.
- Prototype2는 별도 저장소다. 현재 chore/dialogue-integration-handoff에 Blueprint.md 미커밋 인계가 있다. 본 계획에서 이를 커밋하거나 코드·플러그인을 복사하지 않는다.

## Scope / Constraints

- In: Blueprint BP-101~106, BP-108~118의 확정 범위와 기존 BP-001~008에서 이에 영향을 받는 연결부, 개발자용 한국어 주석·샘플·검증 코드·실행 스크립트.
- Out: BP-107 보류 기능, 그룹 접기·중첩 조건·다중 Block 검색, Prototype2의 게임 시스템 구현/배치, 새 외부 플러그인, 실제 서비스 로그 전송, 실제 사용자 세이브 변경.
- Runtime에 Editor 모듈 의존성을 넣지 않는다. 게임별 아이템·능력치 enum/상태를 플러그인에 하드코딩하지 않는다.
- 기존 프로토타입 에셋은 사용자가 수동 재작성하기로 했다. 자동 이전·일괄 삭제·이름 변경을 하지 않는다. 새 검증 에셋은 독립 테스트 경로에 UE 직렬화 API로 생성하고 기존 바이너리를 텍스트로 수정하지 않는다.
- 공통 기본 인사 Block은 사용자가 생성한다. 플러그인은 에셋 선택기·검증·안내만 제공하고 사용자용 인사 에셋을 대신 생성하지 않는다. 별도 테스트 fixture는 사용자 콘텐츠와 구분한다.
- 신규 확정 스키마/체크포인트 구현과 지원 저장의 무단 파괴는 구분한다. 실제 배포된 저장/에셋이 발견되면 지원 범위와 이전 경로를 확인하고 훼손하지 않는다.

## Design Handoff / Approval Evidence

- Workflow: Docs/AI/GameDirector.md의 단일 흐름 적용. 제품 정책의 근거는 Blueprint.md 5~6절, 이번 범위는 위 BP 항목이다.
- 관찰 가능한 목표: 제작자는 Graph와 스토리보드로 조건부 대화를 작성·검사·PIE 테스트하고, 게임은 공통 제공자와 저장 조율자로 실제 상태를 연동한다.
- 확정 관계: 한 Line은 한 화자/발화, Choice 항목별 조건, Branch True/False, StateChange 게임 효과 요청, End 완료. 입력 다중/출력별 단일 연결. 사전 확인은 직접 다음 노드만이며 효과·진행·저장을 변경하지 않는다.
- 확정 저장: 결과 있는 선택/StateChange 성공 시 효과·선택·다음 위치 스냅샷, FIFO 비동기 저장 중 진행 허용. End 저장 기본 꺼짐. 성공 저장만 복원하고 효과 재실행 금지. 읽음 기록과 게임 사건 조건은 별개다.
- 확정 후보: 대상 태그 정확 일치, 요청 시점 조건 인정, 최우선→일반 우선순위 동률 균등→기본 순회→공통 인사. 일반 세션 완료는 임시 제외, 세션을 넘는 제외는 게임 조건. 소속 없음은 신규 후보 제외, 제한 없음과 구분한다.
- 확정 호환: 구버전은 신규 노출 중단으로 보존하고 활성 복원에 사용. 명시적 ID/버전범위 교체만 허용하며 대체 처음부터 시작, 필요한 보정은 게임 성공 후 실행. 스토리 초기화는 이전 활성 위치를 해제하고 신규 사용 콘텐츠를 선정한다.
- 오류 정책: 일반 빈 조건은 무조건, 빈 Branch는 로그 후 True, 평가 오류 항목은 로그 후 True를 다른 조건과 종합. 기본 Choice는 일반 적격 항목이 없을 때만 표시. 빈 Choice 런타임은 진단 후 정상 End, 저장 검사에서는 완전 빈 Choice 저장 금지. 잘못된 연결/자동 Branch 순환은 중단하고 마지막 저장 유지.
- 2026-10-13 사용자 요청: 자동 구현·검증·단계별 Branch/Add/Commit/Push가 가능한 Plan과 지침 작성. 계획 실행 요청 후 승인된 범위의 반복 확인은 하지 않는다. 실행 승인 원문/일시는 착수 시 Progress Log에 기록한다.
- GameDirector는 현재 세션에서 호출 가능한 역할이 아니다. 상태는 Unavailable / Not reviewed이며 다른 검토자가 Director 검토를 완료했다고 대체하지 않는다. 승인된 독립 기술 작업은 계속할 수 있으나 제품 검토 미완료를 숨기지 않는다.
- Technical Freedom: 합의 범위 안의 클래스 분리·자료형·테스트 구현·내부 API는 메인이 근거를 남겨 결정한다. 제품 동작 변경이나 미확정 게임 정책은 반환한다.

## Dependencies / Assumptions / Open Questions

- 단계 순서는 S0→S1→S2→S3→S4→S5→S6→S7. UI 편의 기능은 뒤로 미루되 데이터 왕복과 기본 UI 검증은 S2부터 수행한다.
- 일반 제품 결정의 미결정 항목은 현재 없다. 상세 내부 API와 테스트 구성은 아래 PoC 결과를 반영한다.
- 남은 기술 불확실성: 새 ID와 지역화 키 연결, 새로운 Graph 저장 왕복, 실제 UI 바인딩, 공통 저장 스냅샷 연동, cook 메타데이터/구버전 에셋 보존, GUI/음향 확인 수단.
- Invoke-UE.ps1은 Check/Build/Test를 지원하며 S0에서 별도 Invoke-PackageValidation.ps1의 Check/Package 경로를 구현했다. PIE Action은 제공하지 않는다. 실제 화면·입력 검증은 별도 필요하다.
- GUI 제어가 없으면 코드 기반 Editor/기능 테스트를 먼저 자동화하고 실제 표시·입력·음향의 수동 검증이 남는 범위를 명시한다. 새 외부 도구 설치를 자동 전제로 삼지 않는다.

## 자동 실행 및 Git 운영 지침

2026-10-14 사용자 지시가 이전의 단계별 연쇄 Branch 정책을 대체한다. 통합 Branch의 정확한 이름은 `Codex_Develop`이며 일반적인 기능 하나마다 여기서 분기하고 검증 후 여기로 merge한다. Develop/main/DiagnoseGraphSystem으로 자동 통합하지 않는다.

```text
Codex_Develop ──●────────────────M1────────────────M2──→
               └─ feature/A ─c1─c2─┘               │
                                  └─ feature/B ─c3─c4─┘
```

1. 착수/재개 시 AGENTS.md, 관련 Blueprint, 본 계획의 Progress Log/Evidence와 Windows Git 상태·HEAD·upstream·원격을 확인한다. 메인만 Git·수정·빌드·테스트를 수행하고 필요한 실제 읽기 전용 하위 검토를 취합한다.
2. 최초에는 승인된 현재 기준 HEAD를 기록하고 `Codex_Develop`를 생성한다. 계획 변경만 미커밋이면 안전한 Windows Git `switch -c Codex_Develop`로 그대로 보존한 뒤 계획 파일만 검증·Stage·준비 커밋·일반 push한다. 사용자 변경은 포함하거나 정리하지 않는다. 이미 Codex_Develop가 있으면 로컬/원격과 기록을 확인해 재개하며 같은 이름으로 재생성·reset하지 않는다. 원격에만 있으면 해당 Branch를 추적하고, 관계가 불명확하면 먼저 확인한다. dirty tree를 거부하는 NewBranch 래퍼를 강제로 통과시키지 않는다.
3. 기능마다 작업 목적·범위·의존 기능·테스트·복구 단위를 먼저 기록한다. `feature/dialogue-<기능>`, 버그는 `fix/dialogue-<문제>`, 준비 작업은 `chore/dialogue-<작업>`을 쓴다. S1이나 S5 전체를 하나의 기능으로 묶지 않는다. 서로 떨어뜨리면 컴파일/동작이 깨지는 동일 기능의 Runtime/Editor 변경은 같은 Branch에서 처리한다.
4. 기능 Branch는 최신 검증된 `Codex_Develop` HEAD에서 만든다. 선행 기능이 필요하면 선행 기능의 검증과 통합을 먼저 끝낸다. 미통합 기능 Branch를 다음 기능의 기준으로 사용하는 연쇄 분기는 하지 않는다. 병렬 진행이 꼭 필요하면 공통 검증 기준에서 독립 분기하고 merge 전 최신 통합 상태에서 다시 검증한다.
5. 커밋은 기능 완성 시 한 번에 몰아서 하지 않고 하나의 설명 가능한 변경 목적별로 작게 나눈다. 예: 조건 자료형과 직렬화 → 조건 평가와 해당 테스트 → 게임 제공자 연결과 예제. 무관한 정리/포맷팅/다른 기능은 섞지 않는다. 반대로 헤더와 구현, 스키마와 필수 소비처, 구현과 필요한 회귀 테스트를 억지로 나눠 깨진 중간 커밋을 만들지 않는다. 가능한 각 커밋을 빌드·검증 가능한 상태로 유지한다.
6. 각 작은 작업 후 diff·관련 검증을 확인하고 `git add -- <명시적 경로>` 또는 필요한 hunk만 Stage한다. add . / add -A 금지. `git diff --cached --check`와 staged diff를 확인한 뒤 Subject/Description으로 커밋한다. 본문에 목적·검증·미검증·단계/BP와 필요한 선행 커밋을 남긴다. 여러 줄 본문은 UTF-8 임시 파일과 --file을 사용한다. 실제 세이브·비밀값·빌드 산출물을 포함하지 않는다.
7. 기능 Branch의 커밋을 origin에 일반 push하여 복구 기준을 원격에도 보존한다. 기능을 끝낼 때까지 커밋/push를 무조건 미루지 않는다. 매 커밋과 push는 재확인하지 않으며 인증/네트워크 실패와 로컬 성공을 구분한다. 강제 push하지 않는다.
8. 기능 상태는 Planned→In Progress→Implemented→Verification Pending/Failed 또는 Verified로 기록한다. Commit/Push 및 Director 제품 검토 상태는 별개다. WIP 커밋·push는 가능하지만 미검증 기능을 Codex_Develop에 merge하지 않는다. 실행 불가인 필수 검증을 삭제해서 통과로 만들지 않는다.
9. 기능 완료 시 사용자 안내서 작성·사용법 검증을 포함한 독립 검토와 기능 필수 검증을 마친다. 통합 직전에 fetch하여 원격 Codex_Develop와 기록을 확인하고 검증에 사용한 `IntegrationBaseSHA`를 기록한다. 통합 Branch가 전진했다면 기능 Branch에 최신 Codex_Develop를 `git merge --no-commit Codex_Develop`로 반영하고, 새 merge 커밋이 필요한 경우 필수 메시지를 작성한다. 충돌을 확정 설계에 맞게 해결한 뒤 영향 검증을 재실행한다. rebase/force로 이력을 재작성하지 않는다. 원격이 예기치 않게 바뀌면 무조건 push하지 않고 차이를 먼저 검토한다.
10. 실제 merge 직전에 Codex_Develop HEAD가 검증한 IntegrationBaseSHA와 같은지 다시 확인한다. 바뀌었으면 기능 Branch에 최신 통합을 반영하고 영향 검증부터 반복한다. 깨끗한 Codex_Develop로 전환해 검증한 기능 tip을 `git merge --no-ff --no-commit <기능 Branch>`로 통합 내용을 준비하고 아래 필수 Subject/Description을 작성해 merge 커밋을 만든다. squash/fast-forward로 기능 경계를 지우지 않는다. Codex_Develop가 첫 부모인지 확인하고 BaseSHA·FeatureTipSHA·MergeSHA·Parents를 기록한다. 일반 merge는 이 사용자 지시로 승인된 범위이므로 재확인하지 않는다.
11. merge 결과가 기능 Branch에서 검증한 트리와 같은지 확인한다. 같으면 기존 전체 검증 증거를 재사용하되 실제 통합 Branch에서 정해 둔 핵심 회귀 검사를 수행한다. 다르거나 통합 충돌을 해결했다면 관련 빌드/회귀 검증을 다시 수행한다. 트리 비교나 merge 무충돌만으로 통합 검증을 대신하지 않는다. 실패한 통합 결과는 안정 상태로 push하거나 다음 기능의 기준으로 쓰지 않는다. 기능 전체 revert 등 아래 복구 절차로 통합 상태를 복구한다.
12. 통합 검증 후 상태를 Integrated로 기록하고 `git push origin Codex_Develop`하고 원격 SHA를 확인한다. 다음 기능은 이 통합 커밋에서 시작한다. 마일스톤의 모든 기능이 통합되면 단계 전체 검증도 수행한 후 의존 마일스톤으로 넘어간다. 단계 전체 실패 시 실패 기능을 추적·복구하고 완료로 표시하지 않는다.
13. 같은 이름의 기능 Branch는 BaseSHA·진척이 일치할 때만 재개한다. 다른 작업이면 보존하고 고유한 새 이름을 쓴다. 기능 Branch는 merge 후에도 로컬/원격에 보존한다. 자동 삭제·정리·prune/gc하지 않는다. 사용자 미커밋 변경을 stash/reset/clean으로 없애지 않는다.
14. 반복 승인 없이 구현·오류 수정·검증·작은 커밋·push·통합을 계속하되 실패 원인 없이 검증을 반복하지 않는다. 컨텍스트 전환 전 기능/커밋별 재개 정보를 기록한다. GUI/환경 때문에 검증이 막히면 독립 기능만 마지막 검증 통합점에서 진행한다.
15. 정상 복구는 이력을 보존하는 revert를 우선한다. 커밋을 이력에서 제거하는 reset/rebase/amend, force push, 기존 에셋/사용자 데이터 삭제는 별도 대상·영향 확인 없이 하지 않는다. 개발자에게 개별 커밋/기능 단위 제거가 가능한 경계를 제공한다는 요구를 공유 이력 무단 삭제 허가로 해석하지 않는다. 샌드박스 승인 거부도 우회하지 않는다.

### 모든 커밋의 Subject / Description 작성 의무

- 일반 구현·테스트·문서·WIP·merge·revert·재도입을 포함한 모든 커밋에 작업을 요약한 Subject와 비어 있지 않은 Description을 작성한다. 기본 merge/revert 메시지만 사용하거나 제목만 적고 커밋하지 않는다.
- Subject: `유형: 이번 커밋에서 변경한 기능과 동작`으로 짧고 구체적으로 쓴다. 예: `feat: Choice 항목별 조건 필터링 추가`. `수정`, `작업 완료`처럼 내용을 알 수 없는 제목은 사용하지 않는다.
- Description: 변경 내용 요약, 변경 이유/문제, 실제 검증 결과와 미실행 검증, 관련 단계/BP를 기록한다. 의존 커밋·데이터/호환성 영향이 있으면 함께 적는다. 미완료 WIP는 상태와 남은 문제를 명시한다.
- merge Description에는 통합 기능 요약·기능 Branch/검증 tip·통합 기준 SHA·실행한 검증을 적는다. merge 커밋 시점 이후의 검증을 미리 성공으로 쓰지 않고 후속 Evidence에 결과를 기록한다. revert Description에는 제거하는 동작·대상 SHA·이유·의존성/호환성 영향·복구 검증을 적는다.
- 자동 생성 merge/revert도 먼저 `--no-commit`으로 변경을 준비하고, 내용 검토 후 UTF-8 메시지 파일을 `git commit --file <파일>`에 전달한다. merge 충돌 또는 실패 시 바로 commit하지 않고 해결·검증 후 진행한다. 메시지 파일은 Stage하지 않는 임시 경로에 둔다.
- 커밋 후 `git log -1 --format=%B`와 변경 파일을 확인하여 Subject/Description과 실제 diff가 일치하는지 확인한다. 확인 없이 push하지 않는다. 오류를 바로잡기 위해 무단 amend/이력 재작성하지 않는다.

### 기능별 사용자 문서 작성 의무

- 각 구현 항목을 마무리할 때 실제 구현 결과를 바탕으로 `Docs/UserGuide.md`를 작성·갱신한다. 사용자 문서 검증을 해당 기능의 완료/통합 조건에 포함한다. 전체 구현 종료까지 문서 작성을 미루지 않는다.
- 안내서는 Git에서 직접 읽을 수 있는 Markdown으로 관리한다. 문서 제목 바로 아래, 소개 본문보다 앞에 클릭 가능한 목차를 둔다. 기능을 추가할 때 목차와 본문을 함께 갱신한다. Git에서 유효한 상대 경로 링크와 제목 앵커를 사용한다.
- 실제 완료된 구현 항목 순서대로 번호가 있는 기능 절을 추가하고, 각 절에 기능 이름·목적·대상 사용자·선행 설정·사용 방법·기대 결과·주의사항/제한을 기록한다. 기존 기능을 수정하면 기존 절을 갱신하여 오래된 사용법을 중복 유지하지 않는다. 필요한 선행 기능은 해당 절로 연결한다.
- 게임 개발자용 설명은 실제 공개 C++ 함수/헤더, Blueprint에서 호출 가능한 노드, 공개 인터페이스·델리게이트·설정 등 외부에 노출된 계약을 기준으로 작성한다. 이름·시그니처·인자·반환/오류·호출 순서·상태/저장 영향·비동기 완료 시점을 설명하고 최소 사용 예제를 제공한다. private 함수나 내부 상태 직접 조작을 정식 사용법으로 안내하지 않는다.
- 시나리오 작가용 설명은 실제 에디터 메뉴 진입 경로, 창/탭·Details 항목·버튼·노드/핀·단축키를 정확한 UI 표시명으로 적고 번호가 있는 조작 순서로 설명한다. 예: 에셋 열기→노드 생성→조건 설정→연결→검사/저장→테스트. 코드 구현 지식 없이 따라 할 수 있게 결과 확인 방법과 오류 안내를 포함한다. 존재하지 않는 메뉴를 추정하여 적지 않는다.
- 확장 가능한 기능은 공개 확장 지점, 게임 측 구현 책임, 구현/등록/연결 순서, 교체 가능한 기본/샘플 구현, 반환 계약과 제한을 명시한다. 실제 제공하지 않는 상속·override·플러그인 수정 경로를 임의 제안하지 않는다. 확장 지점이 없는 기능은 없다고 명시한다.
- 코드와 UI가 모두 관련되면 같은 기능 절 안에서 개발자 연동과 작가 작업을 구분한다. 해당하지 않는 항목은 해당 없음과 이유를 적고 사용하지 않는 API 설명을 억지로 만들지 않는다. 내부 테스트 기반처럼 최종 사용자 노출이 없는 항목은 개발자 대상 설정·실행·결과 확인 절차를 기록한다.
- 예제 API/파일/설정이 구현과 일치하는지 확인하고, 가능한 코드 예제는 컴파일 또는 대응 테스트로 검증한다. UI 절차는 실제 에디터 검증 증거와 연결한다. 문서의 목차·링크·번호·Git 표시를 점검한다. 미검증 환경/제약을 명시하고, 사용법을 검증할 수 없으면 기능의 필수 검증을 완료로 표시하지 않는다.
- 문서는 기능 Branch에서 코드와 함께 갱신하며 같은 기능 merge에 반드시 포함한다. 작은 별도 docs 커밋은 허용한다. 상대 링크로 참조하는 이미지/예제 파일도 필요한 것만 Git 추적한다. Saved/Intermediate 같은 미추적 산출물이나 로컬 절대경로만으로 사용 설명을 제공하지 않는다.
- 기능 되돌림·API 변경·UI 변경 시 사용자 안내서도 같은 복구/수정 범위에서 갱신한다. 안내서에 제거된 기능을 여전히 사용 가능하다고 남기지 않는다. Subject/Description에 문서 변경과 검증 결과를 요약한다.

사용자 안내서의 기본 형식은 다음과 같다. 아래는 작성 형식이며 미구현 기능을 안내서에 완료 상태로 채워 넣으라는 뜻이 아니다.

````markdown
# DialogueMaker 사용자 안내서

## 목차
- [1. 기능 이름](#1-기능-이름)

## 1. 기능 이름

### 기능과 준비 사항
대상 사용자, 기능 목적, 선행 설정과 기대 결과.

### 게임 개발자 사용법
실제 공개 API/Blueprint 노드와 최소 예제, 호출 순서 및 성공·실패 처리.

### 시나리오 작가 사용법
실제 메뉴·Details 표시명 기준의 순차 조작과 결과 확인.

### 확장 방법
공개 인터페이스/등록 지점, 게임 측 책임, 확장 예제와 제한.

### 주의사항과 검증 범위
저장·비동기 영향, 지원 환경, 확인한 결과와 남은 제한.
````

### 실행 명령 기준

Windows PowerShell에서 해당 프로젝트 루트에 진입한 후 실행한다. 각 명령 종료 코드를 확인하며 실패한 명령 뒤의 변경 명령을 무조건 이어 실행하지 않는다.

```powershell
.\Scripts\AI\Invoke-Git.ps1 -Action Status
.\Scripts\AI\Invoke-Git.ps1 -Action NewBranch -Name feature/dialogue-stable-identifiers
.\Scripts\AI\Invoke-UE.ps1 -Action Check
.\Scripts\AI\Invoke-UE.ps1 -Action Build
# 아래 접두사는 해당 단계에서 테스트를 실제 등록한 후 사용한다.
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter DialogueKit.Contracts -TimeoutMinutes 10
```

Git Stage/Commit/Push는 Windows Git 실행 파일로 명시적 인자를 전달한다. NewBranch 예시는 S1의 안정 식별자 기능이며, 통합 Branch Codex_Develop는 기존 래퍼의 feature/fix/chore 이름 검사 대상이 아니므로 Windows Git으로 직접 생성한다. 기능 Branch는 아래 후보를 기능 범위에 맞게 사용한다. 문서 변경만 있으면 UE 빌드를 돌리지 않고 diff·링크·상태 일치 검사를 한다.

## Plan

S0~S7은 마일스톤이다. 각 항목의 기능 Branch 후보를 같은 마일스톤 Branch 하나로 합치지 않는다. 착수 시 일반 기능 하나의 관찰 가능한 목적과 복구 경계를 기준으로 실제 작업 목록을 확정한다. 작은 공통 기반은 별도 기능으로 선행 통합하고 후속 기능은 최신 Codex_Develop를 기준으로 한다. 아래 후보 목록은 제품 범위 추가가 아니라 기존 범위를 Git 단위로 나눈 것이다.

### S0 — 기준선과 검증 기반 준비

- 기능 Branch 후보: chore/dialogue-test-harness, chore/dialogue-package-validation. Files: Scripts/AI/Invoke-UE.ps1, Docs/AI/Build.md, R/Private/Tests 및 E/Private/Tests의 신규 테스트 기반, 필요 시 모듈 Build.cs, 본 계획.
- Change: 현재 Editor 빌드와 시작 오류를 기록하고 기존 기능/새 기능 실패를 분리한다. 로컬 테스트 슬롯/디렉터리, 게임 상태 샘플, fixture 생성 경로를 마련한다. GUI·cook/package 실행 수단을 확인한다. 필요한 스크립트 확장은 기존 mutex/실행 종료 코드/보고서/실제 사용자 데이터 격리를 유지한다.
- Validation: Check, Build, 실제 등록된 테스트 1개 이상과 실패/0개 보고의 구분. 이 단계의 엔진 내장 테스트는 연결 확인일 뿐이다. GUI 검증 미지원과 필요한 수동 절차를 명시한다.
- Exit: 후속 단계마다 사용 가능한 명령/절차·실패 판정·증거 위치가 준비되고 기존 빌드 실패가 해결되거나 영향이 분리돼 있어야 한다. 엔진/외부 플러그인 변경 없이는 불가능하면 관련 경로만 차단한다.

### S1 — 공통 데이터·ID·게임 계약

- 기능 Branch 후보: feature/dialogue-stable-identifiers, feature/dialogue-condition-contract, feature/dialogue-effect-contract, feature/dialogue-checkpoint-contract, feature/dialogue-story-scope, feature/dialogue-diagnostics. Files: R/Public/DialogueGraph.h, R/Private/DialogueGraph.cpp, R/Public/DialogueRuntimeGraph.h, R/Public/DialogueNodeType.h, R/Public/DialogueNodeInfo*.h, R/Public/Struct/DialogueStructure.h, R/Public/Struct/DialogueConditionEvalCriteria.h, 신규 조건/효과/저장/진단 계약·샘플·테스트.
- Change: 안정 Block/Node/선택 식별자와 핀 대응, Chapter/Scene 정의·소속/제한 없음 자료형(관리 UI는 S5), 콘텐츠/저장/열람 버전 분리, Tag/Value/Count 정의와 조회, 조건 AND/OR, 게임 효과 묶음·오류값·저장 스냅샷 DTO·진단 이벤트 규약을 만든다. 구현을 다 끝내기 전 기존 공개 소비처가 컴파일되게 필요한 연결부를 함께 조정한다.
- Validation: DialogueKit.Contracts 접두사(신규 등록). ID 생성/복제/중복 검출, DTO 직렬화 왕복, 알려진 아이템 수량 0과 조회 실패 구분, 빈 조건과 True 대체 후 AND/OR 평가, 런타임의 Editor 의존성 없음, 샘플 어댑터 컴파일.
- Exit: 저장/실행/편집용 공통 계약과 연결 fixture에서 동일한 ID와 조건 의미를 사용하고 게임별 코드를 수정하지 않고 샘플 제공자를 교체할 수 있다. 기존 실행기·저장기·후보 소비처의 실제 전환은 각각 S2·S3·S4 완료 조건이며 S1 완료로 주장하지 않는다.

### S2 — 최소 Graph 편집·왕복·실행

- 기능 Branch 후보: feature/dialogue-line-node, feature/dialogue-choice-node, feature/dialogue-branch-flow, feature/dialogue-statechange-node, feature/dialogue-end-flow. Files: E/Private/DialogueGraphEditor.cpp, E의 DialogueEdGraphNode/DialogueEdGraphNodeBase/DialogueEdGraphSchema 및 시작/종료/분기 노드, 신규 Choice/StateChange 노드, R/Private/DialogueSubsystem.cpp와 헤더, 최소 테스트용 UI/fixture.
- Change: 새 노드 생성·Details·입력 다중/출력 단일 연결, 저장/재열기 왕복, Line·Choice·Branch·StateChange·End 해석, 직접 다음 타입 확인, 자동 Branch 순환/잘못된 연결 방어를 연결한다. StateChange는 게임 요청 경계까지 시험하고 저장 안전성은 S3에서 검증한다. 기본 Choice 표시/빈 조건/End 처리는 확정 규칙을 따른다.
- Validation: DialogueKit.Graph 및 DialogueKit.Runtime. Line→Choice 분기→Line→End, Branch 양쪽, 정상 사용자 루프, 자동 순환, 빈 Choice 정상 종료, Branch/조회 오류 로그, ID/Pin/Details의 저장·재열기 보존. S2 필수 기준은 화면 없는 실행·Editor 저장 왕복·UI 데이터 전달/이벤트 바인딩의 자동 검증까지다. 실제 시각·입력 관찰은 S6의 필수 기준으로 이관하며 S2에서 확인 완료로 표시하지 않는다.
- Exit: 최소 실행 Graph를 직접 작성/저장/다시 열고 진행할 수 있다. 검색·스토리보드 등 고급 도구를 이 단계에 전부 포함하지 않는다.

### S3 — 효과·체크포인트·누적 이력

- 기능 Branch 후보: feature/dialogue-effect-execution, feature/dialogue-save-coordination, feature/dialogue-read-history, feature/dialogue-checkpoint-resume. Files: R의 DialogueSubsystem, ShownDialogueSaveData, 신규 실행 상태/체크포인트/누적 이력 클래스, 샘플 게임 저장 조율자·제공자, 테스트. 기존 PlayerProgress/Quest 저장 연결은 게임 소유 계약으로 분리하되 무관한 게임 시스템을 재설계하지 않는다.
- Change: 효과 일괄 성공/실패 무변경, 실행 식별자 기반 중복 전달 방어와 정상 반복 허용, 같은 시점 스냅샷·FIFO·재시도·세대별 정리, End 캐시/저장 구분, 읽은 Line 버전, 확보된 동일 Block/버전에서 복원을 구현한다. 저장 중 진행과 표시 알림을 제공한다.
- Validation: DialogueKit.Checkpoints. 고정 A스냅샷 이후 B상태 변경, 선행 저장 실패와 후속 대기, 저장 성공 이후 버퍼 정리 범위, 중복 재전달/새 정상 실행, 저장된 효과 재실행 없음, 미저장 상태 미혼합, 사망 후 읽음 유지, 미열람 갱신. 실패 주입 메모리 조율자와 별도 테스트 슬롯의 실제 파일 재실행을 모두 사용한다.
- Exit: 실행/게임 상태/재개 위치가 같은 성공 저장에서 복원된다. 비동기 완료 전 강제 종료로 마지막 성공 저장으로 돌아가는 한계를 그대로 유지한다.

### S4 — 후보·비동기 로드·콘텐츠 버전 복원

- 기능 Branch 후보: feature/dialogue-metadata-filter, feature/dialogue-candidate-selection, feature/dialogue-session-state, feature/dialogue-latest-load, feature/dialogue-version-restore, feature/dialogue-block-replacement, feature/dialogue-story-reset. Files: R의 DialogueGraph/DialogueSubsystem/DialogueSettings, 신규 metadata/selection/version mapping 코드, Config/DefaultGame.ini, Plugins/DialogueKit/Config/DefaultDialogueKit.ini, 테스트.
- Change: target 정확 일치·소속/제한 없음·조건 metadata, 우선순위/균등 후보·기본 순회, 세션 수명, 요청 최신 교체, 구버전 정확 복원/명시적 교체와 게임 보정, 스토리 초기화, 공통 인사 soft asset 설정. 후보 선정과 활성 복원 API를 분리한다.
- Validation: DialogueKit.Selection, DialogueKit.Restore. 로드 전 조건 검사·선정 하나만 로드, 결정 가능한 테스트 난수로 후보집합/동률 확인, A→B→C 콜백 역전·즉시 완료·공유리소스·해제, 소속 없음 제외, 구버전과 매핑범위/충돌/누락, 보정 실패, 초기화 후 신규 선정. 실제 cook 결과의 Registry 정보와 복원용 에셋 포함 확인.
- Exit: 새 대화와 저장 복원이 섞이지 않고 패키지용 데이터에서 후보와 복원 대상을 찾는다. 현재 조건 재검사로 활성 저장을 다른 후보로 바꾸지 않는다.

### S5 — 제작 도구 완성

- 기능 Branch 후보: feature/dialogue-line-inheritance, feature/dialogue-reroute, feature/dialogue-duplicate, feature/dialogue-search, feature/dialogue-validation-panel, feature/dialogue-storyboard, feature/dialogue-storyboard-move-delete, feature/dialogue-read-renewal. Files: E의 GraphEditor/Schema/Commands/노드 위젯, 신규 검색·진단·스토리보드·Details, S1/S4의 Chapter/Scene 정의·범위 metadata를 사용하는 편집 연결부, Editor 테스트.
- Change: 드래그 생성/연속 화자 원본선택·상속, 경유점, Ctrl+D 복제(내부 연결만), Ctrl+F와 메뉴 검색(화자/대사 AND, Choice 별도), 저장/수동 Graph 검사, 노드 이동 결과목록, 스토리보드 Chapter/Scene 등록·Block 조회·이중 삭제 확인·소속 이동·미출시 Block 선택 삭제, Line 미열람 갱신·시간 일괄 편집.
- Validation: DialogueKit.Editor. Undo/Redo·취소 무변경·새 ID·출력 교체, 상속 필드만 복사, 검색 혼합 입력/결과 없음 안내, 진단 클릭이동, 빈 Choice 저장 불가, 두 단계 삭제 취소/선택 범위/실패표시, 소속 없음 신규 제외. 삭제 시험은 격리 fixture만 사용한다. 실제 Editor UI 확인과 저장 왕복 증거를 남긴다.
- Exit: 저작 변경이 runtime/metadata에 일치하며 사용자용 검사·탐색·재배치가 동작한다.

### S6 — 연출·Block PIE 도구

- 기능 Branch 후보: feature/dialogue-text-timing, feature/dialogue-portrait-presentation, feature/dialogue-audio, feature/dialogue-block-preview. Files: R의 DialogueSubsystem/DialogueSettings/ActionSequencer/PortraitSubsystem/DialoguePortraitData/PortraitItemWidget, 필요한 UI 연결·음향 코드, E의 테스트 메뉴·프리셋·PIE 연결, 격리 테스트 월드/위젯.
- Change: 글자당/전체 출력 시간 상호배타 편집과 0 즉시 출력, 입력 시 음성 중지·전체 표시·선택 동시 노출 및 중복 입력 방어, 이미지/표정/위치/방향·BGM/SFX/음성, 툴바 DialogueMaker 테스트 목록, 실패 조건 안내, 테스트 중 편집 잠금·실제 저장/게임효과 격리.
- Validation: DialogueKit.Presentation/Preview의 비시각 로직 테스트와 실제 PIE의 출력 시간/입력/초상화/음향 확인을 구분한다. 테스트 종료는 PIE 생명주기를 사용한다. 실제 게임 세이브/에셋 자동 저장이 없는지 전후 상태를 대조한다.
- Exit: 사용자용 Block 단독 테스트에서 확정된 연출/조건/흐름이 보이고 게임 상태와 저장에 영향이 없다. 수동 관찰이 필요하면 증거 수신 전 Verified로 올리지 않는다.

### S7 — 플러그인 전체 통합·배포 검증

- 기능 Branch 후보: feature/dialogue-localization-compat, chore/dialogue-release-regression. Files: 기존 소비처/지역화 CSV·DataAsset·Subsystem, 샘플 호스트/가이드, 테스트 및 필요한 Build/설정, Blueprint와 본 계획.
- Change: 기존 UDialogueGraph 사용처·UI 공개 함수·CSV/지역화 ID 연결을 점검하고, 샘플 게임 어댑터를 통해 전체 저장/선정/실행 계약을 검증한다. 실제 Prototype2 통합과 별개임을 명시한다.
- Validation: 관련 DialogueKit 전체 등록 테스트, 실제 에디터→저장→재열기→PIE→테스트 슬롯 재개, 복수 이전 버전 fixture·매핑·보정·초기화·읽음·강제 종료, Win64 cook/package 후 실행·조건 metadata/구버전 의존 리소스 포함. Editor/Shipping 차이와 개발용 태그 주입 유출도 점검한다.
- Exit: 모든 단계 필수 증거·미검증·제품 검토 상태를 취합한다. 실제 Prototype2·Sentry 외부 수신 검증은 수행 여부를 따로 표시하며 샘플 성공으로 완료 처리하지 않는다.

## Prototype2 인계 경계

- Prototype2/Blueprint.md의 기존 필수 논의와 저장·조건·효과 계약을 인계 기준으로 유지한다. 현재 작업에서는 Prototype2 파일을 수정하지 않는다.
- 플러그인의 버전/계약이 고정되면 별도 계획/ImplementationBrief에 게임 상태 제공자, DataTable 정의 연결, 통합 FIFO 저장, Chapter/Scene·스토리 초기화, 게임 Sentry 어댑터와 설치 방식을 기록한다.
- 시스템 배치·세부 사망 초기화 등 미결정 게임 기능은 여기서 추정하지 않는다. Prototype2 구현/플러그인 설치는 해당 프로젝트의 인계와 승인 범위가 갖춰졌을 때 별도 Branch에서 수행한다.
- 새 프로젝트·개별 플러그인 분리가 결정되면 해당 Blueprint와 기존 역할 구성을 참고한 에이전트 지침을 먼저 작성한다. 현재 계획은 새 프로젝트 생성을 승인하거나 요구하지 않는다.

## Uncertainty & PoC Track

| 불확실성 | 최소 실험과 성공 기준 | 실패 신호·전환 규칙 |
|---|---|---|
| Graph/ID 왕복 | 새 fixture 저장·재열기 후 ID/핀/값 동일 | 유실/중복이면 S2 안에서 변환 경로 수정, 제품 규칙 변경 필요 시 반환 |
| 저장 원자적 스냅샷 | 효과 성공 시 상태/위치 캡처, 실패/지연 후 같은 자료 재시도 | 서로 다른 시점 혼합이면 S3 통과 금지, 실파일 검증도 통과 후 본 흐름 사용 |
| cook 메타데이터/보존 | 작은 신규/복원전용 fixture로 cooked registry와 에셋 조회 | 에디터에서만 성공하면 S4 미검증, 검색/포함 설정 수정 후 재실험 |
| 실제 UI/PIE | 최소 Line+Choice 위젯 실제 입력/노출 확인 | 내부 바인딩 미확인/화면수단 부재는 Pending, 무화면 성공으로 대체 금지 |

PoC 코드는 격리 테스트 경로에서 시작한다. 성공한 최소 구조만 정식 코드로 정리하고 임시 하드코딩은 제거한다. 실패 실험을 제품 완료로 남기지 않으며 기존 사용자 에셋을 폐기 대상으로 삼지 않는다.

## Risks & Mitigations / Approval Points

- 합의된 신규 스키마·API 구현은 계획 실행 범위에 포함한다. 매 단계 같은 구조 변경 허락을 다시 받지 않는다. 다만 지원 저장을 이전 경로 없이 깨거나 합의와 다른 데이터 의미로 바꿔야 하면 영향 범위와 대안을 제시한다.
- 기존 에셋의 실제 삭제·이름 변경·덮어쓰기, 엔진 변경·새 외부 플러그인, 원격 목적지 변경, 데이터 소실 Git 동작, Prototype2의 미확정 게임 정책/승인 밖 통합은 자동 진행에서 제외한다. 삭제 기능 구현과 실제 사용자 에셋 삭제 실행은 다르다.
- 기존 미커밋 변경, 실행 중 에디터/빌드, 인증/환경 제한은 강제 해소하지 않는다. 가능한 독립 작업을 계속하고 필요한 경우 구체적인 장애만 보고한다.
- 샘플 게임은 게임 계약 검증용이며 실제 게임 시스템이 아니다. 저장 조율 책임을 플러그인으로 이동시키거나 샘플 제공자로 조용히 대체하지 않는다.
- 미검증 결과의 무조건 진행을 금지하지만, 사소한 불확실성을 새 제품 질문이나 반복 승인으로 확대하지 않는다.

## Backward-compat Matrix

| 표면 | 기존/신규 차이 | 전략 | 검증 |
|---|---|---|---|
| 프로토타입 Graph | 내장 Choice→독립 Choice/StateChange, 새 안정 ID | 사용자 수동 재작성, 자동 대량 수정 없음, 에디터 실패 시 명확한 진단 | 새 fixture 왕복 및 기존 소비처의 안전한 실패/연동 |
| 공개 UDialogueGraph/Blueprint API | 기존 클래스 소비 유지, 계약 확장 | 호출처 조사·연결부 보완·필요한 호환 래퍼, 조용한 의미변경 금지 | 호스트 컴파일·실제 UI·CSV |
| 프로토타입 열람 저장 | 이름+GUID→BlockID/LineID/열람버전 | 기존 테스트 이력 자동이전 요구 없음, 테스트 슬롯 분리 | 누적 이력/새게임/사망 차이 |
| 새 지원 저장 | 저장 버전/활성 Block/선택/효과 상태 | 버전별 reader·유효성 검사, 마지막 성공 저장 보존 | 손상·중간종료·이전 버전 fixture |
| 출시 콘텐츠 버전 | 신규 노출 중단 및 대체 | 구버전·의존 리소스 보존, 명시적 매핑·게임 보정 | 정확 복원·매핑범위·초기화 후 신규 |
| 지역화 | 이름/Node·Pin 의존 키 | ID 대응을 명시하고 버전/키 유실 방지 | CSV→DataAsset→실행 왕복 |

## Rollback Plan

- 기능 시작 전 Codex_Develop의 BaseSHA를 기록하고 작은 커밋을 순서대로 보존한다. 기능 tip과 통합 merge commit은 원격 기능 Branch/Codex_Develop에 남긴다. merge 전 검증 결과, merge의 첫 부모/두 번째 부모, 적용된 저장 스키마 버전을 추적한다.
- 기능 일부 취소: 작은 커밋의 변경·선행/후속 의존을 확인해 `git revert --no-commit <CommitSHA>`로 역변경을 만든다. 여러 커밋이 의존하면 후속부터 역순으로 처리한다. 단일 커밋이 항상 독립 취소 가능하다고 가정하지 않는다. 공통 API를 되돌릴 때 소비처도 같은 복구 작업에 포함한다.
- 기능 전체 취소: Codex_Develop 첫 부모를 기준으로 한 merge임을 확인한 뒤 `git revert --no-commit -m 1 <MergeSHA>`로 해당 기능의 통합 변경을 제거한다. merge 이후 의존 기능이 있다면 영향받는 기능부터 역순으로 복구하거나 호환 수정으로 해결한다. -m 1을 부모 확인 없이 기계적으로 적용하지 않는다.
- 이미 통합한 오류의 복구는 `fix/dialogue-revert-<기능>`을 Codex_Develop에서 만들어 revert·빌드·회귀 검증 후 --no-ff merge·일반 push한다. 통합 직후 로컬 검증 실패로 Codex_Develop가 잠시 실패 상태라면 해당 merge를 이력 보존 revert하고 마지막 정상 동작 복구를 검증한 후에만 push한다. 미검증 통합 tip에서 새 기능을 시작하지 않는다.
- 기능 수정 후 재도입: 원래 Branch를 다시 merge하는 것만으로 revert한 변경이 돌아온다고 가정하지 않는다. 새 fix/feature Branch에서 필요한 역-revert와 수정 또는 새 구현을 검토하고 회귀 검증한 뒤 통합한다.
- 개발자가 이력에서 커밋 자체를 제거해야 하는 경우 BaseSHA·작은 커밋·merge 경계 기록을 제공한다. 에이전트는 범위가 명확히 승인되지 않은 reset/rebase/force를 수행하지 않는다. 원격 공유 이력의 기본 복구는 삭제가 아닌 revert다.
- Git은 코드/추적 에셋을 되돌릴 뿐 이미 변경된 게임 세이브나 외부 상태를 되돌리지 않는다. 테스트는 별도 슬롯·디렉터리·이전 버전 fixture로 격리하고 지원 데이터에 대한 reader/호환 검증을 유지한다. 코드 rollback 후 이전/신규 fixture 양쪽의 저장을 읽을 수 있는지도 확인해야 복구 완료다. 지원 저장에 필요한 reader/이전 코드를 제거하는 단순 revert는 하지 않고 호환 복구안을 먼저 만든다.
- 각 기능은 완료 전 복구 단위(커밋/merge)와 의존 목록을 검토한다. 스키마·공통 API 등 영향이 큰 기능은 격리된 검증 공간에서 revert 후 빌드/이전 fixture 복원까지 점검한다. 복구 검증이 없으면 무조건 복구 가능하다고 보고하지 않는다.
- push 실패는 로컬 커밋을 보존하고 동일 SHA의 일반 push를 재시도한다. 실제 사용자 변경과 세이브를 복구 실험에 사용하지 않는다.

## Progress Log / Resume Record

아래 초기 중단 표는 S1 재개 전의 역사 기록이다. 최신 진행 상태는 문서 하단의 S1 최종 결과를 따른다.

| 단계/기능 | 상태 | 기능 Branch / 통합 Branch | BaseSHA / FeatureTip / MergeSHA | 검증·Commit·Push 근거 |
|---|---|---|---|---|
| 계획 | 작성 완료, 실행 미착수 | DiagnoseGraphSystem | d1b1937 / 해당 없음 | 문서 검증만 수행 |
| S0 테스트 기반 | Integrated | chore/dialogue-test-harness / Codex_Develop | 7601263 / 5a24cbd / 84ac3db | 3 UE 테스트·9 보고서 검사 통과; 실제 missing filter 실패 판정 확인 |
| S0 패키징 기반 | Integrated | chore/dialogue-package-validation / Codex_Develop | 76e4fb5 / d150dfc / d7517d5 | Package_20261014_174122_a18bf895: UAT 0, 새 exe/pak 확인, Error 0 |
| S1 안정 Block 식별자 | Integrated / 사용자 요청으로 중단 | feature/dialogue-block-identity / Codex_Develop | d7517d5 / 구현 9db7760 / 이 행을 포함한 통합 커밋 | Editor 빌드·7개 UE 테스트·Game cook/package 성공 |
| S1 나머지~S7 | Planned / 사용자 요청으로 대기 | 기능별 Branch / Codex_Develop | 미정 | 미착수 |

실행 시 기능별 행으로 분리하고 작은 커밋 목록·의존 커밋·복구 단위·Codex_Develop의 merge/검증 SHA를 함께 기록한다. 다음 정보를 갱신한다: 실행 승인 원문/일시, 현재 Branch·HEAD·BaseSHA, 변경 파일, 완료/미완료 체크, 테스트 명령·보고서·실패, 기술 검토/제품 검토 상태, Commit/Push SHA, 다음 한 가지 작업. 커밋 SHA는 커밋 후 보고 또는 다음 기록에서 반영하며 자기 SHA를 넣으려고 amend하지 않는다.

- 실행 승인: 2026-10-14 사용자 “이제 앞서 논의한 내용을 순서대로 스스로 작업해봐.”에 따라 구현·검증·문서·기능별 Git 통합을 시작했다. 기준 빌드: Saved/AI/20261014_172137_a97d4dc2/Build.log, ExitCode 0. 기존 EnhancedInput 선언 경고는 분리 추적한다.

- S0 테스트 기반 검증: Build `Saved/AI/20261014_172742_d4a3b189`, UE 3개 Success `Saved/AI/20261014_172756_47af708b`, 합성 보고서 9개 `Saved/AI/ReportValidation_cdacc1f12b6c4d209c18cd64a5429655`, 의도한 실패 `Saved/AI/20261014_172830_9f259835`(EditorExit 255 / NoReport / wrapper 비정상 종료). 기존 6종 초기화 오류 12줄만 확인, 신규 오류 0. Sol Medium 검토 통과. 사용자 안내서 1~2절 작성. 다음 작업은 별도 package-validation 기능이며 UI·cook 검증은 아직 미실행이다.

- S0 컴파일 수정 통합: fix/dialogue-runtime-enum-metadata, Base 84ac3db / Feature 0cece7c / Merge 76e4fb5. Game 빌드 `GameBuild_0d6864bcb4684197a44475942223898d`, Editor `20261014_173826_7b801094`, UE 4개 `20261014_173845_2a23088a` 통과. 통합 후 핵심 1개 `20261014_174016_c64237d2` 통과. 기존 초기화 진단 12줄은 별도 미해결이며 신규 오류 0. 수정/통합 브랜치 push 완료.
- S0 패키징 재개: WIP 3ef1d26에 통합 수정 반영 33f3306. `Package_20261014_174122_a18bf895`에서 UAT 0, 새 exe/pak, Error 0 확인. 사용자 안내서 4절과 실행 명령 작성. Sol Medium 독립 검토 차단 없음. timeout 소유 프로세스 종료는 정적 검토만 수행. GUI/PIE/실제 패키지 실행 미검증. 다음 작업은 S1 안정 식별자이며 패키징 기능 통합 후 시작한다.

### 2026-10-14 사용자 중단 요청과 재개 위치

사용자 요청: “우선은 현재 작업 중인 내용까지만 작업하고 멈추자.” 안정 Block 식별자 기능의 문서·검증·Git 통합까지만 마무리하고 후속 구현을 시작하지 않는다. 재개는 사용자의 다음 요청 후 진행한다.

| 순서 | 구현 계획 | 중단 시 상태 |
|---|---|---|
| S0 | 기준 빌드, 회귀 테스트, 실패/0개 보고 판정, 패키징 실행 기반 | 완료·통합 |
| S1-1 | 안정 Block ID/콘텐츠 버전 참조, Factory/복제, 카탈로그 검사 | 완료·통합 / 여기서 중단 |
| S1-2 | Node/Choice 식별·핀 대응, 저장/열람 버전 구분의 나머지 | 미착수 |
| S1-3 | Tag/Value/Count 정의·조회와 조건 AND/OR, 게임 샘플 제공자 | 미착수 |
| S1-4 | 효과·오류값·저장 스냅샷 계약 | 미착수 |
| S1-5 | Chapter/Scene 정의·소속·제한 없음 자료형 | 미착수 |
| S1-6 | 범용 진단 이벤트·게임 로그 연결 계약 | 미착수 |
| S2 | Line/Choice/Branch/StateChange/End 최소 편집·연결·실행·왕복 | 미착수 |
| S3 | 효과 실행·중복 방어·FIFO 저장·체크포인트·열람/복원 | 미착수 |
| S4 | 메타데이터 필터·후보·최신 요청 로드·버전 보존/교체·스토리 초기화 | 미착수 |
| S5 | 드래그/상속·경유점·복제·검색·검사 UI·스토리보드·미열람 갱신 | 미착수 |
| S6 | 텍스트 시간·초상화·음향·입력·Block 단독 PIE 도구 | 미착수 |
| S7 | 기존 소비처·지역화·전체 통합·배포/실행 검증 | 미착수 |

- S1-1 근거: Build `20261014_174737_40e22f64`, UE `20261014_174821_6f70a734` 7개 성공(기존 진단12/신규0), Package `Package_20261014_174841_e8bda23c` UAT0/새 exe·pak/Error0. Sol Medium 독립 구현 검토 차단 없음. 사용자 안내서 5절. 기존 에셋/저장 포맷/PrimaryAssetId 소비처는 유지한다.
- 재개 시 우선 새 필드를 기존 NodeGuid/PinId 및 신규 Choice 항목에 연결하는 S1-2 범위를 조사·확정하고 독립 기능 브랜치에서 진행한다. 별도 의미가 없는 Node ID를 중복 도입하지 않는다. S1-1은 안정 ID 기반이며 저장·지역화·후보 선정 전환 완료가 아니다.
- 후속 검증: S4에서 실제 legacy package fixture와 persistent Registry 카탈로그 연결을 검사한다. 일반 DuplicateObject는 새 Block이므로 같은 Block의 새 콘텐츠 버전 저작은 전용 경로가 필요하다. 기존 초기화 오류 6종/12줄, 실제 UI/PIE/패키지 실행, GameDirector 제품 검토(Unavailable)는 미완료로 유지한다.
- Prototype2 구현/설치 및 게임 Sentry 수신 검증은 이번 작업에서 수행하지 않았다.

## Decision Log / Discoveries

- 2026-10-13: 제품 동작 논의 완료. 데이터·최소 편집/실행을 앞당기고 저장·효과 검증과 버전 해석을 분리하는 7단계 경계를 정리했다.
- 2026-10-13: 당시 Sol Medium 독립 검토에서 Branch 기준점 고정, Commit/Push와 Verified 분리, 승인 범위/반환 조건 명시를 권고했다. Luna High 조사에서 현재 스크립트가 Check/Build/Test만 지원하며 GUI/패키징 경로와 자체 기능 테스트 준비가 필요함을 확인했다.
- 단계별 검증이 끝나야 의존 후속으로 넘어간다. 실제 실행 가능성을 정적 검토만으로 보장하지 않는다. 최종 검토에서 계획 미커밋 시 초기 Branch/보존 절차, S1의 Chapter/Scene 자료형 선행, S2의 비시각 UI 계약 검증과 S6의 실제 관찰 분리를 보완했다.

- 2026-10-14: 사용자 지시로 단계별 연쇄 Branch를 폐기하고 Codex_Develop 중심 기능별 분기/--no-ff merge로 변경했다. 작은 목적별 커밋, 기능 Branch 보존, 부분/전체 revert와 의존성·저장 호환 검증을 필수 복구 지침으로 추가했다. 현재는 지침 변경이며 실제 Branch/Commit/Merge/Push를 실행한 기록이 아니다.

- 2026-10-14: 최종 문서 검증 및 Stage/Commit/Push 요청을 받았다. 모든 구현·merge·revert 커밋의 Subject/Description 작업 요약을 필수로 명시했다. 이 문서 커밋 요청은 기능 구현 실행 요청과 구분한다.

- 2026-10-14: 사용자 최종 명칭 지시로 개발 통합 Branch를 `Codex_Develop`로 통일한다. 기능 Branch는 여기서 분기하고 검증 후 여기로 merge한다. 기존 중심 Branch 제거는 이번 요청에 한해 명시적으로 승인되었으며, 일반 기능 Branch 보존 규칙은 유지한다.

- 2026-10-14: 사용자 요청으로 항목별 사용 안내서 작성·검증을 완료 조건에 추가했다. Docs/UserGuide.md를 최상단 목차와 구현 항목 순서로 구성하고 공개 API·실제 에디터 UI·게임 측 확장 방법을 구분하여 Git에서 관리한다.

## Verification / Evidence / Result

- 계획 작성 및 최종 검증: Sol Medium 독립 검토에서 제품 합의·단계 의존·기능 통합/복구·Prototype2 경계의 중대한 구조 모순을 발견하지 못했다. 추가 지시인 모든 커밋 메시지 요약 의무를 반영했다. 관련 문서·코드 경로/스크립트 정적 검토와 Windows Git diff 검사. UE 빌드·PIE·자동화·패키징은 실행하지 않았다.
- 구현 시: Saved/AI/<실행ID>/Build.log, result.json, Report/index.json 등 실제 산출물 경로와 테스트 수/결과를 단계 행에 기록한다. 테스트 0개·보고서 없음·미실행은 통과가 아니다.
- 종료 보고: 단계·변경 목적·Branch/커밋/원격 push·통과 검증·미검증·제품 검토·다음 단계. 로그 원문은 필요 최소한만 공유한다.
- 이전 중단 시점 결과: S0와 S1 안정 Block 식별자까지 완료했다. 이후 S1 재개/최종 결과는 아래 별도 기록을 따른다.

## 2026-10-14 S1 재개 승인

- 사용자 지시: 기존 코드 스타일 분석·지침 반영 후 S1 전체 구현. 완료 시 보고하고 다음 요청까지 대기하며 S2는 착수하지 않는다. 이전 중단 기록은 과거 이력이다.
- CodeStyle.md 기준을 새 구현·검토에 적용한다. S1은 Node/Choice 식별·열람 버전, 조건/효과/저장 계약 및 샘플, Chapter/Scene 자료형, 진단 연결까지이며 S2 실행기와 S3 실제 저장 조율을 완료로 표시하지 않는다.
- 각 기능은 Codex_Develop에서 분기·작은 커밋·검증·문서화·일반 merge/push한다. 기존 에셋 일괄 변경과 기존 저장 형식 교체는 하지 않는다. GameDirector 역할은 여전히 미노출이며 제품 검토 미완료로 유지한다.

- S1 Node/Choice 식별: 기존 NodeGuid 유지, ChoiceId↔PersistentGuid↔RuntimePin 대응과 재정렬 연결 보존, ReadVersion 경계 및 순수 검사 구현. 첫 실행에서 AddDialogueChoice에 같은 배열 요소를 입력할 때 assert가 재현되어 로컬 복사 후 추가하도록 수정했다(193301_e85ec019). 수정 후 전체9개 통과(193356_719a6d30), 경계 보완 Build 193437_3b5b650e. 실제 Graph 저장/재열기 UI는 S2 검증 대상. Sol Medium 검토 차단 없음.

- S1 진단 계약: Blueprint/native 이벤트와 서비스 비의존 FDialogueDiagnostic/Context 구현. Build 193723_e0c3f871 및 native 전달·해제/일부 Context 테스트 193759_291fe40b 통과. Sol Medium 검토 후 문서의 BP 이벤트/전체 필드 검증 주장을 낮췄다. 실제 Sentry 전송은 미검증.

- S1 조건 계약: Tag/Value/Count·정의/현재값 인터페이스·AND/OR·공통 DataTable·메모리 샘플 구현. 오류항목 True 대체·0과 조회실패 구분·int64 정밀도·NaN/대상격리·기본실패 검증. Build 194429_7a15d66c 통과. 기존 조건 소비처는 S2에서 전환한다. Sol Medium 검토 반영, UserGuide 8절.

- S1 효과 계약: 순서 있는 묶음의 all-or-none 적용·실행 ID와 의미상 payload 비교·성공/실패 재전달·상태/ledger 메모리 왕복 샘플. 독립 검토에서 발견한 ledger 한쪽 누락을 실패 처리하고 양방향 손상 fixture 추가. Build 195441_21f6f662, Effects 2개 195514_52ea3e78 성공(기존12/신규0). 디스크 exactly-once/FIFO는 S3. UserGuide 9절.

- S1 체크포인트 계약: 논리 스냅샷/물리 요청/완료 분리, 전체 누적열람·완료 일관성, 세대/재시도 상관검사와 비영속 샘플 구현. 첫 빌드의 Interface 기본 구현 선언 누락/테스트 destructor 접근 오류(195846_6e3daff9)를 수정. Build 200059_d505aa9a 성공, Checkpoints 2개 통과. Sol Medium 검토 반영. 실제 FIFO/파일/복원은 S3, UserGuide 10절.

- S1 스토리 소속: Chapter/Scene 정의 에셋·안정 ID, SceneId 정본 소속·부모 조회, 게임 제공 현재 진행과 순수 범위 판정. Build 200646_3bd91110, Story 2개 통과. Sol Medium 검토 경계 테스트 반영. S4 전 기존 Graph 소속을 명시 설정하고, S5 삭제/이동은 transaction과 위치 포함 진단을 제공한다. UI/Registry/후보 연결은 아직 후속 단계이며 UserGuide 11절에 명시했다.

## 2026-10-14 S1 최종 결과 — 완료 후 대기

- 사용자 코드 스타일 분석을 `CodeStyle.md`와 AGENTS.md에 반영했다. S1의 공통 자료형·공개 계약·샘플·검증을 완료했으며 다음 사용자 요청 전 S2에 착수하지 않는다.
- S1 종료 기준은 신규 계약과 메모리 연결 fixture 기준이다. 기존 실행기/저장기/후보의 교체는 S2/S3/S4이고 실제 제품 전체 통합 완료가 아니다. 기존 에셋/저장 포맷 및 Prototype2는 변경하지 않았다.

| 단계 | 구현 항목 | 최종 상태 |
|---|---|---|
| S0 | 기준 회귀 테스트·실패 판정·패키징 기반 | 완료 |
| S1-1 | 안정 Block ID·콘텐츠 버전·생성/복제/검사 | 완료·통합(이전 작업) |
| S1-2 | Node/Choice ID·핀 대응·열람 버전 | 완료·통합 |
| S1-3 | Tag/Value/Count 정의·조회·AND/OR·샘플 | 완료·통합 |
| S1-4 | 효과 묶음·오류 반환·체크포인트 DTO/요청/완료·샘플 | 완료·통합 |
| S1-5 | Chapter/Scene 정의·소속·제한 없음 판정 | 완료·통합 |
| S1-6 | 범용 진단 이벤트·게임 로그 연결 계약 | 완료·통합 |
| S2 | Line/Choice/Branch/StateChange/End·Graph 편집/왕복/실행 | 미착수·대기 |
| S3 | 실제 효과 실행·FIFO 저장·누적 이력·복원 | 미착수 |
| S4 | Registry 필터·후보·비동기 로드·버전/교체 | 미착수 |
| S5 | 검색·복제·경유점·스토리보드·편집 진단 | 미착수 |
| S6 | 연출·입력·음향·Block PIE 도구 | 미착수 |
| S7 | 기존 소비처·전체 통합·배포/실행 검증 | 미착수 |

- 최종 Editor Build: `Saved/AI/20261014_201009_a925ffc3/Build.log` Exit0.
- 전체 DialogueKit 테스트: `Saved/AI/20261014_201031_22d53f72/Report/index.json` 19/19 성공. 기존 초기화 진단12/신규0, PassedWithDiagnostics. 테스트 중단/실패 이력은 지우지 않았고 수정 근거를 위 진행 기록에 남겼다.
- Game Win64 Development 빌드/cook/package: `Saved/AI/Package_20261014_201059_6f35ae4a/result.json` UAT0·새 exe/pak 확인, PackagedNotLaunched.
- 메모리 계약 연결 fixture는 조건→효과→스냅샷 요청의 동일 ID와 복사 격리를 검증한다. 게임 상태+대화 요청의 실제 단일 원자 저장을 증명한 것은 아니며 S3에서 구현·검증한다.
- Sol Medium 기술 검토와 Luna High 문서/스타일 조사를 실제 위임했다. ledger 비대칭 손상·완료 이력 일치·런타임 envelope/영속 DTO 분리·Scene 부모 정본·테스트 실패 접근 방어를 반영했다. GameDirector 역할은 미제공으로 제품 검토 미실행이다.
- GUI/PIE/실제 패키지 실행·음향·외부 Sentry 전송·Prototype2 연동 미검증. 기존 구조체 초기화 진단과 EnhancedInput 의존성 경고는 남아 있다.
- 후속 주의: S2에서 새 노드/조건 소비처를 연결하고 Graph 파일 왕복 검증, S3에서 불변 통합 저장/FIFO/완료 대기 추적, S4에서 legacy 소속 명시 및 정의 에셋 선택·메타데이터, S5에서 Scene 이동/삭제 transaction·진단 위치를 구현한다.
- 사용법: `Docs/UserGuide.md` 5~12절. 변경 범위·검증·기능별 merge/복구 기준은 `Docs/Reports/S1-2026-10-14.md`를 따른다. 각 기능 Branch는 원격에 보존하며 통합 Branch는 Codex_Develop다.

## 2026-10-14 S2 실행 승인과 세부 계획

- 사용자: “S2 단계까지 진행하고 작업 내용을 정리해서 나한테 출력해.” 기준 Codex_Develop 5ae3a57. S2만 구현·검증·문서·기능별 Commit/Push하며 S3는 시작하지 않는다.
- 기존 사용자 에셋은 자동 변환하지 않는다. 기존 DialogueNode 타입/Info/API를 남겨 읽기 호환을 유지하고 신규 Line/Choice/StateChange 타입을 추가한다. 공통 실행기는 UDialogueGraph를 직접 받는 테스트 가능한 UObject로 분리하고 Subsystem 공개 연결 및 UI 프레임 이벤트를 제공한다.
- 기능 1 Line: 신규 단일 발화 노드/Details·단일 출력, Start-Line-End 최소 실행/직접 다음 타입, Editor↔Runtime 공통 변환과 패키지 왕복 fixture. 기존 Line 표현에 쓰는 UDialogueNodeInfo의 공개 읽기 API와 ID는 재사용하며 legacy 선택 편집은 새 Line에서 숨긴다.
- 기능 2 Choice: 독립 항목/조건/기본 항목/ID별 출력, 입력 다중·출력 단일 교체, 현재 Line의 직접 Choice 조회 및 실제 선택 검증. 기본/빈 Choice는 확정 규칙 적용.
- 기능 3 Branch: S1 조건 제공자 연결·True/False·빈 조건 진단·자동 구간 Branch 재방문 차단. 사용자 입력 후 정상 반복은 허용한다.
- 기능 4 StateChange: 실제 진입에서만 게임 효과 묶음을 요청하고 실패 반환·진단한다. 공개 효과 완료 경계를 제공하되 FIFO/체크포인트 파일 저장은 S3에 남긴다. StateChange 자동 순환 추가 정책은 구현하지 않는다.
- 기능 5 End/통합: 정상 완료/오류 중단 구분·Save on End 기본false 메타데이터·게임 연결/UMG 데이터 이벤트, 전체 노드 저장/재열기 및 실행 테스트. 기존 게임 저장/Quest를 신규 실행기에서 자동 호출하지 않는다.
- 기능마다 기능 Branch, 작은 구현/테스트 및 사용법 커밋, 독립 Sol Medium 검토, Editor Build/관련 자동화 후 --no-ff 통합 및 push. 최종 전체 회귀/패키징. 원격 기능 Branch와 Base/Tip 복구 기록 보존.
- PoC: Editor 공통 변환 API로 임시 패키지 저장/언로드/재열기를 자동 시험한다. ID/노드 속성/연결 유실이나 잘못된 소유 관계는 수정 후 본 경로에 적용한다. GUI 창/실제 음향은 S6 관찰 대상이며 S2는 비시각 데이터 바인딩을 검증한다.
- 조사: 기존 변환은 RuntimeNode Outer가 작업용 EditorGraph인 문제와 입력/출력 연결 검증 부재가 있다. 런타임 객체는 영속 RuntimeGraph 하위로 소유하고 변환 실패 시 이전 assetGraph를 유지한다. 조회가 ID를 생성하거나 진행/효과를 변경하지 않게 한다.
- 제품 검토: GameDirector 역할은 여전히 제공되지 않아 미실행이다. 기존 확정 설계만 구현하며 Sol Medium 기술 검토와 Luna High 조사를 실제 위임한다.

- S2 Line: 공통 Codec/독립 실행기/단일 출력/저장 전 검사/GC 수명 보완. Build 220457_fd859d4a, 전체21개 220525_bf08e25e 통과(기존12/신규0). 첫 파일 왕복 테스트의 임시 마운트 조기 해제는 지연 검증을 고려해 프로세스 수명까지 유지하도록 수정했다. GUI는 미검증.
- S2 Line 최종 보완: legacy/Line 본문 혼동 및 손상 Start/End 거부, 출력 교체/입력 합류 검증 추가. Build 220821_45ec5056, 전체22개 220835_ac3a7b68 통과. Sol Medium 지적 반영.
- S2 Choice: 독립 선택 목록/고정ID핀/필터링/기본fallback/정상루프 구현. 초기 Codec 타입 누락 테스트 실패(221119_b227cf41)를 수정, 전체23개 221243_bb91b145 통과. 표시전 숨은 ID 실행 및 대기중 전체탈락 방어 보완 후 Build 221328_c1b84e96/Runtime2개 221426_6ab8f205 통과. Sol Medium 검토 반영. 외부 상태 변경 알림 기반 선택 UI 갱신은 S6 연동에서 확인한다.
- S2 Branch: 공통 조건과 실제진입 분기, 입력 경계별 방문 집합, 자동순환 재평가 전 중단. Build 221754_c03d3f89, 전체24개 221608_865e758a 및 최종Runtime3개 221830_290dec71 통과. Sol Medium 지적에 따라 Choice 대기시 직접다음조회 Unknown, True/False 핀검사와 입력을 거친 Branch 재방문 검증 추가.
- S2 StateChange: 상태변경 저작/실제진입 효과 및 Choice효과/저장플래그, 성공 직후 후속진입 전 경계 구현. Build 222328_8846ebb3, 전체25개 222038_f6464f55 및 최종Runtime4개 222358_b1c0b2e2 통과. Sol Medium 지적의 no-effect ExecutionId를 S1 계약과 일치시키고 true/false 저장플래그·잘못된효과 무변경 시험 추가. 디스크 저장안전성은 S3이며 이벤트 접수실패 보완도 S3 책임이다.

- S2 End/연동 완료: 기본 SaveOnEnd=false, 정상완료 공통경계·혼합Graph선검사·Subsystem/동적UI·독립Choice CSV 연결. Build 222919_439f1121, 전체27개 223011_a7bc1397 통과(기존12/신규0), 최종 Package_20261014_223102_2722c993 UAT0/새exe·pak/Error없음/실행미검증. Sol Medium 최종 차단 없음. 실제 GUI/PIE·음향은 S6, 저장안전은 S3, 전체 지역화/배포 회귀는 S7에 남긴다.

## 2026-10-14 S2 최종 결과와 대기

- S2 완료 보고서: `Docs/Reports/S2-2026-10-14.md`. 사용 안내: `Docs/UserGuide.md` 13~17절.
- 기능 통합: Line f6e241d → Choice 31dd1f7 → Branch 7767987 → StateChange 6eb3614 → End/연동 a4d54e5. 각 feature 브랜치와 작은 커밋을 보존하고 원격에 push했다.
- 최종 빌드 222919_439f1121, 자동화27개 223011_a7bc1397, 패키징 Package_20261014_223102_2722c993 성공. 기존 오류12/신규0. 실제 화면/음향·패키지 실행·S3 저장은 미검증 범위다.
- S0/S1/S2 완료. S3~S7 미착수. 위의 과거 시점별 미착수 표는 작업 이력이며 현재 상태는 이 절을 기준으로 한다.
- S3 인계: 성공 경계에서 불변 통합 스냅샷 확보·큐 접수실패/dirty 유지·재시도·복원, 완료/열람 캐시 연결. S5 인계: Save All/AutoSave까지 포함한 검사/저장 흐름, 조건 편집에서 Legacy 표시 명확화. S6 인계: 외부 상태 변경 알림에 따른 선택 UI 갱신과 실제 Widget·입력·연출.
- 사용자 범위에 따라 여기서 보고·대기하며 후속 단계는 시작하지 않는다.

## S3·S4 실행 재개 (2026-10-15)

- 승인: 사용자의 S3·S4 구현 시작 요청. S3를 기능별 검증·통합한 뒤 S4를 진행한다. 시작 상태는 깨끗한 Codex_Develop이다.
- 조사: Sol Medium은 저장 경계·원자성, Luna High는 Registry·후보·복원을 읽기 전용으로 조사했다. 메인만 구현한다.
- S3 순서: 효과 상태/실행 원장 캡처·복원 → 게임 소유 FIFO 샘플 → 열람/재개 위치 연결 → 실제 격리 슬롯 복원 검증.
- S4 순서: Registry 메타데이터 → 후보/세션 → 최신 비동기 요청 → 정확 버전/명시적 교체 → 스토리 초기화 → cook 검증.
- 호환성: 기존 에셋·저장 형식을 덮어쓰지 않으며 신규 계약과 샘플 포맷을 사용한다. 기존 공개 API를 유지한다.
- 주요 위험: 불변 스냅샷 접수 전 상태 변경, 실패 head 추월, 늦은 완료 콜백, 구버전 복원과 신규 후보 선정 혼합. 각 기능 테스트로 차단한다.
- 검증: Scripts/AI/Invoke-UE.ps1의 Build/Test, DialogueKit.Checkpoints·Selection·Restore와 전체 회귀. 실제 파일과 cooked Registry는 별도 증거가 있어야 통과다.
- 복구: 기능별 branch와 작은 커밋을 보존하고 Codex_Develop에 --no-ff 통합한다. 의존 후속 기능부터 역순 revert하며 이력 삭제는 하지 않는다.
- 제품 검토: 현재 game_director 역할이 제공되지 않아 실제 제품 일치 검토는 미실행이다. 다른 역할로 대체하지 않는다.
- 현재 진행: feature/dialogue-effect-execution의 샘플 효과 상태·원장 캡처/복원. S3 전체와 S4는 아직 완료되지 않았다.


## 2026-10-15 S3·S4 중간 결과와 확인 대기

현재 상태의 정본은 이 절이다. 위 시점별 표와 S2 대기 문구는 과거 이력이다. 상세 결과는 `Docs/Reports/S3-S4-2026-10-15.md`, 사용법은 `Docs/UserGuide.md` 18~22절을 따른다.

| 단계/항목 | 현재 결과 |
|---|---|
| S0~S2 | 이전 완료 유지 |
| S3 효과 상태·원장 | 캡처/검증/복원 및 재전달 방어 완료, merge 8659384 |
| S3 FIFO·비동기 샘플 | 불변 요청·실패 대기·재시도·세대 검사·실파일 합본 저장 완료, merge 4f80368 |
| S3 누적 열람 | feature/dialogue-read-history의 3ac33bd에 WIP 보존, 미통합·승인 대기 |
| S3 Runner 재개·전체 저장 연동 | 미완료. End 임시 기록 정리/선택 기록 검증과 구 스키마 이전 승인 필요 |
| S4 metadata | 정확 대상/소속/조건 무로딩 조회·RestoreOnly 제외·cook tag 검사 완료, merge 74c0ce6 |
| S4 후보/세션 | 최우선·일반 우선순위·기본 순회·명시적 세션 종료 완료, merge 3afb7ee |
| S4 최신 비동기 로드 | 독립 로더·요청 교체/취소·재진입/공유핸들·공통 인사 설정 완료, merge f90b348 |
| S4 버전 복원/명시적 교체/스토리 초기화 | S3 복원 연결에 의존하여 미완료. 실제 복원용 cook 에셋 검증도 남음 |
| S5 / S6 / S7 | 제작 편의 / 실제 연출·PIE / 통합·배포 회귀 미착수 |

### 차단 근거와 다음 조치

자동 승인 검토가 체크포인트 SchemaVersion 1→2 및 복원 의미 변경을 별도 사용자 확인이 필요한 저장 형식 변경으로 거부했다. 거부된 변경은 적용하지 않았다. 기존 1 스키마를 유지하며 WIP를 Codex_Develop에 merge하지 않았다. 초안 테스트 31개 통과만으로 완료 처리하지 않는다. 발견된 결함은 (1) End 스냅샷 복원 시 임시 Choice 기록 재등장, (2) Choice 트리거와 실제 선택 레코드의 일치 검증 누락이다.

확인할 구체 변경: 새 체크포인트를 스키마 2로 기록하고, 구 스키마 1의 알려진 마지막 선택을 이전하며, 정상 완료된 End 스냅샷 복원 시 그 저장에 포함된 임시 선택 기록을 정리한다. 사용자가 승인하면 기능 브랜치에서 이 변경과 호환성·손상/구버전 테스트를 구현한 뒤 Runner 재개·프로세스 재시작·S4 버전 복원/교체/초기화를 진행한다.

### 검증과 운영

- Editor Build `20261015_115642_0db0f59c`, 전체 DialogueKit **35개 통과** `20261015_115703_57156fa8`, 기존 초기화 Error 12개/신규 0개.
- 최종 패키징 `Package_20261015_115728_2f64d1a7` 성공, 새 exe/pak 확인, 패키지 실행은 미실행. cooked Registry 검사 `20261015_115934_3c396ea4` 별도 성공.
- 실제 새 객체 파일 복원과 실제 임시 에셋 비동기 로드는 확인했다. 프로세스 강제 종료/재실행·Runner 저장 재개·GUI/PIE·음향·Sentry 전송은 미검증이다.
- metadata 시험 첫 실행의 Editor native tag 등록 신규 오류는 Runtime 등록 태그 조회로 수정했다. 효과 스냅샷 시험의 배열 self-alias assert는 값 복사 후 Add로 수정했다. 실패 증거와 기존 오류 허용 기준을 보존했다.
- Sol Medium 검토와 Luna High 조사를 실제 위임했고 메인만 수정/빌드/Git을 수행했다. GameDirector 역할은 제공되지 않아 제품 검토는 Unavailable/Not reviewed다.
- 각 일반 기능의 branch와 Subject/Description 커밋을 보존하고 비강제 push/--no-ff 통합했다. 복구는 의존 후속 기능부터 merge revert(-m 1), 이력 삭제나 사용자 변경 폐기 없이 수행한다.


## 2026-10-15 초기화 오류 수정

- 사용자 승인: 앞서 설명한 기본 초기값 적용. 화자 두 필드=TestNPC, 표정=None, 위치=Left, 아이템 종류=Consumable, Value=0. enum 순서/값과 기존 에셋은 변경하지 않는다.
- feature 단위 `fix/dialogue-struct-defaults`에서 수정. 단순 기본값 지정이므로 독립 위임은 생략했다. 사용법은 UserGuide 23절, 자동화 허용 목록 제거와 Build 안내를 함께 갱신했다.
- Build `20261015_133113_5ed12fbc` 성공. 전체 DialogueKit 테스트 `20261015_133134_232c2391` 35개 통과, LogErrorLines=0, StartupClean=true. 초기화 6종/중복12줄은 해소됐다. 기존 라이선스·EnhancedInput 빌드 경고는 별개다.
- 이번 수정에서는 GUI/PIE·패키징·패키지 실행을 재검증하지 않았다. 이전 보고서의 초기화 오류 잔존 문구는 당시 기록이며 현재 결과는 이 절을 따른다.
- 체크포인트 스키마 변경은 설명 요청만 있었으며 승인되지 않았다. 관련 코드/버전은 변경하지 않았다. S0~S2 완료, S3·S4 부분 완료/복원 승인 대기, S5~S7 미착수는 유지한다.


## 2026-10-15 교체 시 롤백 추가 요청

- 사용자가 스키마 1/2 지원과 Block 교체 기능, 특정 Line 통과에 따른 태그·아이템·State 롤백 옵션을 요청했다. 이전 스키마 변경 확인 대기 이후의 새 요청이며, 같은 요청에 대한 재승인을 요구하지 않는다.
- Blueprint의 「교체 시 선택적 상태 롤백」에 요청/기술 제약/권고/미정 사항을 분리했다. 사용자 후속 답변으로 롤백 범위는 원본 Block의 지정 효과만으로 확정됐으며 다른 경로의 변경은 보존한다. 전체 게임 상태 복원은 제외한다. 스키마 2의 실행 증거 자료와 이전 규칙은 이 정책에 맞춰 구체화한다. 후속 답변으로 증거 없는 구 저장은 오류 반환·게임 상태/마지막 성공 저장 보존·대체 Block 미시작으로 확정했다. 후속 답변으로 Line에서 다음 흐름 진행 시 통과를 기록하고 실제 효과 성공을 별도로 확인하며, 효과로 저장할 때 해당 증거를 함께 보존하는 방식도 확정했다.
- 조사 근거: 현재 효과 원장은 before-image/획득 출처/BlockRun별 순서를 보장하지 않으며, 스키마 1 누락 선택 이력은 재구성 불가다. 누적 열람을 현재 실행 증거로 쓰지 않고 게임 제공자의 명시적 원자적 보정·재전달 방어를 권고했다.
- 이번에는 문서만 변경했으며 Runtime 스키마·기존 저장·Block 에셋은 변경하지 않았다. 빌드/테스트는 재실행하지 않는다. 구현 단계 완료 상태는 직전과 동일하다.

- 후속 확정 반영: 원본 Block의 지정 효과만 롤백한다는 사용자 선택을 문서에 반영했다. 앞선 Sol Medium 기술 검토를 재사용하는 작은 문서 수정이므로 추가 위임·빌드·테스트는 생략했다. Runtime/에셋/저장 형식 변경은 없다.

- 추가 확정: 구 저장의 롤백 근거가 부족하면 추측/무보정 교체를 하지 않고 오류 반환 후 상태와 마지막 성공 저장을 보존한다. 모든 스키마 1 저장의 일괄 거부는 아니다. 문서만 갱신하고 구현 완료 상태는 변경하지 않았다.


## 2026-10-15 Line 진행·효과 증거 저장 확정

- 사용자 선택: 기존 Block–Line ID 구조 재사용, 중복 없는 누적 열람, 현재 BlockRun의 통과 ID와 실제 효과 성공 기록 분리. 아이템·태그·상태 변화의 기존 저장 경계에서 해당 Line의 지급 여부를 함께 저장한다.
- 구현 기준: Line 통과 집합과 효과 발생별 ExecutionId를 구별한다. 관련 Line은 실제 발생 Choice/StateChange 노드를 대체하지 않으며 Line 없이 발생한 효과에 임의 Line ID를 붙이지 않는다. 게임 상태·성공 증거·통과 ID·후속 재개 위치를 한 시점에 복사하여 같은 합본 FIFO에 넣는다. 저장 요청 이후 변경은 그 스냅샷에 섞지 않는다.
- 후속 검증: 과거 회차 열람과 이번 실행 분리; 같은 Line 반복과 효과 반복 구분; Line 통과 후 효과 실패; 효과 성공 후 저장 실패; 지연 저장 중 후속 진행; 복원 시 미저장 증거 배제; 관련 Line 없는 효과; 구 저장 근거 부족 거부; 지정 원본 효과만 보정하고 다른 출처 상태 유지.
- 이전 스키마 확인 대기 기록은 당시 이력이다. 이후 스키마 1/2 지원·교체 요청과 롤백/증거 정책의 사용자 확정이 현재 기준이며 같은 범위의 재승인을 반복 요구하지 않는다. 자료형·이전·실행기 연결 및 교체 보정 구현과 검증은 아직 남아 있다.
- 이번 변경은 결정 문서만이며 Runtime·저장 파일은 수정하지 않았다. 독립 기술 검토 결과를 재사용하고 빌드·테스트는 실행하지 않았다. GameDirector 제품 검토 미실행 상태는 유지한다.


## 2026-10-15 S3·S4 잔여 구현 재개

- 사용자 재개 명령에 따라 확정된 스키마1/2·선택적 롤백·통과/성공 증거 정책을 구현한다. 현재 기준 Codex_Develop 8ba8a4a. 과거 확인 대기는 후속 사용자 확정으로 해소됐다.
- 기능 순서: 체크포인트 v2/열람/실행 증거 → Runner·게임 저장 세션 재개 연결 → 정확 콘텐츠 버전 조회/보존 → 명시적 교체·게임 보정 → 스토리 초기화·통합 및 cook/실파일 재시작 검증.
- 기존 WIP 3ac33bd는 보존하고 검증한 부분을 최신 기반 기능 브랜치에 반영한다. 미통합 문서와 알려진 결함을 무검증 merge하지 않는다.
- 읽기 전용 Sol Medium 검토: legacy 증거 Unknown, 실행별 ID, Line 통과 선기록, 효과 후 캡처 실패 중단/보존, End 임시 선택 정리. Luna High는 정확버전/등록/복원 연결점을 조사한다. 메인만 구현·Git·빌드한다. GameDirector 역할 미제공 상태는 유지한다.

### 실행 증거 구현과 검증 차단 (2026-10-15)

- `feature/dialogue-checkpoint-evidence`: 스키마 1→2의 명시적 이전, UDialogueCheckpointState 누적 읽음/현재 통과/성공 효과 분리, End 저장의 occurrence별 정리, 오래된 End 복원 시 임시 선택 재등장 방지 구현. 효과 배치의 BlockRunId/ChoiceId와 같은 시점 게임 원장을 교차 검사한다. 구 저장의 누락된 증거는 bComplete=false로 보존한다.
- Runner에는 실행 ID 발급/조회 및 경계·효과 전달만 반영했다. 자동 CheckpointState 연결, 후속 노드 실행 전 Line 통과 연결, 준비/재개 API, 게임 저장 캡처 실패 보존·복구는 아직 남았다. 현재 증거 클래스의 존재를 전체 저장 연동 완료로 해석하지 않는다.
- 최종 Editor Build 성공: `Saved/AI/20261015_141152_bb60fe77/Build.log`. 추가 회귀 테스트 컴파일 성공. 테스트 시도 `20261015_140855_c2fccc38`, `20261015_141010_895bb9a8`는 모두 엔진 시작의 `PlatformPreInit.ResolutionTooLow`로 종료하여 보고서가 없다. 후속 Windows GetSystemMetrics 확인값은 508×948이다. 실행 테스트 통과 아님.
- 화면 해상도가 복구되어야 동일 공식 스크립트로 전체 DialogueKit 테스트를 실행할 수 있다. 검증 전 Codex_Develop에 merge하지 않으며 기능 브랜치에 보존한다. 의존 후속 기능은 검증·통합 후 별도 브랜치로 진행한다.
- S4 조사 완료: 후보용 Active 필터와 별도의 정확 ID/버전 복원 조회가 필요하다. 범위 매핑·게임 보정·신규 버전 저작/보존·스토리 초기화 연결 및 cook 보존 검증은 미구현이다. 기존 S4 후보/비동기 로드 완료 범위를 확장했다고 보고하지 않는다.
- 사용법은 UserGuide 24절. 실제 GUI/PIE·패키징·패키지 실행은 이번 변경에서 검증하지 않았다. Prototype2와 사용자 에셋은 수정하지 않았다.
