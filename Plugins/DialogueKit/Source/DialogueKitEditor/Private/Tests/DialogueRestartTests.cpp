#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueRestoreController.h"
#include "DialogueSampleSaveCoordinator.h"
#include "DialogueVersionLibrary.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"

namespace
{
// 전용 wrapper가 지정한 GUID만 사용한다. 실제 게임 슬롯/기존 에셋과 격리한다.
FString ValidationId()
{
	const FString Id = FPlatformMisc::GetEnvironmentVariable(TEXT("DIALOGUE_VALIDATION_ID"));
	FGuid Parsed;
	return FGuid::ParseExact(Id, EGuidFormats::Digits, Parsed) ? Id : FString();
}
FString Manifest(const FString& Id)
{
	return FPaths::ProjectSavedDir() / TEXT("AI/RestoreValidation") / Id / TEXT("manifest.txt");
}
UDialogueEdGraphNodeBase* AddRestartNode(UEdGraph* Graph, UClass* Type, UEdGraphPin* From)
{
	FNewNodeAction Action(Type, FText(), FText(), FText(), 0);
	return CastChecked<UDialogueEdGraphNodeBase>(Action.PerformAction(Graph, From, FVector2D::ZeroVector));
}
bool SaveFixture(UDialogueGraph* Block)
{
	const FString Filename = FPackageName::LongPackageNameToFilename(Block->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	return UPackage::SavePackage(Block->GetOutermost(), Block, *Filename, Args);
}
struct FRestartFixture
{
	TStrongObjectPtr<UDialogueSampleEffectProvider> Game{NewObject<UDialogueSampleEffectProvider>()};
	TStrongObjectPtr<UDialogueSampleSaveCoordinator> Save{NewObject<UDialogueSampleSaveCoordinator>()};
	TStrongObjectPtr<UDialogueCheckpointSession> Session{NewObject<UDialogueCheckpointSession>()};
	TStrongObjectPtr<UDialogueRestoreController> Restore{NewObject<UDialogueRestoreController>()};
	TStrongObjectPtr<UDialogueBlockLoader> Candidate{NewObject<UDialogueBlockLoader>()};
	FString Id;
	FString Text;
	FGuid NextLine;
	FGuid PriorLine;
	FDialogueCheckpointSnapshot Snapshot;
	TArray<FDialogueBlockCandidate> Candidates;
	double Started = FPlatformTime::Seconds();
	bool bFinished = false;
	bool bRestored = false;
	int32 Phase = 0;
};
class FWaitForRestartWrite : public IAutomationLatentCommand
{
public:
	FWaitForRestartWrite(FAutomationTestBase* NewTest, TSharedRef<FRestartFixture> NewFixture) : Test(NewTest), Fixture(NewFixture) {}
	bool Update() override
	{
		if (Fixture->Save->GetSaveQueue()->GetPendingCount() && FPlatformTime::Seconds() - Fixture->Started < 20) return false;
		if (Test->TestEqual(TEXT("Real async writer drained"), Fixture->Save->GetSaveQueue()->GetPendingCount(), 0))
		{
			IFileManager::Get().MakeDirectory(*FPaths::GetPath(Manifest(Fixture->Id)), true);
			Test->TestTrue(TEXT("Persist handoff after durable save"), FFileHelper::SaveStringToFile(Fixture->Text, *Manifest(Fixture->Id)));
		}
		Fixture->Save->Shutdown();
		return true;
	}
private:
	FAutomationTestBase* Test;
	TSharedRef<FRestartFixture> Fixture;
};
class FWaitForRestartRead : public IAutomationLatentCommand
{
public:
	FWaitForRestartRead(FAutomationTestBase* NewTest, TSharedRef<FRestartFixture> NewFixture) : Test(NewTest), Fixture(NewFixture) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds() - Fixture->Started > 20)
		{
			Test->AddError(TEXT("Restart restore/async save timeout"));
			return true;
		}
		if (Fixture->Phase == 0)
		{
			if (Fixture->bFinished == false) return false;
			if (Test->TestTrue(TEXT("Exact restore callback succeeded"), Fixture->bRestored) == false) return true;
			Test->TestEqual(TEXT("Persisted next line"), Fixture->Session->GetRunner()->GetPresentation().NodeGuid, Fixture->NextLine);
			Test->TestEqual(TEXT("Persisted same execution"), Fixture->Session->GetRunner()->GetBlockRunId(), Fixture->Snapshot.Resume.BlockRunId);
			Test->TestEqual(TEXT("RestoreOnly old version resumed"), Fixture->Session->GetActiveBlock()->GetContentVersion(), 1);
			Test->TestTrue(TEXT("Prior read survives process"), Fixture->Session->GetCheckpointState()->HasReadLine(Fixture->Snapshot.Resume.Block.BlockId, Fixture->PriorLine, 1));
			int64 Count = 0;
			Fixture->Game->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
			Test->TestEqual(TEXT("Reward not repeated on restore"), Count, int64(10));
			// 초기화 직전 미완료 신규 요청도 함께 취소되어야 한다.
			Fixture->Candidate->LoadOverride.BindLambda([](FGuid, const FSoftObjectPath&) { return true; });
			const FGuid Pending = Fixture->Candidate->RequestCandidates(Fixture->Candidates, Fixture->Snapshot.Resume.TargetTag, nullptr, FDialogueStoryProgress(), Fixture->Game.Get(), nullptr);
			Test->TestTrue(TEXT("Death reset captured"), Fixture->Restore->ResetStoryAndSave(false));
			Test->TestFalse(TEXT("Late candidate invalidated by death"), Fixture->Candidate->IsCurrentRequest(Pending));
			Test->TestTrue(TEXT("Death retains read"), Fixture->Session->GetCheckpointState()->HasReadLine(Fixture->Snapshot.Resume.Block.BlockId, Fixture->PriorLine, 1));
			Test->TestTrue(TEXT("Death drops old resume"), Fixture->Session->GetCheckpointState()->GetResumeAnchor().Kind == EDialogueResumeKind::None);
			Fixture->Candidate->LoadOverride.Unbind();
			Fixture->Candidate->RequestCandidates(Fixture->Candidates, Fixture->Snapshot.Resume.TargetTag, nullptr, FDialogueStoryProgress(), Fixture->Game.Get(), nullptr);
			Fixture->Phase = 1;
			return false;
		}
		if (Fixture->Candidate->IsLoading() || Fixture->Save->GetSaveQueue()->GetPendingCount()) return false;
		const FDialogueBlockLoadResult Result = Fixture->Candidate->GetLastResult();
		if (Test->TestTrue(TEXT("New candidate loaded"), Result.bLoaded))
		{
			Test->TestEqual(TEXT("After death chooses current version"), Result.Block->GetContentVersion(), 2);
			Test->TestEqual(TEXT("Same logical block"), Result.Block->GetBlockId(), Fixture->Snapshot.Resume.Block.BlockId);
		}
		Fixture->Save->Shutdown();
		Test->AddInfo(TEXT("Isolated fixture assets and slots retained for cook/audit; no user data changed."));
		return true;
	}
private:
	FAutomationTestBase* Test;
	TSharedRef<FRestartFixture> Fixture;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueRestartWriteTest, "ProjectCheckpoint.Write", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 첫 프로세스가 원본/신버전 신규 에셋과 실제 선택 체크포인트를 생성한다.
bool FDialogueRestartWriteTest::RunTest(const FString& Parameters)
{
	TSharedRef<FRestartFixture> Fixture = MakeShared<FRestartFixture>();
	Fixture->Id = ValidationId();
	if (TestFalse(TEXT("Wrapper GUID supplied"), Fixture->Id.IsEmpty()) == false) return false;
	const FString Folder = TEXT("/Game/DialogueValidation/") + Fixture->Id;
	if (TestFalse(TEXT("Never overwrite fixture"), FPackageName::DoesPackageExist(Folder / TEXT("Old"))) == false) return false;
	auto* Block = NewObject<UDialogueGraph>(CreatePackage(*(Folder / TEXT("Old"))), TEXT("Old"), RF_Public | RF_Standalone);
	Block->InitializeBlockIdentity();
	Block->DialogueTarget = FGameplayTag::RequestGameplayTag(TEXT("DialogueKit.Test.HistoryTarget"));
	Block->StoryScope.Mode = EDialogueStoryScope::Unrestricted;
	Block->Availability = EDialogueBlockAvailability::RestoreOnly;
	auto* Graph = NewObject<UEdGraph>(Block);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	auto* Line = AddRestartNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	auto* Choice = AddRestartNode(Graph, UDialogueChoiceEdGraphNode::StaticClass(), Line->Pins[1]);
	FDialogueOption Option;
	Option.ChoiceId = FGuid::NewGuid();
	Option.Text = FText::FromString(TEXT("Checkpoint reward"));
	FDialogueEffect Effect;
	Effect.Key.Kind = EDialogueStateKind::Count;
	Effect.Key.TargetId = TEXT("Player");
	Effect.Key.ItemId = TEXT("Coin");
	Effect.Count = 10;
	Option.Effects = {Effect};
	CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo())->Options = {Option};
	Choice->OnPropertiesChanged();
	auto* Next = AddRestartNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Choice->Pins[1]);
	AddRestartNode(Graph, UDialogueEdEndGraphNode::StaticClass(), Next->Pins[1]);
	FString ExportError;
	if (TestTrue(TEXT("Export fixture"), FDialogueGraphCodec::ExportGraph(Graph, Block, ExportError)) == false) return false;
	FAssetRegistryModule::AssetCreated(Block);
	FName Error;
	auto* NewVersion = UDialogueVersionLibrary::CreateContentVersion(Block, Folder / TEXT("New"), TEXT("New"), 2, Error);
	if (TestNotNull(TEXT("Create preserved version"), NewVersion) == false) return false;
	if (TestTrue(TEXT("Save original package"), SaveFixture(Block)) == false || TestTrue(TEXT("Save new package"), SaveFixture(NewVersion)) == false) return false;
	FDialogueStateDefinition Definition;
	Definition.Key = Effect.Key;
	Fixture->Game->AddDefinition(Definition);
	Fixture->Save->Initialize(TEXT("DialogueRestart_") + Fixture->Id, Fixture->Game.Get(), Error);
	Fixture->Session->Initialize(Fixture->Save.Get(), Fixture->Game.Get(), Fixture->Game.Get(), nullptr);
	Fixture->Session->AttachSaveQueue(Fixture->Save->GetSaveQueue());
	TestTrue(TEXT("Start source"), Fixture->Session->StartBlock(Block, Block->DialogueTarget));
	TestTrue(TEXT("Save real choice"), Fixture->Session->GetRunner()->SelectChoice(Option.ChoiceId));
	Fixture->Text = Block->GetPathName() + TEXT("\n") + NewVersion->GetPathName() + TEXT("\n") + Next->NodeGuid.ToString() + TEXT("\n") + Line->NodeGuid.ToString() + TEXT("\n") + FString::FromInt(FPlatformProcess::GetCurrentProcessId());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForRestartWrite(this, Fixture));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueRestartReadTest, "ProjectCheckpoint.Read", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 두 번째 프로세스의 새 UObject만으로 실제 파일과 Registry를 복원한다.
