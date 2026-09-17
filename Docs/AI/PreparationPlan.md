# Task: UE 5.4.4 개발 자동화 준비

## Goal
두 독립 저장소 Prototype2와 DialogueMaker에서 기능 요청을 조사, 구현, 빌드, 검증하고 Fork로 검토할 수 있는 작업 절차를 준비한다.

## Background / Self-Contained Context
- 작업 루트: C:\Users\kjs94\Documents\Unreal Projects
- 엔진: C:\Program Files\Epic Games\UE_5.4, Build.version = 5.4.4 / 35576357.
- Prototype2: Runtime Prototype2, Editor 타깃 Prototype2Editor, 기존 Sentry 플러그인.
- DialogueMaker: 호스트 Runtime DialogueMaker, 플러그인 Runtime DialogueKit / Editor DialogueKitEditor.
- DialogueMaker의 ImageProduction 브랜치에는 기존 소스·설정·에셋·지침 변경이 있다. 새 자동화 파일 위주로 작업한다.

## Scope / Constraints
- In: 환경 조사, Docs/AI, Scripts/AI, 프로젝트 작업 지침, 기준 빌드.
- Out: 게임 기능 변경, 플러그인 통합, 에셋 편집, 엔진 변경, 외부 플러그인 설치.
- 사용자의 미커밋 변경을 보존한다. Git push/merge/사용자 변경 커밋은 수행하지 않는다.

## Dependencies / Assumptions / Open Questions
- Windows PowerShell 5.1 실행 연결 확인. WSL 샌드박스 밖 Windows 실행에는 실행 권한이 필요하다.
- MSVC/SDK의 실제 사용 가능 여부는 기준 빌드로 확인한다.
- 프로젝트 Source 및 DialogueKit Source에서 자동화 테스트 등록 매크로를 찾지 못했다.

## Uncertainty & PoC Track
- PoC: 엔진 버전, 타깃, 프로세스를 검사한 뒤 Editor 빌드를 하나씩 실행한다.
- 성공: 빌드 종료 코드 0과 새 로그. 실패: 실제 오류 기록 후 게임 코드를 임의 수정하지 않는다.
- 통과한 절차만 검증 완료로 기록한다. GUI/PIE 접근과 기능 테스트는 별개다.

## Plan
1. Source/Config/타깃/Git/엔진 조사. 읽기 결과를 Docs/AI/Environment.md에 기록.
2. Scripts/AI에 환경 검사, 빌드, 테스트 실행 절차 추가. PowerShell 파싱과 사전 검사 확인.
3. 각 Editor 타깃을 순차 빌드. 로그와 종료 코드 기록.
4. AGENTS.md와 Docs/AI/Build.md에 Git/Fork 및 검증 절차 연결. 새 파일 diff 검사.

## Risks & Mitigations / Approval Points
- 저장 형식 변경, 기존 에셋 삭제·이름 변경, 엔진 변경·새 외부 플러그인, 범위 밖 재설계는 사용자 확인.
- 열려 있는 에디터나 다른 빌드가 있으면 강제 종료하지 않고 빌드를 보류한다.
- 기존 변경이 있으므로 현재 브랜치를 강제로 전환하거나 전체 파일을 stage하지 않는다.

## Rollback Plan
이번 작업에서 추가한 Docs/AI와 Scripts/AI 및 새 지침의 diff만 검토하여 되돌린다. 기존 변경에 reset/clean을 사용하지 않는다. 생성 로그는 Saved/AI에 둔다.

## Backward-compat Matrix
게임 코드, SaveData, DialogueGraph, uasset/umap 형식: 변경 없음.

## Progress Log
- [x] 엔진/프로젝트/지침 조사
- [x] 자동화 파일 준비
- [x] 두 Editor 기준 빌드 성공 및 내장 테스트 각각 1개 성공 기록

## Decision Log
- 2026-09-15: 기존 .codex는 빈 일반 파일이므로 설정 폴더로 바꾸지 않는다. AGENTS.md와 스크립트로 준비한다.
- 2026-09-15: 새 기능 구현은 현재 준비 범위에 포함되지 않는다.
- 2026-09-15: Windows Git으로 실제 변경 범위를 재확인했다. Prototype2는 추적 파일 변경 없음, DialogueMaker는 기존 에셋 2개와 C++ 파일 5개 변경이다. WSL Git의 대량 변경 표시는 정리하지 않는다.
- 2026-09-15: .NET Process로 테스트 프로세스의 실제 종료 코드를 확인한다. 시작 로그 오류와 선택한 테스트 결과를 구분한다.
- 2026-09-15: Prototype2의 에셋 포인터 문제를 Windows Git LFS checkout으로 복구했다 (5897/5897, 8.3 GB 처리). 새 에셋 생성이나 형식 변경이 아닌 기존 로컬 LFS 원본 체크아웃이다.
- 2026-09-15: 테스트 실행이 갱신한 Prototype2 .sentry-native 추적 진단 파일은 실행 전 HEAD 상태로 복구했다. 새 실행 파일은 Saved/AI에 보관하고, 이후 테스트는 Sentry 자동 초기화를 실행 옵션으로 끈다.

## Discoveries / Result / Verification / Evidence
최종 환경 및 명령별 결과는 같은 폴더의 Environment.md와 Build.md에 기록한다. 실행 로그는 각 프로젝트 Saved/AI에 저장한다.

준비 결과: 양쪽 Editor 빌드와 내장 테스트 실행/보고서 판독 성공. 없는 테스트는 실패로 거부했고, 기존 변경이 있는 저장소의 브랜치 생성도 중단했다. Prototype2 LFS 포인터 문제를 로컬 원본으로 복구했고 실행이 건드린 진단 파일도 복원했다. 현재 기존 게임 소스/에셋 변경을 보존한 채 자동화 문서·스크립트와 지침만 추가/수정된 상태다. 남은 기존 구조체 초기화 오류는 각 환경 문서에 기록했다. GUI/PIE/패키징 및 실제 기능 테스트는 이번 검증 대상이 아니다.
