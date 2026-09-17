# Task: 역할별 Agent 및 SubAgent 구성

## Goal / Background
사용자가 지정한 Astra Medium 메인, Sol Medium 검토자, Luna High 검색자, 어려운 버그 전용 Sol High 검토자를 구성한다. 기존 UE 5.4.4 빌드/테스트/Git 절차를 메인이 사용하며 결과를 통합한다.

## Self-Contained Context / Scope
대상은 Unreal Projects 부모 작업 공간과 Prototype2, DialogueMaker의 프로젝트별 .codex 설정 및 AGENTS.md다. 기존 DialogueMaker/.codex는 0바이트 일반 파일이므로 Docs/AI/codex-placeholder.backup에 보존한 뒤 설정 폴더로 바꾼다. 게임 소스/에셋/엔진/저장 형식은 변경하지 않는다.

## Constraints / Dependencies
설치된 codex-cli 0.154.0과 공식 subagents 문서를 기준으로 standalone .codex/agents/*.toml을 사용한다. 메인 제외 동시 하위 3개. 검토자는 파일 수정/빌드/Git 변경/에디터 조작/추가 위임을 하지 않는다. 부모 런타임 권한이 sandbox 기본값보다 우선할 수 있으므로 읽기 전용 설정을 절대 보안 경계로 표현하지 않는다.

## Assumptions / Open Questions / PoC
새 세션에서 프로젝트 설정을 읽는다. 실행 중 세션 모델은 파일 변경으로 소급 전환되지 않는다. TOML 파싱, CLI 설정 로드, 역할별 실제 읽기 전용 위임을 확인한다. 계정 권한과 호스팅 환경이 모델/커스텀 역할을 제공하지 않으면 실패 사실을 알리고 임의 대체하지 않는다.

## Plan
1. Tools/AI/AgentTemplates에 설정 원본, 메인 작업 절차, 설치 스크립트 작성.
2. 세 .codex 위치에 검토된 설정 설치, AGENTS.md와 문서 연결. 기존 파일은 덮어쓰지 않고 검증한다.
3. TOML/CLI 검사 및 실제 Sol Medium, Luna High 하위 작업으로 연결 확인.
4. GPT_Manual.txt, 구조도 포함 HTML/PDF 작성, 셸에 텍스트 전체 출력.

## Approval Points / Risks & Mitigations
사용자가 이미 에이전트 구성을 요청했다. 보호된 .codex 설치에는 도구의 파일 시스템 권한이 필요할 수 있다. 저장 형식/기존 에셋 삭제·이름/엔진·외부 플러그인/범위 밖 재설계 확인 조건은 유지한다.

## Rollback Plan / Backward-compat Matrix
새 설정과 지침 추가분만 되돌린다. 이전 빈 .codex 파일은 백업으로 복구 가능하다. 게임 코드·에셋·저장 데이터의 동작과 형식 변경 없음.

## Progress / Decision Log / Evidence
- [x] 공식 문서와 CLI 0.154.0 확인, 기존 지침 Sol Medium 검토.
- [x] 부모/두 프로젝트 설정 설치, 기존 빈 .codex 백업, TOML/매핑 검사.
- [x] 새 CLI 세션 runtime turn_context에서 Astra medium, 커스텀 Sol medium, Luna high 확인. 두 하위 READY 수신.
- [x] 매뉴얼 TXT 및 구조도 포함 10페이지 PDF 생성. PDF 헤더/EOF/내장 폰트 확인.
- [x] GPT_Manual.txt 전체를 현재 셸 출력으로 확인.
- OpenAI Docs: https://learn.chatgpt.com/docs/agent-configuration/subagents

## Result / Verification

## Follow-up: 역할 범위 일반화
사용자 승인에 따라 특정 기능 예시를 역할 정의에서 제거한다. 부모/두 프로젝트/템플릿의 세 역할 설명과 지침, 운영 문서, TXT 및 PDF 구조도를 함께 갱신한다. 모델·추론·권한·동시 한도는 유지한다. 검증은 TOML 파싱, 복사본 일치, 잔여 기능명 문맥 확인, PDF 재생성으로 수행한다. 이전 READY 검증은 역할 연결 검증으로 구분한다.
- [x] 역할 정의 및 동적 배분 지침 갱신. 기능명은 역할 TOML에서 제거하고 문서의 명시적 예시로만 유지.
- [x] 세 설치 위치의 TOML 문법·템플릿 일치, 두 운영 문서 일치, TXT/HTML/PDF 재생성 확인. 모델·추론·권한 설정은 유지. 이번 문구 수정에 대한 실제 작업별 배분 실험은 수행하지 않음.

설치된 CLI 0.154.0의 strict-config 로드 성공. 제한 밖 doctor 19 ok / 1 idle / 0 warn / 0 fail. 최초 샌드박스 DB/네트워크 오류는 제한 밖 재검사에서 해소돼 DB를 수정하지 않았다. 실제 역할 연결 증거는 Saved/AI/AgentSetupSmoke.txt에 런타임 세션 ID와 함께 기록했다. Sol High는 어려운 버그 전용이므로 구성만 검증했다. 게임 코드 변경이 없어 UE 빌드/테스트를 반복하지 않았다.
