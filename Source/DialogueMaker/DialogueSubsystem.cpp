// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueSubsystem.h"

#include "DialogueBranchNodeInfoBase.h"
#include "DialogueNodeInfo.h"
#include "DialogueProgressSaveData.h"
#include "DialogueSettings.h"
#include "GameplayTags.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueDubsSubsystem, Log, All);

// NPC의 호출로 현재 노출 가능한 Dialogue를 Show하기 위한 함수
void UDialogueSubsystem::BeginDialogue(ENPCID NPCID)
{
	if (OnCurrentDialogueChanged.IsBound() == false)
	{
		OnCurrentDialogueChanged.BindUObject(this, &UDialogueSubsystem::UpdateCurrentDialogueNode);
	}

	if (OnDialogueReady.IsBound() == false)
	{
		OnDialogueReady.BindUObject(this, &UDialogueSubsystem::StartDialogue);
	}
	
	GetDialogueGraph(NPCID);
}

// Player가 갖고 있는 GameplayTagContainer을 통해 현재의 Condition을 점검하여 현 상황에서 노출 가능한 DialogueGraph 에셋들을 추출한다. 
void UDialogueSubsystem::GetDialogueGraph(ENPCID NPCID)
{
	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::GetDialogueGraph : Enter"));
	
	// NPCID, ChapterID 기준 필터 제작
	// TODO 현재 Player가 진행 중인 ChapterID 가져오기
	EChapterID TEMPChapterID = EChapterID::Chapter01;
	FARFilter ARFilter = GetDialogueGraphAssetFilter(NPCID, TEMPChapterID);

	// 제작된 필터로부터 Assets 가져오기
	TArray<FAssetData> AssetDatas;
	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	AssetRegistry.GetAssets(ARFilter, AssetDatas);

	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::GetDialogueGraph : Start Asset Check"));
	
	TArray<FAssetData> CandidateAssetDatas;
	for (const FAssetData& AssetData : AssetDatas)
	{
		if (IsCandidateDialogueGraphAsset(AssetData) == false)
		{
			continue;
		}

		const FPrimaryAssetId AssetId = AssetData.GetPrimaryAssetId();
		if (AssetId.IsValid())
		{
			CandidateAssetDatas.Add(AssetData);
			
			UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::GetDialogueGraph : Add Candidate %s"), *AssetId.ToString());
		}
		else
		{
			UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::GetDialogueGraph : Valid Error %s"), *AssetData.GetSoftObjectPath().ToString());
		}
	}
	
	// 우선순위에 따른 정렬
	CandidateAssetDatas.Sort([] (const FAssetData& A, const FAssetData& B)
	{
		FString DialogueGraphTypeStringA = A.GetTagValueRef<FString>("DialogueGraphType");
		FString DialogueGraphTypeStringB = B.GetTagValueRef<FString>("DialogueGraphType");

		const UEnum* Enum = StaticEnum<EDialogueGraphType>();
		int32 DialogueGraphTypeA = Enum->GetValueByNameString(DialogueGraphTypeStringA);
		int32 DialogueGraphTypeB = Enum->GetValueByNameString(DialogueGraphTypeStringB);

		// Enum값이 낮을 경우 높은 우선순위 (추가되는 Enum목록 가능성 대비)
		if (DialogueGraphTypeA != DialogueGraphTypeB)
		{
			return DialogueGraphTypeA < DialogueGraphTypeB;	
		}

		// 가중치가 높은 경우 높은 우선순위
		const int32 PriorityWeightA = A.GetTagValueRef<int32>("DialoguePriorityWeight");
		const int32 PriorityWeightB = B.GetTagValueRef<int32>("DialoguePriorityWeight");
		return PriorityWeightA > PriorityWeightB;
	});

	// PrimaryId 리스트화
	TArray<FPrimaryAssetId> AssetIds;
	for (const FAssetData& AssetData : CandidateAssetDatas)
	{
		AssetIds.Add(AssetData.GetPrimaryAssetId());
	}

	// DialogueGraph Asset Load
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FName> Bundles;
	FStreamableDelegate OnDialogueLoaded = FStreamableDelegate::CreateUObject(this, &UDialogueSubsystem::OnDialogueLoaded);
	CurrentHandle = AssetManager.LoadPrimaryAssets(AssetIds, Bundles, OnDialogueLoaded);
}

