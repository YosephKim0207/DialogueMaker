#include "DialogueSampleSaveCoordinator.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
TMap<FString, TWeakObjectPtr<UDialogueSampleSaveCoordinator>> SlotOwners;
}


// 전체 합본과 알려진 포맷만 복원 대상으로 인정한다.
bool UDialogueSampleCombinedSave::Validate() const
{
	FName Error;
	if (SchemaVersion != 1 || Revision < 1 || Request.Validate(Error) == false || Game.Validate(Error) == false)
	{
		return false;
	}
	for (const FDialogueEffectEvidence& Evidence : Request.Snapshot.RunEvidence.Effects)
	{
		const FDialogueEffectBatch* Batch = Game.Requests.Find(Evidence.ExecutionId);
		const FDialogueEffectResult* Result = Game.Results.Find(Evidence.ExecutionId);
		if (Batch == nullptr || Result == nullptr || Result->IsSuccess() == false ||
			(Batch->Context.Block == Request.Snapshot.Resume.Block) == false || Batch->Context.NodeGuid != Evidence.NodeGuid ||
			(Request.Snapshot.RunEvidence.bComplete && Batch->Context.BlockRunId != Request.Snapshot.Resume.BlockRunId) ||
			(Request.Snapshot.RunEvidence.bComplete && Batch->ChoiceId != Evidence.ChoiceId))
		{
			return false;
		}
	}
	for (FGuid CorrectionId : Request.Snapshot.AppliedCorrections)
	{
		if (Game.Corrections.ContainsByPredicate([&](const FDialogueSampleCorrectionReceipt& Receipt) { return Receipt.CorrectionId == CorrectionId; }) == false) return false;
	}
	const FDialogueCheckpointTrigger& Trigger = Request.Snapshot.Trigger;
	if (Trigger.bHasEffects)
	{
		const FDialogueEffectBatch* Batch = Game.Requests.Find(Trigger.ExecutionId);
		const FDialogueEffectResult* Result = Game.Results.Find(Trigger.ExecutionId);
		return Batch && Result && Result->IsSuccess() && Batch->Context.Block == Request.Snapshot.Resume.Block &&
			Batch->Context.NodeGuid == Trigger.OriginNodeGuid;
	}
	return true;
}

// CRC는 중단/손상 탐지용이며 악의적인 파일 편집을 막는 인증 수단은 아니다.
UDialogueSampleCombinedSave* UDialogueSampleSaveCoordinator::LoadLatest(const FString& Prefix)
{
	UDialogueSampleCombinedSave* Latest = nullptr;
	for (int32 Slot = 0; Slot < 2; ++Slot)
	{
		const FString Name = Prefix + FString::FromInt(Slot);
		if (UGameplayStatics::DoesSaveGameExist(Name, 0) == false)
		{
			continue;
		}
		UDialogueSampleSaveFile* File = Cast<UDialogueSampleSaveFile>(UGameplayStatics::LoadGameFromSlot(Name, 0));
		if (File == nullptr || File->Payload.IsEmpty() || FCrc::MemCrc32(File->Payload.GetData(), File->Payload.Num()) != File->Checksum)
		{
			continue;
		}
		UDialogueSampleCombinedSave* Save = Cast<UDialogueSampleCombinedSave>(UGameplayStatics::LoadGameFromMemory(File->Payload));
		if (Save && Save->Validate() && (Latest == nullptr || Save->Revision > Latest->Revision))
		{
			Latest = Save;
		}
	}
	return Latest;
}

