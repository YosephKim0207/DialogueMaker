#include "DialogueSampleHost.h"
#include "DialogueSubsystem.h"
#include "DialoguePresentationWidget.h"
#include "DialogueSavingWidget.h"
#include "GameFramework/PlayerController.h"

// 모든 제공자를 명시적으로 연결한다. 테스트 제공자를 실제 게임의 대체물로 자동 설치하지 않는다.
bool UDialogueSampleHost::Initialize(UDialogueSubsystem* NewDialogue, UObject* NewStateProvider, UObject* EffectProvider,
	UObject* SaveProvider, UDialogueSaveQueue* NewQueue, UObject* CorrectionProvider, UDialogueDiagnosticRouter* NewRouter)
{
	if (Dialogue || !IsValid(NewDialogue) || NewDialogue->GetCheckpointSession() || !NewDialogue->GetBlockRunner()->CanStartBlock())
	{
		return false;
	}
	NewDialogue->ConfigureBlockProviders(NewStateProvider, EffectProvider, NewRouter);
	if (!NewDialogue->ConfigureCheckpointSession(SaveProvider, NewQueue))
	{
		return false;
	}
	Dialogue = NewDialogue;
	StateProvider = NewStateProvider;
	Queue = NewQueue;
	Router = NewRouter;
	Loader = NewObject<UDialogueBlockLoader>(this);
	RestoreController = NewObject<UDialogueRestoreController>(this);
	if (!RestoreController->Initialize(Dialogue->GetCheckpointSession(), CorrectionProvider, Router, Loader))
	{
		return false;
	}
	Loader->OnLoadCompletedNative.AddUObject(this, &UDialogueSampleHost::Loaded);
	Dialogue->GetBlockRunner()->OnPresentationChangedNative.AddUObject(this, &UDialogueSampleHost::Presented);
	return true;
}

// 실행/복원/접수 재시도 중 새 후보가 문맥을 덮지 못하게 한다. 로드 대기 중 최신 요청 교체는 허용한다.
bool UDialogueSampleHost::CanRequest() const
{
	const EDialogueResumeKind Kind = Dialogue ? Dialogue->GetCheckpointSession()->GetCheckpointState()->GetResumeAnchor().Kind : EDialogueResumeKind::None;
	return (Kind == EDialogueResumeKind::None || Kind == EDialogueResumeKind::BlockCompleted) && Dialogue && !bStarting && !bHandlingRequest && !RestoreController->IsRestoring()
		&& !Dialogue->GetCheckpointSession()->HasPendingCapture() && Dialogue->GetBlockRunner()->CanStartBlock();
}

// Chapter/Scene과 조건 값은 요청 시점의 게임 상태를 사용한다.
FGuid UDialogueSampleHost::RequestDialogue(FGameplayTag NewTarget, UDialogueStoryDefinition* Story, FDialogueStoryProgress Progress)
{
	if (!CanRequest() || !NewTarget.IsValid()) return FGuid();
	TGuardValue<bool> Guard(bHandlingRequest, true);
	Target = NewTarget;
	ActiveRequest.Invalidate();
	return Loader->RequestFromRegistry(Target, Story, Progress, StateProvider, Router);
}

// 레지스트리 외 카탈로그와 통합 테스트에서도 같은 완료 경로를 사용한다.
FGuid UDialogueSampleHost::RequestCandidates(const TArray<FDialogueBlockCandidate>& Candidates, FGameplayTag NewTarget,
	UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress)
{
	if (!CanRequest() || !NewTarget.IsValid()) return FGuid();
	TGuardValue<bool> Guard(bHandlingRequest, true);
	Target = NewTarget;
	ActiveRequest.Invalidate();
	return Loader->RequestCandidates(Candidates, Target, Story, Progress, StateProvider, Router);
}

// 동기 완료도 Result의 ID를 사용한다. Start가 실패하면 노출/완료 목록은 갱신하지 않는다.
void UDialogueSampleHost::Loaded(const FDialogueBlockLoadResult& Result)
{
	if (!Loader->IsCurrentRequest(Result.RequestId) || RestoreController->IsRestoring() || bStarting) return;
	TGuardValue<bool> Guard(bStarting, true);
	if (!Result.bLoaded)
	{
		OnStarted.Broadcast(false, Result.ErrorCode);
		return;
	}
	ActiveRequest = Result.RequestId;
	ActiveReference = Result.Block->GetBlockReference();
	bCompletionPending = false;
	const bool bStarted = Dialogue->StartDialogueBlockForTarget(Result.Block, Target);
	if (bStarted)
	{
		Loader->RecordBlockExposed(ActiveRequest, ActiveReference);
		if (bCompletionPending) RecordCompletion();
	}
	else
	{
		bPendingStart = Dialogue->GetCheckpointSession()->HasPendingCapture();
		if (!bPendingStart)
		{
			ActiveRequest.Invalidate();
		}
	}
	// 실패 안내와 외부 로그 전송은 게임이 이 결과를 받아 결정한다.
	OnStarted.Broadcast(bStarted, bStarted ? NAME_None : FName(TEXT("BlockStartFailed")));
}