// 해당 NPC Actor와 대화 시작
void UDialogueSubsystem::StartDialogue(UDialogueGraph* DialogueGraph)
{
	CurrentDialogueGraph = DialogueGraph;
	InitializeDialogueData();

	CreateDialogueUI();
}

// Dialogue Node의 Type이 End에 도착하는 경우 호출
void UDialogueSubsystem::EndDialogue()
{
	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::EndDialogue : Enter"));
	
	SetInputSettings(false);

	// TODO 진행 상황 저장

	// TODO UI에 Bind해둔 델리게이트 Execute(), UI에서는 위젯을 REmoveFromParent 혹은 VIsiblity만 collaps로 조절
	if (OnDialogueEnded.IsBound())
	{
		OnDialogueEnded.Broadcast();

		UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::EndDialogue : Broadcast"));
	}
}

/* 현재 진행 중인 대화 정보를 반환
 * StartDialogue, SaveCurrentDialogue를 위해 호출
*/
UDialogueNodeInfo* UDialogueSubsystem::GetDialogueNodeInfo(FGuid DialogueNodeGuid)
{
	if (DialogueNodeGuid.IsValid() == false)
	{
		UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetDialogueNodeInfo : Dialogue Guid is invalid"));

		return nullptr;
	}

	UDialogueRuntimeNode* DialogueNode = GetDialogueNode(DialogueNodeGuid);
	
	if (DialogueNode == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetDialogueNodeInfo : Dialogue is not cached"));

		DialogueNode = GetFirstNode();
		if (DialogueNode == nullptr)
		{
			UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetDialogueNodeInfo : First Node is nullptr"));
			return nullptr;
		}
		
		return Cast<UDialogueNodeInfo>(DialogueNode->NodeInfo);
	}
	
	return Cast<UDialogueNodeInfo>(DialogueNode->NodeInfo);
}

// Guid로부터 캐싱된 DialogueRuntimeNode를 반환
UDialogueRuntimeNode* UDialogueSubsystem::GetDialogueNode(FGuid DialogueNodeGuid)
{
	if (IdToNodeMap.Contains(DialogueNodeGuid))
	{
		return IdToNodeMap[DialogueNodeGuid];
	}

	UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetDialogueNode : Dialogue is not cached"));
	
	return nullptr;
}

// Dialogue Graph의 첫 번째 DialogueNodeInfo 반환
UDialogueRuntimeNode* UDialogueSubsystem::GetFirstNode()
{
	FGuid FirstDialogueNodeGuid;
	for (UDialogueRuntimeNode* DialogueRuntimeNode : CurrentDialogueGraph->Graph->Nodes)
	{
		if (DialogueRuntimeNode->DialogueNodeType == EDialogueType::StartNode)
		{
			// StartNode의 OutputPin은 항시 1개로 가정한다
			UDialogueRuntimePin* OutputPin = DialogueRuntimeNode->OutputPins[0];	
			if (OutputPin->Connections.Num() == 0)
			{
				UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetFirstDialogue : There is no Connected pin with Start Node"));
				return nullptr;
			}

			// 1개의 OutputPin은 항상 1개의 InputPin과 연결됨을 가정한다
			UDialogueRuntimePin* LinkedPin = OutputPin->Connections[0];	
			FirstDialogueNodeGuid = LinkedPin->OwnerNodeGuid;
			break;
		}
	}

	if (IdToNodeMap.Contains(FirstDialogueNodeGuid))
	{
		CurrentOngoingNodeGuid = FirstDialogueNodeGuid;
		
		return IdToNodeMap[FirstDialogueNodeGuid];
	}

	UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetFirstDialogue : There is no First Dialogue Node"));
	return nullptr;
}