// 샘플 저장은 사용자가 지정한 전용 슬롯에서만 수행한다.
bool UDialogueSampleSaveCoordinator::Initialize(const FString& NewSlotPrefix, UDialogueSampleEffectProvider* NewProvider, FName& OutError)
{
	OutError = TEXT("InvalidSaveConfiguration");
	if (Queue != nullptr || NewSlotPrefix.IsEmpty() || IsValid(NewProvider) == false ||
		NewSlotPrefix.Contains(TEXT("/")) || NewSlotPrefix.Contains(TEXT("\\")) || NewSlotPrefix.Contains(TEXT(":")))
	{
		return false;
	}
	const FString OwnerKey = NewSlotPrefix.ToLower();
	if (SlotOwners.FindRef(OwnerKey).IsValid())
	{
		OutError = TEXT("SaveSlotAlreadyOwned");
		return false;
	}
	if (LoadLatest(NewSlotPrefix) == nullptr &&
		(UGameplayStatics::DoesSaveGameExist(NewSlotPrefix + TEXT("0"), 0) || UGameplayStatics::DoesSaveGameExist(NewSlotPrefix + TEXT("1"), 0)))
	{
		OutError = TEXT("NoValidExistingSave");
		return false;
	}
	SlotOwners.Add(OwnerKey, this);
	Provider = NewProvider;
	SlotPrefix = NewSlotPrefix;
	if (const UDialogueSampleCombinedSave* Existing = LoadLatest(SlotPrefix))
	{
		Revision = Existing->Revision;
	}
	Queue = NewObject<UDialogueSaveQueue>(this);
	Queue->InitializeQueue(FGuid::NewGuid());
	Queue->OnWriteRequestedNative.BindUObject(this, &UDialogueSampleSaveCoordinator::WriteSave);
	OutError = NAME_None;
	return true;
}

// 효과 성공 경계에서 호출하여 이후 상태가 스냅샷에 섞이지 않게 한다.
FDialogueCheckpointAcceptance UDialogueSampleSaveCoordinator::RequestDialogueCheckpoint_Implementation(const FDialogueCheckpointRequest& Request)
{
	FDialogueCheckpointAcceptance Result;
	Result.ErrorCode = TEXT("SaveCoordinatorNotInitialized");
	if (IsValid(Queue) == false || IsValid(Provider) == false)
	{
		return Result;
	}
	if (PendingCapture && PendingCapture->Request.Snapshot.CheckpointId != Request.Snapshot.CheckpointId)
	{
		Result.ErrorCode = TEXT("PendingCaptureMustBeRetried");
		return Result;
	}
	if (Request.Validate(Result.ErrorCode) == false) return Result;
	if (Queue->ReserveCapture(Request.Snapshot.CheckpointId) == false)
	{
		Result.ErrorCode = TEXT("EarlierCapturePending");
		return Result;
	}
	if (PendingCapture == nullptr)
	{
		PendingCapture = NewObject<UDialogueSampleCombinedSave>(this);
		PendingCapture->Request = Request;
		PendingCapture->Game = Provider->CaptureEffectSnapshot();
	}
	UDialogueSampleCombinedSave* Capture = PendingCapture;
	FDialogueCheckpointRequest FrozenRequest = Request;
	FrozenRequest.Snapshot = Capture->Request.Snapshot;
	if (Capture->Game.Validate(Result.ErrorCode) == false)
	{
		return Result;
	}
	TArray<uint8> Bytes;
	if (UGameplayStatics::SaveGameToMemory(Capture, Bytes) == false)
	{
		Result.ErrorCode = TEXT("SnapshotSerializationFailed");
		return Result;
	}
	Result = Queue->Enqueue(FrozenRequest, Bytes);
	if (Result.bAccepted) PendingCapture = nullptr;
	return Result;
}

