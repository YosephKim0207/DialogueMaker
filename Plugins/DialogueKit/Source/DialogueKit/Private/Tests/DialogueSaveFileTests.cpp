#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueSampleSaveCoordinator.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FSaveFixture
{
	TStrongObjectPtr<UDialogueSampleEffectProvider> Provider{NewObject<UDialogueSampleEffectProvider>()};
	TStrongObjectPtr<UDialogueSampleSaveCoordinator> Coordinator{NewObject<UDialogueSampleSaveCoordinator>()};
	FString Prefix = TEXT("DialogueKit_S3_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	FDialogueEffectBatch Batch;
	FDialogueCheckpointRequest Request;
	double Started = FPlatformTime::Seconds();
};

class FWaitForDialogueSave : public IAutomationLatentCommand
{
public:
	FWaitForDialogueSave(FAutomationTestBase* InTest, TSharedRef<FSaveFixture> InFixture) : Test(InTest), Fixture(InFixture) {}
	virtual bool Update() override
	{
		UDialogueSaveQueue* Queue = Fixture->Coordinator->GetSaveQueue();
		if (Queue->IsWriting() && FPlatformTime::Seconds() - Fixture->Started < 20.0)
		{
			return false;
		}
		if (Queue->GetPendingCount() != 0)
		{
			Test->AddError(TEXT("Async save did not complete; isolated slots retained without modification."));
			return true;
		}
		Test->TestEqual(TEXT("Async queue drained"), Queue->GetPendingCount(), 0);
		TStrongObjectPtr<UDialogueSampleEffectProvider> Restored(NewObject<UDialogueSampleEffectProvider>());
		TStrongObjectPtr<UDialogueSampleSaveCoordinator> Reader(NewObject<UDialogueSampleSaveCoordinator>());
		Test->TestTrue(TEXT("Release original owner"), Fixture->Coordinator->Shutdown());
		FName Error;
		Test->TestTrue(TEXT("New coordinator"), Reader->Initialize(Fixture->Prefix, Restored.Get(), Error));
		FDialogueCheckpointSnapshot Snapshot;
		Test->TestTrue(TEXT("Restore actual file"), Reader->RestoreLatest(Snapshot, Error));
		Test->TestEqual(TEXT("Same checkpoint"), Snapshot.CheckpointId, Fixture->Request.Snapshot.CheckpointId);
		int64 Count = 0;
		Restored->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
		Test->TestEqual(TEXT("Only frozen game state restored"), Count, int64(10));
		Test->TestEqual(TEXT("Ledger restored with game"), Restored->ApplyDialogueEffects_Implementation(Fixture->Batch).Status, EDialogueEffectStatus::AlreadyApplied);
		// 새 슬롯의 손상된 내용은 온전한 이전 저장을 덮어쓰지 않는다.
		UDialogueSampleSaveFile* Broken = NewObject<UDialogueSampleSaveFile>();
		Broken->Payload = {1, 2, 3};
		Broken->Checksum = 0;
		Test->TestTrue(TEXT("Write isolated corrupt slot"), UGameplayStatics::SaveGameToSlot(Broken, Fixture->Prefix + TEXT("0"), 0));
		const UDialogueSampleCombinedSave* Fallback = UDialogueSampleSaveCoordinator::LoadLatest(Fixture->Prefix);
		if (Test->TestNotNull(TEXT("Previous valid save survives"), Fallback))
		{
			Test->TestEqual(TEXT("Same last complete snapshot"), Fallback->Request.Snapshot.CheckpointId, Fixture->Request.Snapshot.CheckpointId);
		}
		Reader->Shutdown();
		// 고유한 테스트 소유 슬롯만 정리한다. 실제 사용자 슬롯에는 접근하지 않는다.
		for (int32 Slot = 0; Slot < 2; ++Slot)
		{
			const FString Name = Fixture->Prefix + FString::FromInt(Slot);
			if (UGameplayStatics::DoesSaveGameExist(Name, 0))
			{
				UGameplayStatics::DeleteGameInSlot(Name, 0);
			}
		}
		return true;
	}
private:
	FAutomationTestBase* Test;
	TSharedRef<FSaveFixture> Fixture;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSaveFileTest, "DialogueKit.Checkpoints.AsyncCombinedFile", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 비동기 파일 완료 후 새 객체가 게임 상태와 대화 자료를 같은 저장에서 복원한다.
bool FDialogueSaveFileTest::RunTest(const FString& Parameters)
{
	TSharedRef<FSaveFixture> Fixture = MakeShared<FSaveFixture>();
	FDialogueStateDefinition Definition;
	Definition.Key.Kind = EDialogueStateKind::Count;
	Definition.Key.TargetId = TEXT("Player");
	Definition.Key.ItemId = TEXT("Coin");
	Fixture->Provider->AddDefinition(Definition);
	FDialogueEffect Effect;
	Effect.Key = Definition.Key;
	Effect.Count = 10;
	Fixture->Batch.Context.ExecutionId = FGuid::NewGuid();
	Fixture->Batch.Effects.Add(Effect);
	Fixture->Provider->ApplyDialogueEffects_Implementation(Fixture->Batch);
	FName Error;
	if (TestTrue(TEXT("Initialize"), Fixture->Coordinator->Initialize(Fixture->Prefix, Fixture->Provider.Get(), Error)) == false)
	{
		return false;
	}
	TStrongObjectPtr<UDialogueSampleSaveCoordinator> Duplicate(NewObject<UDialogueSampleSaveCoordinator>());
	TestFalse(TEXT("Reject duplicate slot owner"), Duplicate->Initialize(Fixture->Prefix, Fixture->Provider.Get(), Error));
	Fixture->Request.RequestId = FGuid::NewGuid();
	Fixture->Request.QueueGenerationId = Fixture->Coordinator->GetSaveQueue()->GetGeneration();
	Fixture->Request.Sequence = 1;
	Fixture->Request.Snapshot.CheckpointId = FGuid::NewGuid();
	TestTrue(TEXT("Accepted"), Fixture->Coordinator->RequestDialogueCheckpoint_Implementation(Fixture->Request).bAccepted);
	FDialogueEffectBatch Unsaved = Fixture->Batch;
	Unsaved.Context.ExecutionId = FGuid::NewGuid();
	Fixture->Provider->ApplyDialogueEffects_Implementation(Unsaved);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForDialogueSave(this, Fixture));
	return true;
}
#endif