// 현재 진행 중인 대화문에서 Choices가 있는지 여부 반환
bool UDialogueSubsystem::HasChoicesInCurrentDialogue(UDialogueNodeInfo* DialogueNodeInfo) const
{
	if (DialogueNodeInfo == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::HasChoicesInCurrentDialogue : DialogueNodeInfo is nullptr"));
		return false;
	}

	if (DialogueNodeInfo->DialogueResponses.Num() > 1)
	{
		return true;
	}
	
	return false;
}

UDialogueNodeInfo* UDialogueSubsystem::ProgressNextDialogue(const int32 SelectedChoiceIndex, const bool bIsFirstDialogue)
{
	UDialogueRuntimeNode* NextRuntimeNode = bIsFirstDialogue ? GetFirstNode() : GetNextNode(SelectedChoiceIndex);
	if (NextRuntimeNode == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::ProgressNextDialogue : NextRuntimeNode is nullptr"));

		EndDialogue();
		return nullptr;
	}

	// Current Ongoing Guid 및 DialogueNodeInfo 갱신
	OnCurrentDialogueChanged.ExecuteIfBound(NextRuntimeNode->NodeGuid);

	if (NextRuntimeNode->DialogueNodeType == EDialogueType::EndNode)
	{
		EndDialogue();
		return nullptr;
	}

	return Cast<UDialogueNodeInfo>(NextRuntimeNode->NodeInfo);
}

// 현재 진행 중인 대화문에서 Choices들이 있다면 반환
TArray<FText> UDialogueSubsystem::GetSelectableChoicesText(UDialogueNodeInfo* DialogueNodeInfo) const
{
	if (DialogueNodeInfo == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::GetSelectableChoicesText : DialogueNodeInfo is nullptr"));
		return TArray<FText>();
	}
	return DialogueNodeInfo->DialogueResponses;
}

FPlayerEvalCondition UDialogueSubsystem::GetPlayerEvalCondition() const
{
	FPlayerEvalCondition PlayerEvalCondition;
	PlayerEvalCondition.PlayerLevel = GetPlayerLevel();
	PlayerEvalCondition.PlayerOwnedTags = GetPlayerOwnedTags();

	return PlayerEvalCondition;
}

// 현재 진행 중인 Dialogue Node의 Info를 반환
const UDialogueNodeInfo* UDialogueSubsystem::GetCurrentDialogueNodeInfo() const
{
	return CurrentOngoingDialogueNodeInfo;
}

TArray<FGuid> UDialogueSubsystem::GetSelectableChoicesLinkedGuid(UDialogueRuntimeNode* DialogueRuntimeNode) const
{
	TArray<FGuid> LinkedGuids;
	for (UDialogueRuntimePin* OutputPin : DialogueRuntimeNode->OutputPins)
	{
		if (OutputPin->Connections.Num() == 0)
		{
			LinkedGuids.Add(FGuid());
			continue;
		}

		LinkedGuids.Add(OutputPin->LinkedToNodeGuid);
	}
	
	return LinkedGuids;
}

// Player가 Dialogue를 볼 수 있는 조건인지 점검
bool UDialogueSubsystem::IsPossibleToShowTrueCondition(UDialogueRuntimeNode* BranchNode) const
{
	UDialogueBranchNodeInfoBase* BranchNodeInfoBase = Cast<UDialogueBranchNodeInfoBase>(BranchNode->NodeInfo);
	if (BranchNodeInfoBase == nullptr)
	{
		return false;
	}
	
	return BranchNodeInfoBase->ConditionCheck(GetPlayerEvalCondition());
}

