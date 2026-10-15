#include "DialoguePackageValidation.h"
#if !UE_BUILD_SHIPPING
#include "DialogueSampleHost.h"
#include "DialogueSubsystem.h"
#include "DialogueSampleSaveCoordinator.h"
#include "DialoguePreviewPolicy.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FPackageValidation
{
	FString Id;
	FString Phase;
	int32 Step = 0;
	double Started = FPlatformTime::Seconds();
	bool bRestoreDone = false;
	bool bRestoreSuccess = false;
	TStrongObjectPtr<UGameInstance> Instance;
	TStrongObjectPtr<UDialogueSubsystem> Dialogue;
	TStrongObjectPtr<UDialogueSampleHost> Host;
	TStrongObjectPtr<UDialogueSampleEffectProvider> Game;
	TStrongObjectPtr<UDialogueSampleSaveCoordinator> Save;
	TStrongObjectPtr<UDialogueGraph> Old;
	FDialogueBlockCandidate Current;
	FDialogueCheckpointSnapshot Snapshot;
	// 결과는 userdir 아래에만 기록한다. 외부 수신/프로젝트 설정 변경은 하지 않는다.
	void Report(const FString& Status, const FString& Detail)
	{
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("DialogueValidation");
		IFileManager::Get().MakeDirectory(*Directory, true);
		const FString Text = FString::Printf(TEXT("Status=%s\nPhase=%s\nId=%s\nPid=%u\nSavedDir=%s\nDetail=%s\n"),
			*Status, *Phase, *Id, FPlatformProcess::GetCurrentProcessId(), *FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), *Detail);
		FFileHelper::SaveStringToFile(Text, *(Directory / (Phase + TEXT(".txt"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
	// 종료 성공과 검증 실패를 별도 코드 및 결과로 남긴다.
	bool Finish(bool bSuccess, const FString& Detail)
	{
		Report(bSuccess ? TEXT("Success") : TEXT("Failed"), Detail);
		if (Save.IsValid() && Save->GetSaveQueue() && Save->GetSaveQueue()->GetPendingCount() == 0) Save->Shutdown();
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
		return false;
	}
	// 패키지 Registry와 원본/신버전 리소스를 실제 로드해 샘플 저장 계약을 검사한다.
	bool Tick(float)
	{
		if (FPlatformTime::Seconds() - Started > 90) return Finish(false, TEXT("Timeout"));
		if (!GEngine) return true;
		if (Step == 0)
		{
			if (UDialoguePreviewPolicy::IsBlockPreviewActive()) return Finish(false, TEXT("PreviewActiveInGame"));
			const FString Root = TEXT("/Game/DialogueValidation/") + Id;
			IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
			FDialogueBlockMetadata OldMetadata;
			if (!FDialogueBlockMetadata::ReadAsset(Registry.GetAssetByObjectPath(FSoftObjectPath(Root / TEXT("Old.Old"))), OldMetadata)) return Finish(false, TEXT("OldMetadataMissing"));
			Current.AssetPath = FSoftObjectPath(Root / TEXT("New.New"));
			if (!FDialogueBlockMetadata::ReadAsset(Registry.GetAssetByObjectPath(Current.AssetPath), Current.Metadata)) return Finish(false, TEXT("CurrentMetadataMissing"));
			if (OldMetadata.Block.BlockId != Current.Metadata.Block.BlockId || OldMetadata.Block.ContentVersion != 1 || Current.Metadata.Block.ContentVersion != 2
				|| OldMetadata.Availability != EDialogueBlockAvailability::RestoreOnly || Current.Metadata.Availability != EDialogueBlockAvailability::Active) return Finish(false, TEXT("VersionMetadataMismatch"));
			Old.Reset(LoadObject<UDialogueGraph>(nullptr, *(Root / TEXT("Old.Old"))));
			if (!Old.IsValid()) return Finish(false, TEXT("OldLoadFailed"));
			Instance.Reset(NewObject<UGameInstance>());
			Dialogue.Reset(NewObject<UDialogueSubsystem>(Instance.Get()));
			Game.Reset(NewObject<UDialogueSampleEffectProvider>());
			Save.Reset(NewObject<UDialogueSampleSaveCoordinator>());
			Host.Reset(NewObject<UDialogueSampleHost>());
			FName Error;
			if (!Save->Initialize(TEXT("PackageValidation_") + Id, Game.Get(), Error)) return Finish(false, Error.ToString());
			if (!Host->Initialize(Dialogue.Get(), Game.Get(), Game.Get(), Save.Get(), Save->GetSaveQueue(), Game.Get(), nullptr)) return Finish(false, TEXT("HostInitializeFailed"));
			if (Phase == TEXT("Write"))
			{
				FDialogueStateDefinition Definition;
				Definition.Key.Kind = EDialogueStateKind::Count;
				Definition.Key.TargetId = TEXT("Player");
				Definition.Key.ItemId = TEXT("Coin");
				Game->AddDefinition(Definition);
				if (!Dialogue->StartDialogueBlockForTarget(Old.Get(), Old->DialogueTarget) || Dialogue->GetBlockPresentation().Choices.Num() != 1) return Finish(false, TEXT("StartOldFailed"));
				if (!Dialogue->SelectBlockChoice(Dialogue->GetBlockPresentation().Choices[0].ChoiceId)) return Finish(false, TEXT("SelectFailed"));
				Step = 1;
			}
			else
			{
				if (!Save->RestoreLatest(Snapshot, Error)) return Finish(false, Error.ToString());
				Host->GetRestoreController()->OnRestoredNative.AddLambda([this](bool bSuccess, FName) { bRestoreDone = true; bRestoreSuccess = bSuccess; });
				if (!Host->RestoreDialogue(Snapshot, nullptr)) return Finish(false, TEXT("RestoreRejected"));
				Step = 2;
			}
			return true;
		}
		if (Step == 1)
		{
			if (Save->GetSaveQueue()->GetPendingCount()) return true;
			return Finish(UDialogueSampleSaveCoordinator::LoadLatest(TEXT("PackageValidation_") + Id) != nullptr, TEXT("DurableChoiceCheckpoint"));
		}
		if (Step == 2)
		{
			if (!bRestoreDone) return true;
			if (!bRestoreSuccess || !Dialogue->GetCheckpointSession()->GetActiveBlock()
				|| !(Dialogue->GetCheckpointSession()->GetActiveBlock()->GetBlockReference() == Snapshot.Resume.Block)
				|| Snapshot.Resume.Block.ContentVersion != 1 || Dialogue->GetBlockPresentation().NodeGuid != Snapshot.Resume.NodeGuid || Dialogue->GetBlockRunner()->GetBlockRunId() != Snapshot.Resume.BlockRunId) return Finish(false, TEXT("ExactRestoreMismatch"));
			int64 Count = 0;
			Game->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
			if (Count != 10 || Save->GetSaveQueue()->GetPendingCount() != 0) return Finish(false, TEXT("RewardDuplicatedOrUnsavedStateRestored"));
			if (Phase == TEXT("Crash"))
			{
				// 실제 강제 종료의 시점을 결정적으로 만들기 위해 디스크 쓰기 직전 큐를 보류한다.
				// OS 파일 쓰기 중 전원 차단을 재현하는 시험과는 구분한다.
				Save->GetSaveQueue()->OnWriteRequestedNative.BindLambda([this](const FDialogueQueuedSave&) { Report(TEXT("Pending"), TEXT("WriteQueuedBeforeDisk")); });
				FDialogueEffectBatch Batch;
				Batch.Context.Block = Snapshot.Resume.Block;
				Batch.Context.BlockRunId = Snapshot.Resume.BlockRunId;
				Batch.Context.NodeGuid = Snapshot.Resume.NodeGuid;
				Batch.Context.ExecutionId = FGuid::NewGuid();
				FDialogueEffect Effect;
				Effect.Key.Kind = EDialogueStateKind::Count;
				Effect.Key.TargetId = TEXT("Player");
				Effect.Key.ItemId = TEXT("Coin");
				Effect.Count = 10;
				Batch.Effects.Add(Effect);
				Game->ApplyDialogueEffects_Implementation(Batch);
				Game->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
				if (Count != 20) return Finish(false, TEXT("UnsavedRewardNotApplied"));
				if (!Dialogue->GetCheckpointSession()->SaveCurrentState()) return Finish(false, TEXT("PendingCaptureFailed"));
				Step = 4;
				return true;
			}
			if (!Dialogue->GetBlockRunner()->AdvanceLine()) return Finish(false, TEXT("RestoredEndFailed"));
			if (!Host->RequestCandidates({Current}, Snapshot.Resume.TargetTag, nullptr, FDialogueStoryProgress()).IsValid()) return Finish(false, TEXT("CurrentRequestFailed"));
			Step = 3;
			return true;
		}
		if (Step == 3)
		{
			if (Host->GetLoader()->IsLoading()) return true;
			return Finish(Host->GetLoader()->GetLastResult().bLoaded && Dialogue->GetCheckpointSession()->GetActiveBlock() && Dialogue->GetCheckpointSession()->GetActiveBlock()->GetContentVersion() == 2
				&& Dialogue->GetBlockPresentation().Choices.Num() == 1, TEXT("OldExactRestoreThenCurrentCandidate"));
		}
		return true;
	}
};
TSharedPtr<FPackageValidation> Validation;
FTSTicker::FDelegateHandle ValidationTick;
}
#endif

// 명시적 GUID와 고유 userdir가 없으면 일반 게임 실행을 변경하지 않는다.
void StartDialoguePackageValidation()
{
#if !UE_BUILD_SHIPPING
	FString Id;
	FString Phase;
	FString UserDir;
	FGuid Parsed;
	if (!FParse::Value(FCommandLine::Get(), TEXT("DialoguePackageValidation="), Id)) return;
	if (!FGuid::ParseExact(Id, EGuidFormats::Digits, Parsed)
		|| !FParse::Value(FCommandLine::Get(), TEXT("DialogueValidationPhase="), Phase)
		|| (Phase != TEXT("Write") && Phase != TEXT("Crash") && Phase != TEXT("Read"))
		|| !FParse::Value(FCommandLine::Get(), TEXT("userdir="), UserDir) || FPaths::IsRelative(UserDir))
	{
		FPlatformMisc::RequestExitWithStatus(false, 2);
		return;
	}
	Validation = MakeShared<FPackageValidation>();
	Validation->Id = Id;
	Validation->Phase = Phase;
	ValidationTick = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateSP(Validation.ToSharedRef(), &FPackageValidation::Tick));
#endif
}

// 모듈 종료 시 검증 객체와 ticker를 명시적으로 정리한다.
void StopDialoguePackageValidation()
{
#if !UE_BUILD_SHIPPING
	FTSTicker::GetCoreTicker().RemoveTicker(ValidationTick);
	Validation.Reset();
#endif
}