// 정상 완료만 세션 순회에 기록한다. Start→End처럼 동기 완료하면 노출 기록 뒤로 미룬다.
void UDialogueSampleHost::Presented(const FDialoguePresentation& Presentation)
{
	if (Presentation.State == EDialogueRunState::Completed && ActiveRequest.IsValid())
	{
		if (bStarting) bCompletionPending = true;
		else RecordCompletion();
	}
}

// 공통 인사는 Loader가 소진 대상에서 제외하며 완료 요청은 한 번만 보낸다.
void UDialogueSampleHost::RecordCompletion()
{
	Loader->RecordBlockCompleted(ActiveRequest, ActiveReference);
	ActiveRequest.Invalidate();
	bCompletionPending = false;
}

// 저장되지 않은 현재 게임 상태의 폐기/복원은 호출 전 게임 저장 조율자가 수행해야 한다.
bool UDialogueSampleHost::RestoreDialogue(const FDialogueCheckpointSnapshot& Snapshot, UDialogueReplacementMap* Map)
{
	if (!Dialogue || bStarting || bHandlingRequest || Dialogue->GetCheckpointSession()->HasPendingCapture()
		|| (Queue && Queue->GetPendingCount() > 0) || !Dialogue->GetBlockRunner()->CanStartBlock()) return false;
	TGuardValue<bool> Guard(bHandlingRequest, true);
	ActiveRequest.Invalidate();
	return RestoreController->Restore(Snapshot, Map);
}

// 화면/실행과 세션 순회를 종료한다. 저장 큐와 저장 표시 위젯은 완료까지 유지한다.
bool UDialogueSampleHost::EndConversation()
{
	if (!Dialogue || bStarting || bHandlingRequest || Dialogue->GetCheckpointSession()->HasPendingCapture()) return false;
	TGuardValue<bool> Guard(bHandlingRequest, true);
	if (!Dialogue->GetBlockRunner()->CancelBlock()) return false;
	RestoreController->CancelRestore();
	Loader->EndSession();
	ActiveRequest.Invalidate();
	if (PresentationWidget) PresentationWidget->RemoveFromParent();
	return true;
}

// 호출자가 입력 모드/포커스 정책을 적용한다. 저장 표시는 대화보다 긴 수명으로 소유한다.
bool UDialogueSampleHost::ShowPresentation(APlayerController* Player)
{
	if (!Dialogue || !IsValid(Player) || !Player->IsLocalController()) return false;
	if (!PresentationWidget) PresentationWidget = CreateWidget<UDialoguePresentationWidget>(Player);
	if (!PresentationWidget) return false;
	if (!PresentationWidget->IsInViewport()) PresentationWidget->AddToViewport();
	PresentationWidget->BindSubsystem(Dialogue);
	if (Queue)
	{
		if (!SavingWidget) SavingWidget = CreateWidget<UDialogueSavingWidget>(Player);
		if (!SavingWidget) return false;
		if (!SavingWidget->IsInViewport()) SavingWidget->AddToViewport(10);
		SavingWidget->BindQueue(Queue);
	}
	return true;
}

// 실패한 첫 캡처도 원 요청을 유지한다. 재시도 동안 발생한 동기 완료는 노출 뒤 기록한다.
bool UDialogueSampleHost::RetryCapture()
{
	if (!Dialogue || bStarting || bHandlingRequest || RestoreController->IsRestoring())
	{
		return false;
	}
	TGuardValue<bool> Guard(bStarting, true);
	const bool bAccepted = Dialogue->GetCheckpointSession()->RetryCapture();
	if (bAccepted && bPendingStart)
	{
		bPendingStart = false;
		Loader->RecordBlockExposed(ActiveRequest, ActiveReference);
		if (bCompletionPending)
		{
			RecordCompletion();
		}
		OnStarted.Broadcast(true, NAME_None);
	}
	else if (bAccepted && bCompletionPending)
	{
		RecordCompletion();
	}
	return bAccepted;
}