// // 현재 대화 진행 상황 저장
// void UDialogueSubsystem::SaveDialogueProgress(FGuid CurrentDialogueGuid)
// {
// 	if (CurrentDialogueGraph == nullptr)
// 	{
// 		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::SaveDialogueProgress : CurrentDialogueGraph is null"));
// 		return;
// 	}
// 	
// 	if (IsPossibleToLoadDialogueProgressData() == false)
// 	{
// 		CachedDialogueProgressSaveData = Cast<UDialogueProgressSaveData>(UGameplayStatics::CreateSaveGameObject(UDialogueProgressSaveData::StaticClass()));
// 	}
//
// 	CachedDialogueProgressSaveData->AddDialogueProgress(CurrentDialogueGraph, CurrentDialogueGuid);
// 	
// 	UGameplayStatics::SaveGameToSlot(CachedDialogueProgressSaveData, DialogueProgressSaveSlot, SaveIndex);
// }

// Choices가 없는 단일 Response와 연결된 경우
UDialogueRuntimeNode* UDialogueSubsystem::GetNextNode(const int32 SelectedChoiceIndex)
{
	if (IdToNodeMap.Contains(CurrentOngoingNodeGuid))
	{
		UDialogueRuntimeNode* CurrentNode = IdToNodeMap[CurrentOngoingNodeGuid];
		if (CurrentNode == nullptr)
		{
			UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : CurrentNode is null"));
			return nullptr;
		}

		// 현재의 Dialogue Node가 분기 없는 평문인 경우
		if (CurrentNode->DialogueNodeType == EDialogueType::DialogueNode)
		{
			if (CurrentNode->OutputPins.Num() > 0)
			{
				// 입력으로 들어오는 선택지의 index가 OutputPins의 인덱스를 초과하는 경우
				if (SelectedChoiceIndex > CurrentNode->OutputPins.Num() - 1 || SelectedChoiceIndex < 0)
				{
					UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : SelectedChoiceIndex is out of bound"));
					return nullptr;
				}
				
				UDialogueRuntimePin* OutputPin = CurrentNode->OutputPins[SelectedChoiceIndex];
				return GetDialogueNode(OutputPin->LinkedToNodeGuid);
			}

			UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::GetNextNode : No Linked Node"));
			return nullptr;
		}
		
		// 현재의 Dialogue Node가 분기인 경우
		if (CurrentNode->DialogueNodeType == EDialogueType::BranchNode)
		{
			UDialogueBranchNodeInfoBase* BranchNodeInfo = Cast<UDialogueBranchNodeInfoBase>(CurrentNode->NodeInfo);
			if (BranchNodeInfo == nullptr)
			{
				UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : BranchNodeInfo is nullptr"));
				return nullptr;
			}

			bool bIsPossibleToShowTrueNode = IsPossibleToShowTrueCondition(CurrentNode);
			for (UDialogueRuntimePin* OutputPin : CurrentNode->OutputPins)
			{
				// 조건을 충족하는 경우 True Pin에 연결된 노드를 반환
				if (bIsPossibleToShowTrueNode && OutputPin->PinName.IsEqual(FName(TEXT("True"))))
				{
					const FGuid ConnectedNodeGuid = OutputPin->LinkedToNodeGuid;
					return GetDialogueNode(ConnectedNodeGuid);
				}

				// 조건을 충족하지 못하는 경우 False Pin에 연결된 노드를 반환
				if (!bIsPossibleToShowTrueNode && OutputPin->PinName.IsEqual(FName(TEXT("False"))))
				{
					const FGuid ConnectedNodeGuid = OutputPin->LinkedToNodeGuid;
					return GetDialogueNode(ConnectedNodeGuid);
				}
			}
			

			UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : Return Post Branch Node Error"));
			return nullptr;
		}

		if (CurrentNode->DialogueNodeType == EDialogueType::EndNode)
		{
			return CurrentNode;
		}
	}
	
	UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : No Matched Guid"));
	return nullptr;
}

