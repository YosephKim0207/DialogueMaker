# S5 제작 도구 구현 결과 — 2026-10-15

## 목차

- [범위와 상태](#범위와-상태)
- [구현 항목](#구현-항목)
- [검증](#검증)
- [남은 단계와 수동 확인](#남은-단계와-수동-확인)
- [Git과 복구](#git과-복구)

## 범위와 상태

사용자 요청에 따라 미완성이던 S5 구현을 마무리했다. S0~S4는 이전 완료 상태를 유지한다. **S5 코드·자동 검증을 완료했지만 실제 GUI 검증은 대기**다. S6/S7 및 Prototype2 코드는 작업하지 않았다. 기존 콘텐츠 에셋 삭제/변경은 없고 삭제 검증은 이번 실행에 만든 Saved/AI 격리 파일만 사용했다.

## 구현 항목

| 항목 | 내용 | 사용자 문서 |
|---|---|---|
| Line 저작 | 연속 화자 원본 탐색·검색 선택, 지정 필드 상속, 시간 배타 편집/일괄 적용, 미열람 갱신 | UserGuide 30 |
| Graph 배치 | 선 더블클릭 경유점, 주석 제목·설명·색상·그룹 이동, editor-only 배치 저장/복원 | 31 |
| 복제 | Ctrl+D, 선택 내부 연결만 유지, 새 Node/Pin/Choice ID, Start 제외, Undo/Redo | 32 |
| 검색 | Ctrl+F/DialogueMaker 메뉴, 화자+대사 AND, Choice 단독, 결과 이동·선택 항목 강조 | 33 |
| 진단/저장 | 스크롤 진단 목록·노드 이동, 경고 확인 저장, 빈 Choice/구조 오류 차단, Save All/AutoSave 가드 | 34 |
| 스토리보드 | Chapter/Scene 등록·편집, Block 목록/Details, 이동/상위 제거 보존, 미출시 선택 삭제 | 35 |
| Block 설정 | 대상 태그 루트 필터/게임 표시명 검색, 소속 드롭다운, 게임 진행과 저작 정의 분리 | 35 |
| Legacy 안내 | Branch 구형 조건 읽기 전용, Block 수동 재작성 안내 | 36 |

주요 변경 파일은 DialogueKit의 `DialogueLineNodeInfo`, `DialogueNodeInfo`, `DialogueEditorLayout`, `DialogueGraph`, `DialogueSettings`, `DialogueBranchNodeInfoBase`와 DialogueKitEditor의 `DialogueLineAuthoring`, `DialogueGraphCodec`, `DialogueRerouteNode`, `DialogueCommentNode`, `DialogueGraphEditing`, `DialogueGraphSearch*`, `DialogueGraphValidation*`, `DialogueGraphSaveGuard`, `DialogueStoryEditor`, `DialogueStoryboard`, `DialogueGraphDetails`, Editor 테스트다. 공개 API와 실제 UI 절차는 안내서에 기록했다.

## 검증

- 최종 Editor 빌드: `Saved/AI/20261015_170058_5e3b6f2c/Build.log` 성공.
- 전체 자동화: `Saved/AI/20261015_170122_48ef2d09/result.json`, **51개 통과 / 오류 로그 0**.
- Win64 Game 빌드·cook·패키징: `Saved/AI/Package_20261015_170149_2c4e23b3/result.json`, `PackagedNotLaunched` 성공. 새 exe/pak 생성 확인, 실제 실행 미검증.
- Cooked Registry 검사: `20261015_170346_d6ca8059`, 1개 통과 / 오류 0.
- Editor 관련 최종 집중 회귀: `20261015_165937_4a642007`, 9개 통과 / 오류 0.
- 격리 에셋 저장·언로드·재로드, 배치 ID/위치 보존, 실제 선택 파일 삭제·비선택 파일 보존을 포함한다.
- 시행 중 오류: API 헤더/필드명 빌드 오류는 수정 후 통과. 파일 시험의 조기 mount 해제는 프로세스 수명 유지로 해결. 좁은 Storyboard 이름 필터가 비점 접두사 테스트도 실행해 래퍼가 UnexpectedTests로 거부한 건 Editor 범위 재실행으로 확인했다. 실패 기록은 남겼다.
- Sol Medium 검토자 2명과 Luna High 조사자가 구현·저장 경계·삭제/수명·누락·Game/cook 분리를 독립 검토했다. 메인만 수정·Git·실행했다. 최종 구조 차단 오류 없음. GameDirector 역할 미노출로 제품 검토는 미실행이다.

## 남은 단계와 수동 확인

| 단계 | 상태 | 내용 |
|---|---|---|
| S0 | 이전 완료 | 기준선과 검증 기반 |
| S1 | 이전 완료 | 공통 ID·데이터·게임 계약 |
| S2 | 이전 완료 | 최소 Graph 편집·왕복·실행 |
| S3 | 이전 완료 | 효과·체크포인트·누적 이력 |
| S4 | 이전 완료 | 후보·비동기 로드·버전 복원 |
| S5 | 구현/자동 검증 완료, GUI 대기 | 실제 화면·입력 검증 목록 아래 참조 |
| S6 | 미착수 | 실제 텍스트·이미지·음향·저장 표시·Block PIE·편집 잠금 |
| S7 | 미착수 | 전체 연동·배포 실행·Prototype2 검증 |

실제 GUI 도구가 연결되지 않아 다음은 완료로 주장하지 않는다.

- 핀 드래그·선 더블클릭, 경유점 선택/이동, 주석 그룹 이동과 설명 편집.
- 복수 원본 선택/취소, Ctrl+D/Ctrl+F 포커스, 검색 클릭·Choice Details 강조.
- 저장 경고창 클릭 이동·취소·차단 버튼, 실제 Save All/AutoSave 동작.
- 스토리보드 배치·스크롤·선택·정렬·두 단계 취소와 참조 때문에 실패한 삭제 표시.
- 게임 패키지의 실제 화면/입력/음향 및 PIE. NullRHI 자동화는 이를 대신하지 않는다.

Save All/AutoSave는 재진입 모달 없이 차단 오류만 거부한다. 경고별 확인 UI는 Graph의 저장/다른 이름 저장 경로에서 제공한다. 단일 엔진 저장 delegate를 다른 플러그인이 덮어쓰는 통합 상황은 게임에서 확인해야 한다. StoryDefinition은 게임 측 참조/cook 포함 규칙이 필요하다. 이번 샘플 패키징은 모든 게임의 콘텐츠 포함을 보장하지 않는다.

## Git과 복구

일반 기능별 Branch, 구현/검증·문서의 작은 커밋, `Codex_Develop`의 `--no-ff` 통합을 사용했다. 모든 새 Subject/Description은 한국어다. 기능 브랜치를 보존하며 이력 재작성/기존 에셋 삭제는 하지 않았다.

| 기능 | Branch | 통합 커밋 |
|---|---|---|
| Line | feature/dialogue-line-authoring | 355f66a |
| 경유점/주석 | feature/dialogue-reroute | f7972ef |
| 복제 | feature/dialogue-duplicate | e16377c |
| 검색 | feature/dialogue-search | cc659eb |
| 검사/저장 | feature/dialogue-validation-panel | 1c15407 |
| 스토리보드 | feature/dialogue-storyboard | 801f67b |

복구가 필요하면 의존 후속 기능을 먼저 검토하고 해당 기능 merge에 대한 `git revert -m 1 <merge>`로 되돌린다. 부분 수정은 목적별 개별 커밋을 검토한다. 실제 복구 실행은 데이터 소실/이력 정책에 따른다.
