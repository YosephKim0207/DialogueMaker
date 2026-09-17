# 빌드·검증 및 Fork 작업 절차

## 실행 환경
Windows PowerShell 5.1, UE 5.4.4. 프로젝트 루트에서 실행한다.
WSL에서 Windows 실행이 socket failed로 차단되면 승인된 Windows 실행 권한으로 재시도한다.
엔진 또는 사용자 LocalAppData의 빌드 캐시에 쓰기가 필요하다. 권한을 자동으로 넓히는 설정은 추가하지 않았다.

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Check
.\Scripts\AI\Invoke-UE.ps1 -Action Build
```

실제 빌드: Engine/Build/BatchFiles/Build.bat DialogueMakerEditor Win64 Development -Project=<현재 프로젝트의 절대 경로> -WaitMutex -NoHotReloadFromIDE.
엔진 기본 경로는 C:\Program Files\Epic Games\UE_5.4이며 -EngineRoot로 지정 가능하다. 실제 Build.version이 5.4.4가 아니면 중단한다.
엔진의 Build.bat가 번들 .NET 6.0.302를 선택한다. 프로젝트 루트에서 dotnet build로 대체하지 않는다.
스크립트는 프로젝트 폴더 이름과 uproject 이름이 같은 현재 구조를 사용한다. 폴더를 다른 이름으로 복사한 경우 먼저 스크립트를 조정한다.

빌드 로그: Saved/AI/<시각_GUID>/Build.log 및 result.json. ExitCode 0만 빌드 성공이다.
스크립트끼리는 공통 뮤텍스로 직렬 실행한다. 다른 에디터/MSBuild/LiveCoding 프로세스가 있으면 중단한다.
외부 IDE에서 새 빌드를 동시에 시작하지 않는다. 빌드 자체의 중복 실행은 UBT -WaitMutex로도 조정한다.

## 자동화 테스트
현재 프로젝트 자체 C++ 자동화 테스트 등록은 발견하지 못했다. 기능을 추가할 때 해당 요구사항을 검증할 테스트를 선정/작성한다.
다음은 연결 검증용 엔진 내장 테스트이며 게임/대화 기능의 검증이 아니다.

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter System.Core.Serialization.CbArrayBuffer -TimeoutMinutes 5
```

실제 명령은 UnrealEditor-Cmd.exe <project> "-ExecCmds=Automation RunTests <filter>; Quit" -ReportExportPath=<새 폴더> -abslog=<새 로그> -unattended -nop4 -nosplash -NullRHI -nosound.
Quit은 같은 Automation 명령 큐에 들어간다. 시작하자마자 종료시키는 별도의 콘솔 Quit을 사용하지 않는다.
새 Report/index.json의 tests 배열과 각 state를 검사한다. 0개/누락/실패/미실행은 통과가 아니다.
현재 래퍼는 공백 없는 테스트 이름/접두사 하나를 받는다. 테스트 시간 제한 초과 시 래퍼가 띄운 프로세스만 종료한다.
NullRHI 실행은 렌더링/UMG 시각 검증에 사용할 수 없다. 화면·입력 변경은 별도 PIE 시나리오가 필요하다.
실행 한정 -ini:Engine:[/Script/Sentry.SentrySettings]:InitAutomatically=False 옵션으로 Sentry 자동 초기화를 끈다. 기존 진단 데이터 정리/송신을 피하기 위한 테스트 옵션이며 프로젝트 설정을 저장하지 않는다. Sentry 통합 자체는 이 실행으로 검증하지 않는다.
성공한 테스트와 별도로 전체 실행 로그의 Error 줄 수를 result.json에 기록한다. 시작 오류가 있으면 경고를 표시하고 원인을 보고한다.

## Git과 Fork

```powershell
.\Scripts\AI\Invoke-Git.ps1 -Action Status
# 현재 HEAD가 의도한 기준이고 작업 트리가 깨끗할 때만 실행
.\Scripts\AI\Invoke-Git.ps1 -Action NewBranch -Name feature/example
```

Windows Git: C:\Program Files\Git\cmd\git.exe. 저장소의 .git을 그대로 사용하므로 Fork에서 동일한 브랜치/변경을 검토한다.
NewBranch는 현재 HEAD에서 로컬 브랜치만 만들며 더러운 작업 트리, detached HEAD, 기존 브랜치 이름이면 중단한다.
준비 파일과 기존 변경을 먼저 Fork에서 구분해 검토한다. 래퍼가 자동 stash/commit/push/merge를 하지 않는다.
실제 기능 작업에서 커밋 요청을 받으면 git add -- <자신의 명시적 파일> 후 staged diff를 확인한다. git add . 금지.
기존 미커밋 변경이 포함된 파일은 사용자 변경을 통째로 커밋하지 말고 필요한 변경 단위로 검토한다.
Git 전역 설정은 수정하지 않는다. Windows Git에서 보인 safe.directory 잘못된 경로 경고는 별도 설정 정리 대상으로 기록했다.
Prototype2는 Git LFS를 사용한다. 에셋이 포인터로 남았으면 Windows Git의 lfs checkout으로 로컬 원본을 복원한다. 로컬에 없는 원본은 대상 remote와 변경 상태를 확인한 뒤 필요한 lfs pull로 가져온다. 에셋 포인터를 직접 수정하지 않는다.

## 완료 보고
변경 이유/파일, 브랜치, 실행 명령과 종료 결과, 테스트 이름·개수·보고서 경로, 기존 경고, 미검증 PIE/패키징을 보고한다.
승인 확인 조건은 각 프로젝트 AGENTS.md를 따른다.

## 지침 출처
OpenAI Docs 스킬로 [공식 AGENTS.md 문서](https://learn.chatgpt.com/docs/agent-configuration/agents-md)를 확인하고 프로젝트 지침에 실행 문서를 연결했다.
참고 대화의 config.toml/외부 플러그인 예시를 그대로 설치하지 않았다. 현재 세션의 모델과 권한 설정을 유지한다.