bool FDialogueRestartReadTest::RunTest(const FString& Parameters)
{
	TSharedRef<FRestartFixture> Fixture = MakeShared<FRestartFixture>();
	Fixture->Id = ValidationId();
	TArray<FString> Lines;
	if (TestFalse(TEXT("Wrapper GUID supplied"), Fixture->Id.IsEmpty()) == false || TestTrue(TEXT("Read completed writer manifest"), FFileHelper::LoadFileToStringArray(Lines, *Manifest(Fixture->Id))) == false || Lines.Num() != 5) return false;
	TestNotEqual(TEXT("A different actual UE process"), FCString::Atoi(*Lines[4]), int32(FPlatformProcess::GetCurrentProcessId()));
	FGuid::Parse(Lines[2], Fixture->NextLine);
	FGuid::Parse(Lines[3], Fixture->PriorLine);
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous({TEXT("/Game/DialogueValidation/") + Fixture->Id}, true);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FDialogueBlockCandidate Candidate;
		Candidate.AssetPath = FSoftObjectPath(Lines[Index]);
		if (TestTrue(TEXT("Read fixture metadata without load"), FDialogueBlockMetadata::ReadAsset(Registry.GetAssetByObjectPath(Candidate.AssetPath), Candidate.Metadata)) == false) return false;
		Fixture->Candidates.Add(Candidate);
	}
	TestFalse(TEXT("Original not loaded before exact restore"), Registry.GetAssetByObjectPath(FSoftObjectPath(Lines[0])).IsAssetLoaded());
	FName Error;
	Fixture->Save->Initialize(TEXT("DialogueRestart_") + Fixture->Id, Fixture->Game.Get(), Error);
	if (TestTrue(TEXT("Restore same combined disk snapshot"), Fixture->Save->RestoreLatest(Fixture->Snapshot, Error)) == false) return false;
	Fixture->Session->Initialize(Fixture->Save.Get(), Fixture->Game.Get(), Fixture->Game.Get(), nullptr);
	Fixture->Session->AttachSaveQueue(Fixture->Save->GetSaveQueue());
	Fixture->Restore->Initialize(Fixture->Session.Get(), Fixture->Game.Get(), nullptr, Fixture->Candidate.Get());
	const TWeakPtr<FRestartFixture> Weak = Fixture;
	Fixture->Restore->OnRestoredNative.AddLambda([Weak](bool bSuccess, FName)
	{
		if (const auto Pinned = Weak.Pin()) { Pinned->bRestored = bSuccess; Pinned->bFinished = true; }
	});
	TestTrue(TEXT("Exact restore accepted"), Fixture->Restore->Restore(Fixture->Snapshot, nullptr));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForRestartRead(this, Fixture));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueVersionsCookTest, "ProjectCook.DialogueVersions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 배포 Registry에 보존 버전과 현재 버전이 함께 존재하는지 검사한다.
