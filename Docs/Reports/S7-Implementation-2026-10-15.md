# S7 통합·배포 구현 보고

- [1. 완료 범위](#1-완료-범위)
- [2. 구현과 호환](#2-구현과-호환)
- [3. 검증 증거](#3-검증-증거)
- [4. 브랜치와 복구](#4-브랜치와-복구)
- [5. 남은 인수 검증](#5-남은-인수-검증)

## 1. 완료 범위

사용자의 S7 구현 요청으로 `Codex_Develop a767e4e`에서 시작했다. 지역화 호환 보완, 게임 모듈의 공개 API 연결 샘플, 실제 Development 패키지 저장/강제 종료/정확 버전 복원, Shipping 빌드/cook 및 개발 자료 제외 검사를 구현했다. 기술 구현·아래 자동 검증은 완료했으며 전체 제품 인수 상태를 Verified로 올리지는 않는다.

| 단계 | 내용 | 현재 상태 |
|---|---|---|
| S0 | 기준선·검증 실행 기반 | 이전 완료 |
| S1 | 공통 데이터·안정 ID·게임 인터페이스 | 이전 완료 |
| S2 | 독립 노드 Graph·편집 왕복·실행 | 이전 완료 |
| S3 | 효과·체크포인트·FIFO·누적 이력 | 이전 완료, S7 회귀 통과 |
| S4 | 후보·비동기 로드·버전 복원/교체 | 이전 완료, S7 통합 방어 보완 |
| S5 | 저작 편의·검색·검사·스토리보드 | 구현/자동 검증 완료, 실제 GUI 인수 대기 |
| S6 | 연출·저장 표시·Block PIE | 구현/자동·엔진 PIE 생명주기 검증 완료, 화면/입력/청취 대기 |
| S7 | 지역화·호스트·배포 통합 | 이번 기술 구현/자동 검증 완료, 아래 수동/제품 검토 대기 |
| 별도 게임 | Prototype2 실제 저장·진행도·Sentry 통합 | 이번 범위 밖, 미착수 |

## 2. 구현과 호환

- 지역화: Runtime LocalizationDataAsset/Subsystem/Utility와 Editor CSV converter. ChoiceId 열/필드를 추가하고 신규 ChoiceId 우선, 기존 PinId fallback을 유지한다. 기존 4열 **입력** CSV는 유지되지만 새 출력은 5열이다. AssetManager 등록 경로를 사용해 배치 생성 하위 폴더도 찾는다. 명시적 문화권을 보존하면서 언어 전용 코드만 정규화한다.
- 게임 연결: Source/DialogueMaker의 DialogueSampleHost, Subsystem StartDialogueBlockForTarget, RestoreController 후보 취소. 요청 대상·ID를 보존하고 노출/정상 완료와 캡처 재시도를 연결한다. 화면은 명시적으로 생성하며 저장 표시 수명은 대화와 분리한다. 게임 공급 제공자를 사용하고 임의의 샘플 제공자 자동 대체는 하지 않는다.
- 기존 소비처: 초상화 0개에서 PreloadPrimaryAssets가 ensure를 내는 실제 패키지 오류를 수정했다. UI 직접 진행, 비어 있는 handle 해제 방어, null 개발 Graph 및 Shipping ATEST 호출 방어를 추가했다.
- 배포 검증: DialoguePackageValidation과 Invoke-PackagedDialogueValidation.ps1은 명시적 Development 검증에서만 동작한다. Shipping 옵션·fixture 지정 거부·ProjectShipping cooked Registry 검사를 추가했다.
- 기존 에셋 바이너리와 게임 저장 스키마 1/2 독자는 변경하지 않았다. 신규 테스트 fixture는 GUID 경로로만 생성했다. Prototype2 파일/실제 게임 저장은 수정하지 않았다.
- 샘플 합본은 예제 상태·효과 원장·대화 snapshot용이다. Chapter/Scene 등 실제 게임 전체 상태 합본은 게임 개발자의 저장 제공자가 확장할 책임이며 S7 샘플이 이를 완성했다고 주장하지 않는다.
- 사용법과 공개 함수/실제 명령은 UserGuide 42~45절. 핵심 테스트는 LocalizationRoundtripTests, DialogueSampleHostTests, DialogueShippingTests다.

## 3. 검증 증거

모든 경로는 프로젝트 `Saved/AI/` 기준이다. 결과 파일은 로컬 산출물이며 Git에는 코드/문서/재현 절차를 저장한다.

| 검증 | 결과 | 증거 |
|---|---|---|
| 최종 Editor Build | 성공 | `20261015_193503_68412834/Build.log` |
| 전체 DialogueKit | 59/59, Error 0 | `20261015_193637_bd970acc/result.json` |
| 실제 엔진 PIE 시작/취소/실행/종료 | 1/1, Error 0 | `20261015_193749_bd07cb82/result.json` |
| 다른 UE 프로세스 저장/복원 | Write 1, Read 1 성공, Error 0 | `20261015_192753_4649e855`, `20261015_192803_9788b41a` |
| 보존 v1/활성 v2 cook Registry | 2/2, Error 0 | `20261015_192840_43ca632d/result.json` |
| 최종 Development package | 성공 | `Package_20261015_193150_3129acd8/result.json` |
| 실제 packaged Write→Crash→Read | 3단계 성공, 각 로그 Error 0 | `PackageRun_20261015_193237_48841920/result.json` |
| Shipping build/cook/package | 성공, 실제 실행은 미실행 | `Package_20261015_193325_31d2eb35/result.json` |
| Shipping cooked fixture/태그 제외 | 1/1, Error 0 | `20261015_193547_8b3053f0/result.json` |
| Development/Shipping 바이너리 비교 | 개발 표식 3종은 Development에 존재, Shipping에 없음 | `Package_20261015_193325_31d2eb35/ShippingMarkerCheck.json` |

패키지 fixture ID는 `1a7f3a0965724c55bfb900a8d20d6c1e`. 실제 Write는 PID 11892, Crash는 PID 18980, Read는 PID 3284였다. 기존 프로젝트 슬롯과 Crash 전후 durable 파일의 SHA256 불변을 확인했다. Read는 보상 10 유지, 같은 Block ID·v1·실행 ID·재개 노드, 후속 활성 v2 후보 실행을 확인한다. Crash에서 적용한 미저장 보상은 복원되지 않는다.

강제 종료는 **디스크 쓰기 직전 대기 큐**의 실제 프로세스를 종료한다. OS 쓰기 중 부분 기록/전원 차단과 같은 시험이 아니다. 손상 슬롯 fallback은 기존 저장 자동화 검사로 별도 검증된다. NullRHI/nosound 결과는 실제 화면/물리 입력/음향 관찰이 아니다.

실패 이력도 보존했다. 지역화 테스트에서 저장 직후 mount 해제가 엔진 지연 검증 오류를 유발해 고유 mount를 프로세스 수명까지 유지했다. 호스트 테스트는 미완료 anchor 보존을 반영하고 캡처 probe의 준비 계약을 수정한 뒤 통과했다. 첫 실제 패키지는 빈 초상화 preload ensure 때문에 실패했으며 원인을 수정하고 재패키징했다. 수정 중 TEST.cpp 전처리 짝 누락으로 한 차례 빌드/패키징이 실패했고 수정 후 Editor/Development/Shipping 모두 통과했다.

기존 EnhancedInput 플러그인 의존성 선언 경고, XGE 라이선스 경고, Windows Git safe.directory 경고는 별도로 남아 있다. 전역 설정과 엔진은 변경하지 않았다.

## 4. 브랜치와 복구

| 기능 브랜치 | 주요 작은 커밋 | 통합 기준/병합 |
|---|---|---|
| feature/dialogue-localization-compat | c5e9e63 계획, a30d7f0 구현, 4a20b4c 회귀/문서 | a767e4e → 7d6d056 |
| feature/dialogue-sample-host | a38a0f0 경계, 2f7cee6 호스트, ab1dac7 회귀/문서 | 7d6d056 → 4fec9fc |
| chore/dialogue-release-regression | e8610ae 기존 소비처, 4ad03cc 실행 검증, 763ec89 래퍼/Shipping | 4fec9fc 기준, 본 보고서 커밋 후 --no-ff 통합 |

모든 새 커밋의 Subject/Description은 한국어다. 기능 브랜치를 보존하고 통합 중심은 Codex_Develop다. 부분 복구는 의존 커밋을 확인하여 revert하고, 기능 전체는 해당 merge를 `git revert -m 1 <merge>`로 되돌린다. reset/force push로 기존 이력을 제거하지 않는다. 후속 브랜치가 선행 API를 사용하므로 되돌릴 때 사용자 문서와 테스트도 같은 범위로 처리한다.

읽기 전용 검토는 Sol Medium 두 명(지역화/배포, 저장/실행 연결), Luna High 한 명(검증 수단·엔진 인자)을 사용했다. 메인만 수정·Git·빌드·테스트를 수행했다. 후보/UI 종료 경합 지적은 Runner의 bProcessing 방어를 재확인해 철회했고, 실제로 확인된 요청 대상·복원 취소·초기 캡처 재시도 문제를 반영했다.

## 5. 남은 인수 검증

1. S5 실제 에디터 GUI: 검색 결과 이동, 경고창·삭제 확인, 드래그/경유점/스토리보드 조작을 사용자가 확인해야 한다.
2. S6/S7 실제 화면·물리 키/마우스·음향: 출력 시간, 음성 중지, 이미지 위치/방향, 저장 표시, PIE 종료와 편집 잠금 해제를 관찰해야 한다. 자동화 가능한 생명주기와 입력 상태 로직은 통과했다.
3. Shipping 실행·실제 설치 환경, 실제 지역화 화면·미디어 의존 리소스 전체 콘텐츠 검사. 이번 Shipping 검증은 빌드/cook/package와 제외 검사까지다.
4. 실제 출시 저장 fixture와 Chapter/Scene·퀘스트 등 게임 전체 상태의 보정/합본, Prototype2 연동 및 게임별 Sentry 수신은 별도 프로젝트 단계다. 샘플 성공을 게임 통합 완료로 취급하지 않는다.
5. GameDirector 역할이 현재 도구에 없어 제품 최종 검토는 미실행이다. 다른 역할로 대체하지 않았다.

이 잔여 항목 때문에 S5~S7 전체 Exit/제품 인수를 Verified로 표시하지 않는다. 새로운 기능 구현 단계는 남겨 두지 않았으며, 후속은 수동 인수 검증과 실제 게임 통합이다.
