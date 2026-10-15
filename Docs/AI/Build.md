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
2026-10-14부터 프로젝트 자체 `DialogueKit.Baseline` 테스트 3개(선택지 조건·열람 메모리 왕복·고유 슬롯 파일 왕복)를 등록했다. 기능을 추가할 때 해당 요구사항을 검증할 테스트를 선정/작성한다.
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

## DialogueKit 테스트와 결과 판정

```powershell
.\Scripts\AI\Invoke-UE.ps1 -Action Test -TestFilter DialogueKit.Baseline -TimeoutMinutes 5
.\Scripts\AI\Test-AutomationReport.ps1
```

Test 프로세스 실행 이후 실패에도 result.json을 남긴다. Outcome, FailureCategory, ReportFailureCategory, 종료 코드와 테스트 수를 구분한다. 2026-10-15 구조체 초기화 누락 6종을 수정하고 기존 최대12줄 허용 목록을 비웠다. 현재 모든 Error는 실패이며, 같은 초기화 오류의 재발도 허용하지 않는다. 과거 보고서의 KnownBaselineDiagnostics는 당시 결과다. 환경 검사나 기존 실행 프로세스로 인한 시작 전 거부는 콘솔 오류로 보고한다.

개발자용 공개 실행 인자·결과 해석·확장 방법은 [사용자 안내서](../UserGuide.md)를 따른다. 샘플 슬롯은 테스트 실행별 GUID로 격리되며 실제 사용자 슬롯은 사용하지 않는다. 자동화 성공은 GUI/PIE/패키징 성공을 뜻하지 않는다.

## 패키징

```powershell
.\Scripts\AI\Invoke-PackageValidation.ps1 -Action Check
.\Scripts\AI\Invoke-PackageValidation.ps1 -Action Package -TimeoutMinutes 30
```

UE 5.4.4의 BuildCookRun을 사용하며 실행별 Saved/AI/Package_*에 출력한다. 기본 맵은 `/DialogueKit/Sample/Maps/NewWorld`, Win64 Development이다. 기존 사용자 실행 프로세스는 중단하지 않으며 다른 수동 UAT를 동시에 시작하지 않는다. UAT 성공과 새 exe/pak 확인을 PackagedNotLaunched로 기록한다. 이는 실제 패키지 실행/화면/입력/음향 검증이 아니다. 상세 인자와 한계는 사용자 안내서 4절을 따른다.

## Git과 Fork

```powershell
.\Scripts\AI\Invoke-Git.ps1 -Action Status
# 현재 HEAD가 의도한 기준이고 작업 트리가 깨끗할 때만 실행
.\Scripts\AI\Invoke-Git.ps1 -Action NewBranch -Name feature/example
```

Windows Git: C:\Program Files\Git\cmd\git.exe. 저장소의 .git을 그대로 사용하므로 Fork에서 동일한 브랜치/변경을 검토한다.
NewBranch는 현재 HEAD에서 로컬 브랜치만 만들며 더러운 작업 트리, detached HEAD, 기존 브랜치 이름이면 중단한다.
준비 파일과 기존 변경을 먼저 Fork에서 구분해 검토한다. Git 실행 확인은 `AGENTS.md`의 「Git 실행 확인 정책」을 따른다. 데이터 소실 우려가 없는 Git 명령은 사용자 요청 범위 안에서 별도 확인 없이 실행한다. 래퍼에 자동 stash/commit/push/merge 기능이 없다는 사실은 메인의 해당 명령 실행에 별도 확인이 필요하다는 뜻이 아니다.
사용자 요청을 수행하는 데 커밋이 필요하면 git add -- <자신의 명시적 파일> 후 staged diff를 확인하고 일반 commit을 별도 재확인 없이 실행한다. git add . 금지. amend 등 이력 재작성은 Git 실행 확인 정책의 데이터 소실 우려 작업으로 취급한다.
기존 미커밋 변경이 포함된 파일은 사용자 변경을 통째로 커밋하지 말고 필요한 변경 단위로 검토한다.
Git 전역 설정은 수정하지 않는다. Windows Git에서 보인 safe.directory 잘못된 경로 경고는 별도 설정 정리 대상으로 기록했다.
Prototype2는 Git LFS를 사용한다. 에셋이 포인터로 남았으면 Windows Git의 lfs checkout으로 로컬 원본을 복원한다. 로컬에 없는 원본은 대상 remote와 변경 상태를 확인한 뒤 필요한 lfs pull로 가져온다. 에셋 포인터를 직접 수정하지 않는다.

## 완료 보고
변경 이유/파일, 브랜치, 실행 명령과 종료 결과, 테스트 이름·개수·보고서 경로, 기존 경고, 미검증 PIE/패키징을 보고한다.
승인 확인 조건은 각 프로젝트 AGENTS.md를 따른다.

## 지침 출처
OpenAI Docs 스킬로 [공식 AGENTS.md 문서](https://learn.chatgpt.com/docs/agent-configuration/agents-md)를 확인하고 프로젝트 지침에 실행 문서를 연결했다.
참고 대화의 config.toml/외부 플러그인 예시를 그대로 설치하지 않았다. 현재 세션의 모델과 권한 설정을 유지한다.

## S3/S4 실제 재시작 및 버전 배포 검사

`Scripts/AI/Invoke-DialogueRestoreValidation.ps1 -IncludePackage`를 사용한다. 서로 다른 UE 프로세스에서 `ProjectCheckpoint.Write`, `ProjectCheckpoint.Read`를 순차 수행하고, 격리 신규 버전 에셋을 명시적으로 cook한 후 `ProjectCook` 두 검사를 실행한다. 실행별 GUID/manifest/result를 Saved/AI/RestoreValidation에 남긴다. 일반 DialogueKit 전체 테스트에 자동 포함되지 않으며 기존 데이터 정리도 하지 않는다. 사용법/보존 범위는 UserGuide 28절. `Invoke-PackageValidation`의 `DialogueValidationId`는 검증 fixture만 추가하는 선택 인자다.

## S7 패키지 실행과 Shipping 분리

`Invoke-DialogueRestoreValidation.ps1 -IncludePackage`의 GUID와 대응 Development `Package_*/result.json`을 `Invoke-PackagedDialogueValidation.ps1 -PackageResult <절대 경로> -DialogueValidationId <GUID>`에 전달한다. 고유 userdir로 실제 exe의 Write→저장 대기 강제 종료→Read를 실행한다. marker/종료 코드/로그 오류/저장 해시를 함께 판정한다. 자세한 한계와 호출 순서는 UserGuide 44절을 따른다.

`Invoke-PackageValidation.ps1 -Action Package -Configuration Shipping` 뒤 `Invoke-UE.ps1 -Action Test -TestFilter ProjectShipping`을 실행해 현재 cooked Registry의 개발 fixture/태그 제외를 확인한다. Shipping 패키징과 실제 실행은 구분한다. 개발용 명령 harness는 Shipping에 포함하지 않는다.
