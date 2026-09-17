# Task: GameDirector와 프로젝트 청사진 연결

## Goal / Scope
DialogueMaker에 설계 자문 에이전트와 Blueprint.md를 추가하고 총괄의 구현 전 읽기·상태 갱신 절차를 연결한다.
게임 코드·에셋·저장 포맷·엔진·기존 모델 설정은 변경하지 않는다. 사용자가 이번 설정·문서 작업을 요청했다.

## Plan
1. 기존 지침/CLI/공식 설정 문서 확인 및 Sol Medium 독립 검토.
2. game_director.toml과 Blueprint.md 작성, AGENTS.md와 Docs/AI/Agents.md 연결.
3. TOML/참조/상태 일관성 검증, CLI strict config 확인 및 가능한 경우 새 역할 호출 확인.
4. Windows Git diff로 변경 범위 확인 후 결과 보고.

## Compatibility / Risks
- 기존 에이전트 파일과 config.toml 유지. GameDirector는 사용자 요청에 따라 GPT-6 Astra / medium / read-only 사용. 기존 검토자 모델은 유지.
- 메인만 문서 저장. 신규 기능 구현승인과 설계결정 분리.
- 기존 정적 코드 확인을 완료로 과장하지 않음.
- 현재 브랜치는 DiagnoseGraphSystem. 이전 분석은 SheetSystem 기준임을 Blueprint에 명시.

## Rollback
이번 추가 3개 파일과 기존 문서의 GameDirector 절만 대상으로 되돌릴 수 있다. 사용자 변경을 일괄 복원하지 않는다.

## Progress / Evidence
- [x] 기존 지침/CLI 0.154.0/공식 문서 확인.
- [x] Sol Medium 독립 계약 검토 반영.
- [x] 설정·문서 작성.
- [x] TOML/문서 정합성 및 CLI strict config 검증: 4개 역할 TOML 파싱, BP ID 16개 중복 없음, 참조 연결 확인. doctor 19 ok / 1 idle / 0 warn / 0 fail.
- [ ] 실제 game_director 호출 검증: 현재 새 세션 생성 도구에 커스텀 역할 지정 인자가 없어 실패. 역할 대체·게임 작업은 하지 않음.
- [x] Windows Git diff 검토: 지침 2개 수정과 새 파일 3개만 존재, diff --check 통과. 기존 safe.directory 및 줄바꿈 경고는 설정을 변경하지 않음.
- 게임 빌드/테스트: 설정·문서 작업으로 미실행.

## 실행 검증 제한

- 검증 세션: 01a0af79-beb4-7f40-b078-5f9763a51c41. 하위 생성 0회: 커스텀 역할 선택 인자 미노출. 설정 파일 생성/문법 검증과 실제 역할 실행을 구분한다.
- CLI의 상대 경로 최종 출력 저장도 실패하여 Saved/AI/GameDirectorSmoke.txt 로그는 생성되지 않았다. 위 세션 응답과 이 기록을 근거로 남긴다.
- 현재 호스트가 커스텀 역할 호출을 제공할 때 재검증한다. 실행 성공을 주장하거나 다른 모델/역할로 대체하지 않는다.
