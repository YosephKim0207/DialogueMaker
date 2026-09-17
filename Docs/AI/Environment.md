# DialogueMaker 환경 조사 및 준비 결과

## 프로젝트 구조
- 독립 Git 저장소. 기존 브랜치 ImageProduction, HEAD 6a14912 (Flip 연출 기능 추가를 위한 Enum 값 정비).
- DialogueMaker.uproject EngineAssociation = 5.4.
- 호스트 Source/DialogueMaker는 Runtime 모듈. DialogueMakerTarget(Game) / DialogueMakerEditorTarget(Editor).
- 실제 배포 대상 플러그인: Plugins/DialogueKit/DialogueKit.uplugin, 버전 0.1, 콘텐츠 포함.
- DialogueKit Runtime: 대화 그래프/노드, GameplayTag 조건, 진행/이력 저장, 퀘스트, 지역화, 초상화/연출.
- DialogueKitEditor: 그래프 에디터·노드/핀·팩토리·에셋 정의·CSV 변환. UnrealEd/GraphEditor/AssetTools 등 Editor 의존성은 별도 모듈에 있다.
- 시작 맵: /DialogueKit/Sample/Maps/NewWorld.NewWorld.
- Game 타깃에는 non-unity/IWYU 및 Latest include override가 있다. Editor 타깃은 Unreal5_4 기준이다. 이번에 변경하지 않았다.
- global.json의 SDK 6.0.428과 실제 UE Build.bat의 번들 6.0.302는 구분한다. 기준 빌드는 성공했다.
- 기존 AGENTS.md/PLANS.md/ImageProduction.md를 유지한다. 연출 기능 작업 때만 ImageProduction TODO를 추가로 읽는다.
- 초기 조사 시 .codex는 빈 일반 파일이었다. 이후 사용자 요청에 따라 역할별 에이전트를 설치하면서 Docs/AI/codex-placeholder.backup에 보존하고 .codex 설정 폴더로 전환했다. 현재 팀 구성은 Docs/AI/Agents.md를 따른다.

## 기존 변경 보존 (Windows Git 기준)
- Plugins/DialogueKit/Content/Sample/DialogueGraph/NewDataAsset.uasset
- Plugins/DialogueKit/Content/Sample/DialogueGraph/NewDataAsset1.uasset
- Plugins/DialogueKit/Source/DialogueKit/Private/ActionSequencer.cpp
- Plugins/DialogueKit/Source/DialogueKit/Private/PortraitItemWidget.cpp
- Plugins/DialogueKit/Source/DialogueKit/Public/ActionSequencer.h
- Plugins/DialogueKit/Source/DialogueKit/Public/DialoguePortraitData.h
- Plugins/DialogueKit/Source/DialogueKit/Public/PortraitItemWidget.h
- 위 7개 변경과 untracked .codex는 기존 사용자 작업이다. 이번에 수정하지 않았다.

## 기준 검증
- DialogueMakerEditor Win64 Development: 성공, 종료 코드 0, 84 build actions, 약 16.70초.
- 로그: Saved/AI/20260915_172311_a0ccf852/Build.log 및 result.json.
- 기존 빌드 경고: DialogueKit의 EnhancedInput 플러그인 의존성 선언 누락. IncrediBuild 라이선스 미활성으로 standalone 실행.
- 엔진 내장 System.Core.Serialization.CbArrayBuffer: 1개 성공. 게임/대화 기능 검증이 아닌 실행 연결 검증이다.
- 보고서: Saved/AI/20260915_172749_70acb6ce/Report/index.json. 실행 로그: 같은 폴더 Test.log.
- 에디터 시작 시 기존 초기화 오류: FPortraitInitData의 Speaker/EmoteType/PortraitSide, FPortraitActionData의 ActionTargetSpeakerID, FItemRow의 ItemType/Value (필드 6개).
- 관련 파일: DialogueKit/Public/DialogueGraph.h, DialoguePortraitData.h, ItemRow.h. 별도 기능/버그 작업에서 조사할 항목이다. 테스트 통과가 이 오류를 해소한 것은 아니다.
- 게임 기능 자동화 테스트, PIE, 패키징: 미실행.
- 없는 테스트 CodexPreparation.NoSuchTest 실행: UE 종료 코드 255 / 래퍼 실패로 정상 거부. 로그: Saved/AI/20260915_173002_6ae1ee4e/Test.log.