bool FDialogueVersionsCookTest::RunTest(const FString& Parameters)
{
	const FString Id = ValidationId();
	TArray<FString> Lines;
	if (Id.IsEmpty() || FFileHelper::LoadFileToStringArray(Lines, *Manifest(Id)) == false || Lines.Num() != 5) { AddError(TEXT("Missing validation fixture")); return false; }
	FAssetRegistryState Registry;
	if (TestTrue(TEXT("Load real cooked registry"), FAssetRegistryState::LoadFromDisk(*(FPaths::ProjectSavedDir() / TEXT("Cooked/Windows/DialogueMaker/AssetRegistry.bin")), FAssetRegistryLoadOptions(), Registry)) == false) return false;
	TArray<FAssetData> Assets;
	Registry.GetAllAssets(TSet<FName>(), Assets);
	FDialogueBlockReference OldReference;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FAssetData* Asset = Assets.FindByPredicate([&](const FAssetData& Item) { return Item.GetSoftObjectPath().ToString() == Lines[Index]; });
		if (TestNotNull(TEXT("Exact version cooked"), Asset) == false) return false;
		FDialogueBlockReference Reference;
		TestTrue(TEXT("Independent identity tags retained"), FDialogueBlockMetadata::ReadIdentity(*Asset, Reference));
		TestEqual(TEXT("Version retained"), Reference.ContentVersion, Index + 1);
		FDialogueBlockMetadata Metadata;
		TestTrue(TEXT("Selection metadata cooked"), FDialogueBlockMetadata::ReadAsset(*Asset, Metadata));
		TestEqual(TEXT("Availability retained"), Metadata.Availability, Index == 0 ? EDialogueBlockAvailability::RestoreOnly : EDialogueBlockAvailability::Active);
		if (Index == 0) OldReference = Reference;
		else TestEqual(TEXT("Same stable BlockId in cook"), Reference.BlockId, OldReference.BlockId);
	}
	return true;
}
#endif
