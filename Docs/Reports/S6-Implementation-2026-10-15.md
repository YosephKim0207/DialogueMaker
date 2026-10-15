# S6 연출·Block PIE 구현 보고

- [1. 완료 범위](#1-완료-범위)
- [2. 구현과 사용](#2-구현과-사용)
- [3. 검증 결과](#3-검증-결과)
- [4. 브랜치·커밋·복구](#4-브랜치커밋복구)
- [5. 남은 확인과 다음 단계](#5-남은-확인과-다음-단계)

## 1. 완료 범위

사용자의 `s6 작업을 진행해` 요청으로 5ae2cc5에서 시작했다. S6 코드·자동 검증·실제 엔진 PIE 생명주기 검증을 완료했다. 시각/물리 입력/음향 청취는 도구 부재로 확인하지 않았으며 단계 Exit 전체를 Verified로 표시하지 않는다.

| 항목 | 구현 내용 | 상태 |
|---|---|---|
| 텍스트 시간 | 0 즉시, 전체/글자당 시간, 결합 문자 경계 | 구현·자동 검증 |
| 표시 입력 | 펼치기와 실제 진행 분리, 음성 중지, 동일 프레임/이전 revision 방어 | 구현·자동 검증 |
| 기본 화면 | 화자/대사/Choice, 직접 이미지 또는 화자+표정, 위치·반전 | 구현, 시각 확인 대기 |
| 음향 | BGM/SFX/Voice, Line/취소/종료 수명과 음성 펼치기 | 구현, 청취 확인 대기 |
| 저장 표시 | 대화와 독립된 우측 하단 이미지/기본 애니메이션, FIFO·실패·재시도 | 구현·자동 검증, 시각 확인 대기 |
| Block 테스트 | 일반/진입 무시, 프리셋, 미저장 복제, 조건 설명, 실제 PIE | 구현·생명주기 자동 검증 |
| 격리·잠금 | 기존 저장 Subsystem 메모리 격리, 효과 무적용, Graph/Details/Undo/핀/저장 잠금 | 구현·자동 검증, GUI 조작 확인 대기 |

## 2. 구현과 사용

- Runtime 주요 변경: `DialogueTextPresentation`, `DialoguePresentationWidget`, `DialogueSavingWidget`, `DialoguePreviewPolicy` 신규. `DialogueBlockRunner` 표시 갱신/취소, `DialogueLineNodeInfo` 음향 참조, `DialoguePortraitData` 로드 API, 기존 Dialogue/Progress/Quest Subsystem의 테스트 저장 격리.
- Editor 주요 변경: `DialogueBlockPreview` 프리셋/복제/진입 평가/PIE 조율 신규. GraphEditor·TabFactory 메뉴/잠금, Schema 연결 변경 방어, SaveGuard 테스트 저장 거부, 모듈 종료 정리.
- 새 UI는 opt-in이다. 기존 WidgetClass/바이너리 에셋을 자동 교체하지 않는다. 게임은 CreateWidget→AddToViewport→BindSubsystem/BindQueue와 입력 포커스를 연결한다. 커스텀 화면은 공개 표시 객체를 사용한다.
- 작가는 Graph 편집기 `DialogueMaker → 테스트`에서 프리셋·일반/진입 무시 테스트·종료를 사용한다. 프리셋은 명시적 저장만 가능하며 효과 적용 이후 상태는 별도 프리셋으로 시험한다.
- 외부 게임 Subsystem은 `IsPreviewWorld`를 이용해 자기 영속 I/O를 격리해야 한다. 기본 GI만으로 외부 Subsystem이 생성되지 않는다고 가정하지 않는다.
- 실제 사용 절차와 확장 지점은 [UserGuide 37~41절](../UserGuide.md#37-텍스트-표시-상태와-안전한-입력)에 기록했다.

## 3. 검증 결과

모든 경로는 `Saved/AI` 하위다. 래퍼의 새 결과 JSON/보고서를 확인했다.

| 검사 | 결과 | 증거 |
|---|---|---|
| 최종 Editor Build | 성공 | 20261015_175453_5b2b866b |
| 전체 DialogueKit | 56/56, Error 0 | 20261015_175517_60d1fd9a |
| 실제 PIE 생명주기 | 1/1, Error 0 | 20261015_175557_20212f9b |
| Win64 Game 빌드/cook/package | 성공, 새 exe/pak 생성, 실행 미검증 | Package_20261015_175650_993bc95a |
| Cooked Registry | 1/1, Error 0 | 20261015_175741_f99e5e09 |

PIE 테스트는 엔진의 실제 RequestPlaySession 경로를 사용한다. 시작 대기 취소·재시작, 기본 GI, 설정 원복, Line→End, 종료 후 편집 복구를 확인했다. NullRHI/nosound이므로 화면과 소리를 관찰한 결과는 아니다.

격리 테스트는 기존 SaveGames 파일 집합/바이트를 초기화와 종료 이후 저장 호출 전후로 비교했다. 미저장 복제 테스트는 원본 Graph 포인터·패키지 dirty 상태·안정 ID·프리셋 초기 상태를 확인했다. 기존 에셋/슬롯을 시험용으로 덮어쓰지 않았다.

발견하고 수정한 실패도 보존한다. UMG의 UE5.4 헤더/버튼 focus API와 지역 변수 shadow 컴파일 오류, Preview API명/TSharedRef/TObjectPtr 테스트 컴파일 오류를 수정했다. 173730_fbbefb0a 저장 표시 테스트는 성공 응답에 실패 코드가 남은 테스트 데이터 오류였으며 성공 응답 정규화·반환값 검증 후 173855_0cf1f53b에서 3개 통과했다. 최종 전체 테스트에서도 통과했다.

기존 EnhancedInput 의존성·XGE 라이선스·Git safe.directory 경고는 별도 설정 변경 없이 유지했다.

읽기 전용 검토는 Sol Medium 검토자 2개와 Luna High 검색자를 재사용했다. 텍스트/UMG 수명, PIE API·GI 복원·취소/Undo 경계, 테스트 fixture와 요구사항 누락을 조사했다. 메인만 수정·Git·빌드·테스트했다. GameDirector 역할 미노출로 제품 검토는 미실행이다.

## 4. 브랜치·커밋·복구

| 기능 브랜치 | 주요 커밋 | 통합/기준 |
|---|---|---|
| feature/dialogue-text-timing | e5d1574, 0e89221 | ed8e8ee |
| feature/dialogue-presentation-view | 1e4c34a, 8f6df8c | 966d56d |
| feature/dialogue-saving-indicator | 0e3bc1b, e4a27af | 8030994 |
| feature/dialogue-preview-isolation | 6c40544, 5d534a9 | b71ffd4 |
| feature/dialogue-block-preview | d9be313, 67f557a 및 검증/문서 커밋 | b71ffd4에서 분기, 검증 후 Codex_Develop 통합 |

모든 Subject/Description은 한국어로 변경 이유·검증·미검증을 기록했다. 기능 브랜치를 보존하고 비강제 push한다. 복구는 후속 의존성을 확인한 뒤 기능 merge를 `git revert -m 1 <merge>`로 되돌린다. 부분 revert 시 해당 기능의 테스트와 사용 문서를 함께 되돌린다. reset/강제 push/브랜치 삭제는 하지 않았다.

## 5. 남은 확인과 다음 단계

| 단계 | 완료된 내용 | 남은 내용 |
|---|---|---|
| S0~S4 | 기존 구현·단계 검증 완료 유지 | S7의 전체 통합 검증 |
| S5 | 제작 도구 구현·자동 검증 | 실제 에디터 GUI 관찰 |
| S6 | 연출/표시/PIE 도구 구현·자동 검증·PIE 생명주기·패키징 | 실제 키/마우스·글자 출력·초상화·음향·저장 아이콘·잠금 GUI 확인, GameDirector 검토 |
| S7 | 미착수 | 전체 통합·지역화·실제 배포 실행·저장 회귀·게임 어댑터 검증 |

Prototype2는 수정하지 않았다. S7 또는 실제 Prototype2/Sentry 통합 성공을 이번 결과로 주장하지 않는다. 이미지/음향은 표시 시 동기 로드하므로 큰 리소스는 게임에서 미리 로드하는 확장 지점을 사용한다.