## 공통 환경
- 조사일: 2026-09-15 (Asia/Seoul).
- Windows 프로젝트 경로: C:\Users\kjs94\Documents\Unreal Projects.
- 현재 에이전트는 WSL bash에서 작업하며 Windows PowerShell 5.1.26100.9168로 UE/Windows Git을 실행한다.
- 엔진: C:\Program Files\Epic Games\UE_5.4. Windows 등록 경로와 일치한다.
- Build.version: 5.4.4, Changelist 35576357, CompatibleChangelist 33043543.
- 실제 빌드: VS 2022 MSVC 14.38.33144 (설치 디렉터리 14.38.33130), Windows SDK 10.0.19041.0.
- Build.bat가 선택한 .NET SDK: 6.0.302. 별도로 VS 2022 BuildTools도 설치되어 있지만 실제 빌드 로그의 도구를 기준으로 한다.
- 기준 빌드와 내장 테스트는 전체 클린 재빌드/패키징/PIE 기능 테스트를 뜻하지 않는다.

## Git 사용 기준
- Windows Git의 상태를 기준으로 한다. WSL Git에서는 줄바꿈/파일 모드/LFS 환경 차이로 대량 변경 표시가 발생했다. 파일을 정규화하거나 리셋하지 않았다.
- Windows Git에서 잘못된 safe.directory 경로 경고가 있지만 status/빌드는 성공했다. 전역 설정은 수정하지 않았다.
- Fork UI 자체는 자동 조작하지 않았다. 동일 저장소의 로컬 브랜치와 변경을 Windows Git으로 관리한다.
- 준비 파일은 미커밋 상태로 남긴다. 사용자 변경을 포함하는 임의 커밋, 브랜치 전환, push/merge를 하지 않았다.
- Invoke-Git.ps1의 NewBranch는 기존 변경이 있으면 중단한다. DialogueMaker에서 이 보호 동작을 실제 확인했다.

## 자동화 범위와 다음 기능 요청
- 준비됨: 관련 코드 조사/편집, Windows Git 상태 확인과 로컬 브랜치 절차, UE Editor 빌드, 명명된 자동화 테스트 실행과 결과 판독.
- 기능 요청마다 대상 프로젝트, 기대 동작, 재현/완료 조건을 확인하고 관련 코드·테스트 범위를 정한다.
- GUI/PIE 자동 조작 도구는 이번에 연결하지 않았다. 화면/입력 검증은 사용자 실행 확인 또는 별도 도구 연결이 필요하다.
- 초기 환경 준비에서는 UE Python, MCP, 외부 플러그인, 커스텀 에이전트, 신규 Skill을 설치하지 않았다. 이후 사용자 요청으로 역할별 커스텀 에이전트를 추가했으며 현재 구성은 Docs/AI/Agents.md를 따른다.
- 프로젝트 자체 테스트는 Source 및 DialogueKit Source에서 등록 매크로를 찾지 못했다. Blueprint 기능 테스트 자산 내부는 조사하지 않았다.
- 테스트용 게임 저장/로드 검증은 별도 테스트 슬롯/데이터로 설계해야 한다. 실제 사용자 저장을 시험 데이터로 사용하지 않는다.
- 최초 테스트 래퍼에서 새 콘솔 조기 종료와 PowerShell Start-Process 종료 코드 누락을 확인했다. 현재는 .NET Process로 실행하고 실제 프로세스 종료 코드 및 새 JSON 보고서를 함께 확인한다.