// 직렬화는 게임 스레드에서, 실제 파일 기록은 UE 비동기 저장으로 수행한다.
void UDialogueSampleSaveCoordinator::WriteSave(const FDialogueQueuedSave& Save)
{
	FDialogueCheckpointCompletion Completion;
	Completion.RequestId = Save.Request.RequestId;
	Completion.QueueGenerationId = Save.Request.QueueGenerationId;
	Completion.Sequence = Save.Request.Sequence;
	Completion.ErrorCode = TEXT("SaveSerializationFailed");
	UDialogueSampleCombinedSave* Combined = Cast<UDialogueSampleCombinedSave>(UGameplayStatics::LoadGameFromMemory(Save.GamePayload));
	if (Combined == nullptr || Revision == MAX_int64)
	{
		Queue->CompleteWrite(Completion);
		return;
	}
	Combined->Request = Save.Request;
	Combined->Revision = Revision + 1;
	UDialogueSampleSaveFile* File = NewObject<UDialogueSampleSaveFile>();
	if (Combined->Validate() == false || UGameplayStatics::SaveGameToMemory(Combined, File->Payload) == false)
	{
		Queue->CompleteWrite(Completion);
		return;
	}
	File->Checksum = FCrc::MemCrc32(File->Payload.GetData(), File->Payload.Num());
	const int64 WrittenRevision = Combined->Revision;
	const FString Name = SlotPrefix + FString::FromInt(int32(WrittenRevision % 2));
	const TSharedRef<TStrongObjectPtr<UDialogueSampleSaveCoordinator>> KeepAlive = MakeShared<TStrongObjectPtr<UDialogueSampleSaveCoordinator>>(this);
	FAsyncSaveGameToSlotDelegate Delegate = FAsyncSaveGameToSlotDelegate::CreateLambda(
		[this, KeepAlive, Completion, WrittenRevision](const FString&, int32, bool bSuccess) mutable
		{
			Completion.bSaved = bSuccess;
			Completion.ErrorCode = bSuccess ? NAME_None : FName(TEXT("AsyncSaveFailed"));
			if (bSuccess)
			{
				Revision = WrittenRevision;
			}
			// 개발자는 실패 완료 이벤트를 게임의 로그 전송 및 재시도 UI에 연결한다.
			Queue->CompleteWrite(Completion);
		});
	UGameplayStatics::AsyncSaveGameToSlot(File, Name, 0, Delegate);
}

// 미저장 메모리와 섞지 않고 검증된 마지막 합본 전체를 적용한다.
bool UDialogueSampleSaveCoordinator::RestoreLatest(FDialogueCheckpointSnapshot& OutSnapshot, FName& OutError)
{
	OutError = TEXT("SaveRestoreUnavailable");
	if (Queue == nullptr || Queue->GetPendingCount() != 0 || PendingCapture != nullptr || IsValid(Provider) == false)
	{
		return false;
	}
	UDialogueSampleCombinedSave* Save = LoadLatest(SlotPrefix);
	if (Save == nullptr || Queue->InitializeQueue(FGuid::NewGuid()) == false ||
		Provider->RestoreEffectSnapshot(Save->Game, OutError) == false)
	{
		return false;
	}
	OutSnapshot = Save->Request.Snapshot;
	Revision = Save->Revision;
	OutError = NAME_None;
	return true;
}

// 모든 요청이 처리된 뒤에만 슬롯 소유권을 해제한다.
bool UDialogueSampleSaveCoordinator::Shutdown()
{
	if (Queue == nullptr || Queue->GetPendingCount() != 0 || PendingCapture != nullptr)
	{
		return false;
	}
	if (SlotOwners.FindRef(SlotPrefix.ToLower()).Get() == this)
	{
		SlotOwners.Remove(SlotPrefix.ToLower());
	}
	Queue->OnWriteRequestedNative.Unbind();
	Queue = nullptr;
	Provider = nullptr;
	SlotPrefix.Reset();
	return true;
}

// 디스크 실패로 head가 대기 중이어도 후속 불변 자료의 접수는 가능하다.
bool UDialogueSampleSaveCoordinator::CanCaptureDialogueCheckpoint_Implementation() const
{
	return IsValid(Queue) && IsValid(Provider) && PendingCapture == nullptr;
}

// 정상 입력의 메모리 접수는 즉시 수행하고 파일 완료와 분리한다.
FDialogueCheckpointAcceptance UDialogueSampleSaveCoordinator::CaptureDialogueCheckpoint_Implementation(const FDialogueCheckpointSnapshot& Snapshot)
{
	if (IsValid(Queue) == false || IsValid(Provider) == false)
	{
		return FDialogueCheckpointAcceptance();
	}
	return RequestDialogueCheckpoint_Implementation(Queue->MakeRequest(Snapshot));
}