// DialogueGraph 에셋 내부의 NodeInfo를 기반으로 Dialogue를 UI에 출력
void UDialogueSubsystem::CreateDialogueUI()
{
	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::CreateDialogueUI : Enter"));

	if (CurrentDialogueGraph)
	{
		// UI를 생성한다.
		if (DialogueWidget == nullptr)
		{
			const UDialogueSettings* DialogueSettings = GetDefault<UDialogueSettings>();
			DialogueWidgetClass = DialogueSettings->DialogueWidgetClass;

			TSubclassOf<UUserWidget> LoadedDialogueWidgetClass = DialogueWidgetClass.LoadSynchronous();
			APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			DialogueWidget = CreateWidget<UUserWidget>(PlayerController, LoadedDialogueWidgetClass);
			if (DialogueWidget)
			{
				DialogueWidget->AddToViewport();
				SetInputSettings(true);
				return;
			}

			UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::CreateDialogueUI : Create Widget fail"));
			return;
		}

		DialogueWidget->SetVisibility(ESlateVisibility::Visible);
		SetInputSettings(true);
		return;
	}

	UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::CreateDialogueUI : CurrentDialogueGraph is null"));
}

void UDialogueSubsystem::UpdateCurrentDialogueNode(FGuid NewDialogueNodeGuid)
{
	CurrentOngoingNodeGuid = NewDialogueNodeGuid;
	SetCurrentDialogueInfo();
}

void UDialogueSubsystem::SetCurrentDialogueInfo()
{
	CurrentOngoingDialogueNodeInfo = Cast<UDialogueNodeInfo>(IdToNodeMap[CurrentOngoingNodeGuid]->NodeInfo);
}

void UDialogueSubsystem::SetInputSettings(bool bIsShowUI) const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::SetInputSettings : PlayerController is nullptr"));
		return;
	}
	
	PlayerController->SetShowMouseCursor(bIsShowUI);
	if (bIsShowUI)
	{
		FInputModeUIOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		return;
	}

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	
}

// DialogueGraph Asset을 NPCID와 ChapterID를 기준으로 하는 1차 필터 반환
FARFilter UDialogueSubsystem::GetDialogueGraphAssetFilter(ENPCID NPCID, EChapterID ChapterID) const
{
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(UDialogueGraph::StaticClass()->GetClassPathName());

	const UEnum* EnumPtr = StaticEnum<ENPCID>();
	FString NPCEnumName = EnumPtr->GetNameStringByValue(static_cast<uint8>(NPCID));
	ARFilter.TagsAndValues.Add("NPCID", NPCEnumName);
	// 	FString ChapterEnumName = EnumPtr->GetNameStringByValue(static_cast<int64>(ChapterID));
	// 	ARFilter.TagsAndValues.Add("ChapterID", ChapterEnumName);
	
	return ARFilter;
}

bool UDialogueSubsystem::IsPossibleToLoadDialogueProgressData()
{
	if (CachedDialogueProgressSaveData == nullptr)
	{
		CachedDialogueProgressSaveData = Cast<UDialogueProgressSaveData>(UGameplayStatics::LoadGameFromSlot(DialogueProgressSaveSlot, SaveIndex));
		if (CachedDialogueProgressSaveData == nullptr)
		{
			return false;
		}
	}

	return true;
}

void UDialogueSubsystem::InitializeDialogueData()
{
	for (UDialogueRuntimeNode* DialogueRuntimeNode : CurrentDialogueGraph->Graph->Nodes)
	{
		FGuid NodeGuid = DialogueRuntimeNode->NodeGuid;
		IdToNodeMap.Add(NodeGuid, DialogueRuntimeNode);
	}
}

bool UDialogueSubsystem::IsCandidateDialogueGraphAsset(const FAssetData& AssetData) const
{
	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : Enter"));
	
	bool bPossibleCondition = false;
	
	// TODO 갖고있는 Tag는 임시로 HasTag라고 하자 - 추후 PlayerTagSubsystem 등에서 Load하기
	FGameplayTagContainer HasTag;
	HasTag.AddTag(GameplayTags::Chapter_01_Complete);
	HasTag.AddTag(GameplayTags::Chapter_02_Complete);
	HasTag.AddTag(GameplayTags::Item_Weapon_LegendarySword);
	HasTag.AddTag(GameplayTags::Player_Tier_E);
	HasTag.AddTag(GameplayTags::Quest_Chapter_01_VisitHome_Complete);
	
	FString Csv;
	// All Condition인 Tag를 모두 갖고있지 않다면 해당 Dialogue Asset은 후보 제외
	if (AssetData.GetTagValue(*GameplayTags::Required_All_Tags.GetTag().ToString(), Csv))
	{
		UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AllTag Check"));
		
		FGameplayTagContainer RequiredAllTags;
		RequiredAllTags.FromExportString(Csv, PPF_None);
		bPossibleCondition = HasTag.HasAllExact(RequiredAllTags);
		
		if (bPossibleCondition == false)
		{
			UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AllTags false"));
			return false;
		}
	}

	// Block Condition인 Tag를 하나라도 갖고 있다면 해당 Dialogue Asset은 후보 제외
	if (AssetData.GetTagValue(*GameplayTags::Blocked_Any_Tags.GetTag().ToString(), Csv))
	{
		UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : BlockTag Check"));

		FGameplayTagContainer BlockedAllTags;
		BlockedAllTags.FromExportString(Csv, PPF_None);
		bPossibleCondition = !HasTag.HasAnyExact(BlockedAllTags);
		
		// Dialogue 노출을 위해 갖고 있으면 안될 Tag가 하나라도 있는 경우 해당 Dialogue Asset 통과 
		if (bPossibleCondition == false)
		{
			UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : BlockTags false"));
			return false;
		}
	}

	// AnyCondition인 Tag를 하나라도 갖고 있다면 해당 Dialogue Asset은 후보 제외
	if (AssetData.GetTagValue(*GameplayTags::Required_Any_Tags.GetTag().ToString(), Csv))
	{
		UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AnyTag Check"));

		FGameplayTagContainer RequiredAnyTags;
		RequiredAnyTags.FromExportString(Csv, PPF_None);
		bPossibleCondition = HasTag.HasAnyExact(RequiredAnyTags);
		
		// Dialogue 노출을 위해 갖고 있으면 안될 Tag가 하나라도 있는 경우 해당 Dialogue Asset 통과 
		if (bPossibleCondition == false)
		{
			UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AnyTags false"));
			return false;
		}
	}

	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : true"));

	return true;
}

// GameplayTags에 따라 필터링 된 Dialogue Asset Load가 완료된 이후의 콜백 함수
void UDialogueSubsystem::OnDialogueLoaded()
{
	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::OnDialogueLoaded : Enter"));
	
	TArray<UObject*> LoadedObjects;
	if (CurrentHandle.IsValid())
	{
		CurrentHandle->GetLoadedAssets(LoadedObjects);
		CurrentHandle.Reset();
	}
	else
	{
		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::OnDialogueLoaded : Valid Error"));
		return;
	}

	PossibleDialogueGraphs.Reset();
	for (UObject* LoadedObject : LoadedObjects)
	{
		if (UDialogueGraph* DialogueGraph = Cast<UDialogueGraph>(LoadedObject))
		{
			PossibleDialogueGraphs.Add(DialogueGraph);
		}
	}
	
	// Dialogue Show Event 호출
	if (OnDialogueReady.IsBound() && PossibleDialogueGraphs.Num() > 0)
	{
		// 대화의 첫 시작은 최우선순위 DialgoueGraph
		OnDialogueReady.Execute(PossibleDialogueGraphs[0]);
	}
	else
	{
		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::OnDialogueLoaded : Show Dialogue Bind Error"));
		return;
	}
	
	UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::OnDialogueLoaded : PossibleDialogueGraphs Count = %d"), PossibleDialogueGraphs.Num());
}

FGameplayTagContainer UDialogueSubsystem::GetPlayerOwnedTags() const
{
	// TODO PlayerData를 관리하는 Subsystem으로부터 GameplayTagContainer 가져오기
	return FGameplayTagContainer();
}

int32 UDialogueSubsystem::GetPlayerLevel() const
{
	// TODO PlayerData를 관리하는 Subsystem으로부터 Player Level 가져오기
	return 1;
}
