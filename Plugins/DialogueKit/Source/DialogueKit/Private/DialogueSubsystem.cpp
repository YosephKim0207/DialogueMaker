// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueSubsystem.h"
#include "DialogueBranchNodeInfoBase.h"
#include "DialogueEndNodeInfo.h"
#include "ShownDialogueSaveData.h"
#include "DialogueNodeInfo.h"
#include "DialogueSettings.h"
#include "GameplayTags.h"
#include "PlayerProgressSubsystem.h"
#include "QuestSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "Struct/DialogueStructure.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueSubSystemLog, Log, All);

UDialogueSubsystem* UDialogueSubsystem::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World == nullptr)
	{
		UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::Get : World is nullptr"));

		return nullptr;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		return GameInstance->GetSubsystem<UDialogueSubsystem>();
	}
				
	UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::Get : GameInstance is nullptr"));

	return nullptr;
}

void UDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (LoadDialogueSaveData() == false)
	{
		DialogueHistorySaveData = Cast<UShownDialogueSaveData>(UGameplayStatics::CreateSaveGameObject(UShownDialogueSaveData::StaticClass()));
		SaveDialogueSaveData();
	}
}

// NPC의 호출로 현재 노출 가능한 Dialogue를 Show하기 위한 함수
void UDialogueSubsystem::BeginDialogue(ESpeakerID SpeakerID)
{
	CheckDelegates();	
	GetDialogueGraph(SpeakerID);
}

// Player가 갖고 있는 GameplayTagContainer을 통해 현재의 Condition을 점검하여 현 상황에서 노출 가능한 DialogueGraph 에셋들을 추출한다. 
void UDialogueSubsystem::GetDialogueGraph(ESpeakerID SpeakerID)
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::GetDialogueGraph : Enter"));
	
	// SpeakerID, ChapterID 기준 필터 제작
	
	EChapterID TEMPChapterID = GetCurrentChapter();

	// TODO 테스트용 코드
#if UE_BUILD_DEVELOPMENT
	TEMPChapterID = EChapterID::Chapter01;
#endif
	
	FARFilter ARFilter = GetDialogueGraphAssetFilter(SpeakerID, TEMPChapterID);

	// 제작된 필터로부터 Assets 가져오기
	TArray<FAssetData> AssetDatas;
	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	AssetRegistry.GetAssets(ARFilter, AssetDatas);
	
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
			
			UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::GetDialogueGraph : Add Candidate %s"), *AssetId.ToString());
		}
		else
		{
			UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::GetDialogueGraph : Valid Error %s"), *AssetData.GetSoftObjectPath().ToString());
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
	PreloadPortraits();
}

// Dialogue Node의 Type이 End에 도착하는 경우 호출
void UDialogueSubsystem::EndDialogue()
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::EndDialogue : Enter"));
	
	SetInputSettings(false);

	if (GetWorld()->GetTimerManager().IsTimerActive(OnShownDialogueSkipTimerHandle)
		&& OnStopSkip.IsBound())
	{
		OnStopSkip.Broadcast();
	}

	// Load해둔 Portrait 해제
	PortraitPreLoadHandle->ReleaseHandle();

	// TODO 진행 상황 저장
	UDialogueRuntimeNode* DialogueRuntimeNode = GetDialogueNode(CurrentOngoingNodeGuid);
	if (DialogueRuntimeNode == nullptr)
	{
		UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::EndDialogue : DialogueRuntimeNode is nullptr"));
	}
	else
	{
		UDialogueEndNodeInfo* DialogueEndNodeInfo = Cast<UDialogueEndNodeInfo>(DialogueRuntimeNode->NodeInfo);
		if (DialogueEndNodeInfo == nullptr)
		{
			UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::EndDialogue : DialogueEndNodeInfo is nullptr"));
		}
		else
		{
			EDialogueNodeAction EndNodeAction = DialogueEndNodeInfo->Action;
			// 평문 End Node
			if (EndNodeAction == EDialogueNodeAction::None)
			{
				// 진행도 Tag가 있다면 추가
				if (DialogueEndNodeInfo->ClearTag.IsValid())
				{
					if (UPlayerProgressSubsystem* PlayerProgressSubsystem = UPlayerProgressSubsystem::Get(this))
					{
						PlayerProgressSubsystem->AddTag(DialogueEndNodeInfo->ClearTag);
					}
				}
			}
			// Quest 관련 End Node
			else
			{
				UQuestSubsystem* QuestSubsystem = UQuestSubsystem::Get(this);
				if (QuestSubsystem == nullptr)
				{
					UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::EndDialogue : QuestSubsystem is nullptr"));

					return;
				}

				// TODO 각 상황에 맞는 Quest 관련 후속 조치 실행
				if (EndNodeAction == EDialogueNodeAction::StartQuest)
				{
					
				}
				else if (EndNodeAction == EDialogueNodeAction::AdvanceQuest)
				{
					
				}
				else if (EndNodeAction == EDialogueNodeAction::EndQuest)
				{
					
				}
			}
		}
	}
	
	// Storage에 데이터들 Save
	SaveRelativeDatas();
	
	// UI에서 Bind해둔 델리게이트 Execute(), UI에서는 위젯의 Visibility만 collaps로 조절
	if (OnDialogueEnded.IsBound())
	{
		OnDialogueEnded.Broadcast();

		UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::EndDialogue : Broadcast"));
	}
}

// Guid로부터 캐싱된 DialogueRuntimeNode를 반환
UDialogueRuntimeNode* UDialogueSubsystem::GetDialogueNode(FGuid DialogueNodeGuid)
{
	if (IdToNodeMap.Contains(DialogueNodeGuid))
	{
		return IdToNodeMap[DialogueNodeGuid];
	}

	UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetDialogueNode : Dialogue is not cached"));
	
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
				UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetFirstDialogue : There is no Connected pin with Start Node"));
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

	UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetFirstDialogue : There is no First Dialogue Node"));
	return nullptr;
}

// 현재 진행 중인 대화문에서 Choices가 있는지 여부 반환
bool UDialogueSubsystem::HasChoicesInCurrentDialogue(UDialogueNodeInfo* DialogueNodeInfo) const
{
	if (DialogueNodeInfo == nullptr)
	{
		UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::HasChoicesInCurrentDialogue : DialogueNodeInfo is nullptr"));
		return false;
	}

	if (DialogueNodeInfo->GetDialogueChoices().Num() > 1)
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
		UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::ProgressNextDialogue : NextRuntimeNode is nullptr"));

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

	if (CurrentOngoingDialogueNodeInfo->IsDialogueAlreadyShown() == false
		&& GetWorld()->GetTimerManager().IsTimerActive(OnShownDialogueSkipTimerHandle)
		&& OnStopSkip.IsBound())
	{
		OnStopSkip.Broadcast();
	}

	return Cast<UDialogueNodeInfo>(NextRuntimeNode->NodeInfo);
}

// 현재 진행 중인 대화문에서 Choices들이 있다면 반환
void UDialogueSubsystem::GetSelectableChoiceTexts(UDialogueNodeInfo* DialogueNodeInfo, TArray<FText>& OutSelectableChoiceTexts, TArray<int32>& OutSelectableChoiceOriginalIndex) const
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::GetSelectableChoicesText : %s Check Enter"), *DialogueNodeInfo->GetPathName());
	
	if (DialogueNodeInfo == nullptr)
	{
		UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::GetSelectableChoicesText : DialogueNodeInfo is nullptr"));
	}

	int32 SelectableChoiceOriginalIndex = 0;
	FPlayerCondition PlayerConditionEval = GetPlayerEvalCondition();
	for (const FDialogueChoice DialogueConditionEvalCriteria : DialogueNodeInfo->GetDialogueChoices())
	{
		if (DialogueConditionEvalCriteria.IsPossibleToShow(PlayerConditionEval))
		{
			OutSelectableChoiceTexts.Add(DialogueConditionEvalCriteria.ResponseText);
			OutSelectableChoiceOriginalIndex.Add(SelectableChoiceOriginalIndex);
		}

		SelectableChoiceOriginalIndex++;
	}
}

bool UDialogueSubsystem::IsAlreadyShownDialogue(UDialogueNodeInfo* DialogueNodeInfo) const
{
	return DialogueNodeInfo->IsDialogueAlreadyShown();
}

FTimerHandle& UDialogueSubsystem::GetSkipHandler()
{
	return OnShownDialogueSkipTimerHandle;
}

void UDialogueSubsystem::SetSkipHandler(const FTimerHandle& Handle)
{
	OnShownDialogueSkipTimerHandle = Handle;
}

const TArray<UDialogueNodeInfo*>& UDialogueSubsystem::GetDialogueHistory()
{
	return DialogueHistory;
}

void UDialogueSubsystem::SetDialogueRecallWidget(UUserWidget* UserWidget)
{
	RecallWidget = UserWidget;
}

UUserWidget* UDialogueSubsystem::GetDialogueRecallWidget() const
{
		return RecallWidget;
}

FPlayerCondition UDialogueSubsystem::GetPlayerEvalCondition() const
{
	FPlayerCondition PlayerEvalCondition;
	PlayerEvalCondition.PlayerLevel = GetPlayerLevel();
	PlayerEvalCondition.PlayerOwnedTags = GetPlayerOwnedTags();

	return PlayerEvalCondition;
}

const TArray<FPortraitInitData> UDialogueSubsystem::GetInitPortraitDatas() const
{
	if (CurrentDialogueGraph == nullptr)
	{
		UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetHighlightedInitPortraitDatas : CurrentDialogueGraph is null"));
		return {};
	}
	
	return CurrentDialogueGraph->InitPortraits;
}

const TArray<FPortraitActionData> UDialogueSubsystem::GetPortraitActionDatas() const
{
	if (CurrentOngoingDialogueNodeInfo == nullptr)
	{
		return {};
	}

	return CurrentOngoingDialogueNodeInfo->GetPortraitActionDatas();
}

#if !UE_BUILD_SHIPPING
void UDialogueSubsystem::PlayDialogueGraph(UDialogueGraph* DialogueGraph)
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::PlayDialogueGraph : Play %s"), *DialogueGraph->GetName());
	
	CheckDelegates();

	StartDialogue(DialogueGraph);
}
#endif

void UDialogueSubsystem::CheckDelegates()
{
	if (OnCurrentDialogueChanged.IsBound() == false)
	{
		OnCurrentDialogueChanged.BindUObject(this, &UDialogueSubsystem::UpdateCurrentDialogueNode);
	}

	if (OnDialogueReady.IsBound() == false)
	{
		OnDialogueReady.BindUObject(this, &UDialogueSubsystem::StartDialogue);
	}
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

// Choices가 없는 단일 Response와 연결된 경우
UDialogueRuntimeNode* UDialogueSubsystem::GetNextNode(const int32 SelectedChoiceIndex)
{
	if (IdToNodeMap.Contains(CurrentOngoingNodeGuid))
	{
		UDialogueRuntimeNode* CurrentNode = IdToNodeMap[CurrentOngoingNodeGuid];
		if (CurrentNode == nullptr)
		{
			UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetNextNode : CurrentNode is null"));
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
					UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetNextNode : SelectedChoiceIndex is out of bound"));
					return nullptr;
				}

				// 선택지가 존재하는 경우 플레이어의 대사 선택을 위해 skip을 중단
				if (CurrentNode->OutputPins.Num() > 1
					&& GetWorld()->GetTimerManager().IsTimerActive(OnShownDialogueSkipTimerHandle)
					&& OnStopSkip.IsBound())
				{
					OnStopSkip.Broadcast();
					// TODO CurrentNode 반환시 UI가 깜빡이는 경우 nullptr 반환 및 nullptr 반환시 EndDialogue가 아니라 Dialogue 중단되도록 할 필요
					return CurrentNode;
				}
				
				UDialogueRuntimePin* OutputPin = CurrentNode->OutputPins[SelectedChoiceIndex];
				return GetDialogueNode(OutputPin->LinkedToNodeGuid);
			}

			UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::GetNextNode : No Linked Node"));
			return nullptr;
		}
		
		// 현재의 Dialogue Node가 분기인 경우
		if (CurrentNode->DialogueNodeType == EDialogueType::BranchNode)
		{
			if (GetWorld()->GetTimerManager().IsTimerActive(OnShownDialogueSkipTimerHandle)
				&& OnStopSkip.IsBound())
			{
				OnStopSkip.Broadcast();
			}

			
			UDialogueBranchNodeInfoBase* BranchNodeInfo = Cast<UDialogueBranchNodeInfoBase>(CurrentNode->NodeInfo);
			if (BranchNodeInfo == nullptr)
			{
				UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetNextNode : BranchNodeInfo is nullptr"));
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
			

			UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetNextNode : Return Post Branch Node Error"));
			return nullptr;
		}

		if (CurrentNode->DialogueNodeType == EDialogueType::EndNode)
		{
			return CurrentNode;
		}
	}
	
	UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::GetNextNode : No Matched Guid"));
	return nullptr;
}

// DialogueGraph 에셋 내부의 NodeInfo를 기반으로 Dialogue를 UI에 출력
void UDialogueSubsystem::CreateDialogueUI()
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::CreateDialogueUI : Enter"));

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

			UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::CreateDialogueUI : Create Widget fail"));
			return;
		}

		DialogueWidget->SetVisibility(ESlateVisibility::Visible);
		SetInputSettings(true);
		return;
	}

	UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::CreateDialogueUI : CurrentDialogueGraph is null"));
}

void UDialogueSubsystem::UpdateCurrentDialogueNode(FGuid NewDialogueNodeGuid)
{

	CurrentOngoingNodeGuid = NewDialogueNodeGuid;
	SetCurrentDialogueInfo();
}

// 노드가 노출된 적이 있음을 기록
void UDialogueSubsystem::MakeCurrentDialogueNodeToShown()
{
	UDialogueNodeInfo* CurrentNodeInfo = CurrentOngoingDialogueNodeInfo;
	if (CurrentNodeInfo == nullptr)
	{
		return;
	}
	
	DialogueHistory.Add(CurrentNodeInfo);	// 현재 노출 중인 DialogueGraph에서 진행된 대사 Recall용
	CurrentNodeInfo->SetShownCondition(true);
	
	if (ShownDialogueGuids.Contains(CurrentOngoingNodeGuid) == false)
	{
		ShownDialogueGuids.Add(CurrentOngoingNodeGuid);	// 다회차 시도시 이미 본 대사 Skip용
	}
}

void UDialogueSubsystem::SetCurrentDialogueInfo()
{
	CurrentOngoingDialogueNodeInfo = Cast<UDialogueNodeInfo>(IdToNodeMap[CurrentOngoingNodeGuid]->NodeInfo);

	if (OnDialogueNodeInfoChanged.IsBound())
	{
		OnDialogueNodeInfoChanged.Broadcast();
	}
}

void UDialogueSubsystem::SetInputSettings(bool bIsShowUI) const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController == nullptr)
	{
		UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::SetInputSettings : PlayerController is nullptr"));
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

// DialogueGraph Asset을 SpeakerID ChapterID를 기준으로 하는 1차 필터 반환
FARFilter UDialogueSubsystem::GetDialogueGraphAssetFilter(ESpeakerID SpeakerID, EChapterID ChapterID) const
{
	FARFilter ARFilter;
	ARFilter.ClassPaths.Add(UDialogueGraph::StaticClass()->GetClassPathName());

	FString SpeakerEnumName = GetEnumNameString<ESpeakerID>(SpeakerID);
	ARFilter.TagsAndValues.Add(UDialogueGraph::GetSpeakerIDTag(), SpeakerEnumName);
	// 	FString ChapterEnumName = EGetEnumNameString<EChapterID>(ChapterID);
	// 	ARFilter.TagsAndValues.Add(UDialogueGraph::GetChapterIDTag(), ChapterEnumName);

	return ARFilter;
}

void UDialogueSubsystem::InitializeDialogueData()
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::InitializeDialogueData : Enter"));
	
	DialogueHistory.Reset();

	// 반복회차 진행시 skip 및 Text 색 변경을 위함
	ShownDialogueGuids.Reset();
	DialogueHistorySaveData->GetShownDialogues(CurrentDialogueGraph->GetPrimaryAssetId(), ShownDialogueGuids);
	
	for (UDialogueRuntimeNode* DialogueRuntimeNode : CurrentDialogueGraph->Graph->Nodes)
	{
		FGuid NodeGuid = DialogueRuntimeNode->NodeGuid;
		IdToNodeMap.Add(NodeGuid, DialogueRuntimeNode);

		if (ShownDialogueGuids.Contains(NodeGuid))
		{
			UDialogueNodeInfo* NodeInfo = Cast<UDialogueNodeInfo>(DialogueRuntimeNode->NodeInfo);
			NodeInfo->SetShownCondition(true);
		}
	}
}

bool UDialogueSubsystem::IsCandidateDialogueGraphAsset(const FAssetData& AssetData) const
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : Enter"));
	
	bool bPossibleCondition = false;	
	FGameplayTagContainer PlayersTagContainer = GetPlayerOwnedTags();

#if UE_BUILD_DEVELOPMENT
	// TODO 테스트를 위해 임시로 Tag 추가
	PlayersTagContainer.AddTag(GameplayTags::Chapter_01_Complete);
	PlayersTagContainer.AddTag(GameplayTags::Chapter_02_Complete);
	PlayersTagContainer.AddTag(GameplayTags::Item_Weapon_LegendarySword);
	PlayersTagContainer.AddTag(GameplayTags::Player_Tier_E);
	PlayersTagContainer.AddTag(GameplayTags::Quest_Chapter_01_VisitHome_Complete);
#endif
	
	FString Csv;
	// All Condition인 Tag를 모두 갖고있지 않다면 해당 Dialogue Asset은 후보 제외
	if (AssetData.GetTagValue(*GameplayTags::Required_All_Tags.GetTag().ToString(), Csv))
	{
		UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AllTag Check"));
		
		FGameplayTagContainer RequiredAllTags;
		RequiredAllTags.FromExportString(Csv, PPF_None);
		bPossibleCondition = PlayersTagContainer.HasAllExact(RequiredAllTags);
		
		if (bPossibleCondition == false)
		{
			UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AllTags false"));
			return false;
		}
	}

	// Block Condition인 Tag를 하나라도 갖고 있다면 해당 Dialogue Asset은 후보 제외
	if (AssetData.GetTagValue(*GameplayTags::Blocked_Any_Tags.GetTag().ToString(), Csv))
	{
		UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : BlockTag Check"));

		FGameplayTagContainer BlockedAllTags;
		BlockedAllTags.FromExportString(Csv, PPF_None);
		bPossibleCondition = !PlayersTagContainer.HasAnyExact(BlockedAllTags);
		
		// Dialogue 노출을 위해 갖고 있으면 안될 Tag가 하나라도 있는 경우 해당 Dialogue Asset 통과 
		if (bPossibleCondition == false)
		{
			UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : BlockTags false"));
			return false;
		}
	}

	// AnyCondition인 Tag를 하나라도 갖고 있다면 해당 Dialogue Asset은 후보 제외
	if (AssetData.GetTagValue(*GameplayTags::Required_Any_Tags.GetTag().ToString(), Csv))
	{
		UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AnyTag Check"));

		FGameplayTagContainer RequiredAnyTags;
		RequiredAnyTags.FromExportString(Csv, PPF_None);
		bPossibleCondition = PlayersTagContainer.HasAnyExact(RequiredAnyTags);
		
		// Dialogue 노출을 위해 갖고 있으면 안될 Tag가 하나라도 있는 경우 해당 Dialogue Asset 통과 
		if (bPossibleCondition == false)
		{
			UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : AnyTags false"));
			return false;
		}
	}

	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::IsCandidateDialogueGraphAsset : true"));

	return true;
}

// GameplayTags에 따라 필터링 된 Dialogue Asset Load가 완료된 이후의 콜백 함수
void UDialogueSubsystem::OnDialogueLoaded()
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::OnDialogueLoaded : Enter"));
	
	TArray<UObject*> LoadedObjects;
	if (CurrentHandle.IsValid())
	{
		CurrentHandle->GetLoadedAssets(LoadedObjects);
		CurrentHandle.Reset();
	}
	else
	{
		UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::OnDialogueLoaded : Valid Error"));
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
		UE_LOG(DialogueSubSystemLog, Error, TEXT("UDialogueSubsystem::OnDialogueLoaded : Show Dialogue Bind Error"));
		return;
	}
	
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::OnDialogueLoaded : PossibleDialogueGraphs Count = %d"), PossibleDialogueGraphs.Num());
}

FGameplayTagContainer UDialogueSubsystem::GetPlayerOwnedTags() const
{
	UPlayerProgressSubsystem* PlayerProgressSubsystem = UPlayerProgressSubsystem::Get(this);
	check(PlayerProgressSubsystem);
	
	return PlayerProgressSubsystem->GetAllTags();
}

int32 UDialogueSubsystem::GetPlayerLevel() const
{
	UPlayerProgressSubsystem* PlayerProgressSubsystem = UPlayerProgressSubsystem::Get(this);
	check(PlayerProgressSubsystem);

	return PlayerProgressSubsystem->GetPlayerLevel();
}

EChapterID UDialogueSubsystem::GetCurrentChapter() const
{
	UPlayerProgressSubsystem* PlayerProgressSubsystem = UPlayerProgressSubsystem::Get(this);
	check(PlayerProgressSubsystem);

	return PlayerProgressSubsystem->GetCurrentChapter();
}

// DialogueGraph 에셋 내 사용되는 Portrait들을 메모리에 사전 탑재
void UDialogueSubsystem::PreloadPortraits()
{
	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::PreloadPortraits : Enter"));
	
	// Current DialogueGraph를 순회하며 필요한 Portrait 리소스들 Load
	TMultiMap<ESpeakerID, EEmoteType> RequiredPortraitPairs;
	for (const UDialogueRuntimeNode* Node : CurrentDialogueGraph->Graph->Nodes)
	{
		if (const UDialogueNodeInfo* NodeInfo = Cast<UDialogueNodeInfo>(Node->NodeInfo))
		{
			const FSpeakerEmotePair SpeakerEmotePair = NodeInfo->GetSpeakerEmotePair();
			RequiredPortraitPairs.AddUnique(SpeakerEmotePair.Speaker, SpeakerEmotePair.EmoteType);
		}
	}

	UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::PreloadPortraits : Pass Cache RequiredPortraitPairs"));

	// ESpeakerID, EEmoteType가 일치하는 Portrait를 가져오기 위해  DialoguePortraitData로부터 PrimaryAssetId 얻기
	TArray<FPrimaryAssetId> AssetIds;
	for (TTuple<ESpeakerID, EEmoteType> PortraitPair : RequiredPortraitPairs)
	{
		FARFilter ARFilter;
		ARFilter.ClassPaths.Add(UDialoguePortraitData::StaticClass()->GetClassPathName());
		
		FString NPCEnumName = GetEnumNameString<ESpeakerID>(PortraitPair.Key);
		ARFilter.TagsAndValues.Add(UDialoguePortraitData::GetSpeakerIDTag(), NPCEnumName);	// ARFilter는 동일 component 내 or 판정이므로 하나의 조건만 add

		TArray<FAssetData> AssetDatas;
		IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
		AssetRegistry.WaitForCompletion();
		AssetRegistry.GetAssets(ARFilter, AssetDatas);

		UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::PreloadPortraits : Get %s's Portraits"), *NPCEnumName);

		// Major한 조건인 ESpeakerID 1차 필터링 후 EEmoteType이 일치하는 Asset 추출
		for (FAssetData AssetData : AssetDatas)
		{
			FString AssetEmoteString;
			if (AssetData.GetTagValue(UDialoguePortraitData::GetEmoteTypeTag(), AssetEmoteString))
			{
				const UEnum* Enum = StaticEnum<EEmoteType>();
				if (Enum == nullptr)
				{
					UE_LOG(DialogueSubSystemLog, Warning, TEXT(" : EEmoteType's staticEnum is nullptr"));
					break;
				}

				// EEmotyType이 일치하는 Asset만 PrimaryAssetId를 추출
				const uint8 AssetEmoteValue = Enum->GetValueByNameString(AssetEmoteString);
				if (AssetEmoteValue == static_cast<uint8>(PortraitPair.Value))
				{
					const FPrimaryAssetId AssetId = AssetData.GetPrimaryAssetId();
					if (AssetId.IsValid())
					{
						CachedPortraitEmotePairMap.Add(PortraitPair.Key, FPortraitEmoteIDPair(PortraitPair.Value, AssetId));
						AssetIds.Add(AssetId);

						UE_LOG(DialogueSubSystemLog, Display, TEXT("UDialogueSubsystem::PreloadPortraits : Emote %s"), *AssetEmoteString);
					}
					else
					{
						UE_LOG(DialogueSubSystemLog, Warning, TEXT("UDialogueSubsystem::PreloadPortraits : AssetId is not Valid"));
					}
				}
			}
		}
	}

	// 필터와 일치하는 DialoguePortraitData에서 중 Portrait bundle만 가져오기(텍스처 로드)
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FName> Bundles;
	Bundles.Add(TEXT("Portrait"));

	// TODO 콜백함수로 로딩 종료 띄우기
	
	FStreamableDelegate OnPortraitLoaded = FStreamableDelegate::CreateUObject(this, &UDialogueSubsystem::CreateDialogueUI);
	PortraitPreLoadHandle = AssetManager.PreloadPrimaryAssets(AssetIds, Bundles, false, OnPortraitLoaded);
}

UTexture2D* UDialogueSubsystem::GetPortrait(const ESpeakerID SpeakerID, const EEmoteType EmoteType) const
{
	const FPortraitEmoteIDPair* EmotePrimaryAssetIdPair = CachedPortraitEmotePairMap.Find(SpeakerID);
	if (EmotePrimaryAssetIdPair)
	{
		UAssetManager& AssetManager = UAssetManager::Get();
		if (UDialoguePortraitData* PortraitData = AssetManager.GetPrimaryAssetObject<UDialoguePortraitData>(EmotePrimaryAssetIdPair->PortraitAssetId))
		{
			return PortraitData->GetPortraitTexture();
		}
		
	}

	return nullptr;
}

const FPortraitData UDialogueSubsystem::GetPortraitData() const
{
	return CurrentOngoingDialogueNodeInfo->GetPortraitData();
}

void UDialogueSubsystem::SaveRelativeDatas() const
{
	// TODO Save 진행시 프로그레스 UI 띄우도록 하기
	// TODO 전체 Save 관련 GameInstance 별도로 빼기
	SaveDialogueSaveData();
	
	UPlayerProgressSubsystem* PlayerProgressSubsystem = UPlayerProgressSubsystem::Get(this);
	check(PlayerProgressSubsystem);
	PlayerProgressSubsystem->SaveProgress();
	UQuestSubsystem* QuestSubsystem = UQuestSubsystem::Get(this);
	check(QuestSubsystem);
	QuestSubsystem->SaveProgress();
}

bool UDialogueSubsystem::LoadDialogueSaveData()
{
	DialogueHistorySaveData = Cast<UShownDialogueSaveData>(
		UGameplayStatics::LoadGameFromSlot(ShownDialogueSaveSlot, DialogueHistorySaveIndex));
	if (DialogueHistorySaveData)
	{
		return true;
	}

	return false;
}

void UDialogueSubsystem::SaveDialogueSaveData() const
{
	// 진행 동안 노출된 Dialogue의 Guid들을 SaveData에 저장
	if (CurrentDialogueGraph && DialogueHistorySaveData)
	{
		DialogueHistorySaveData->SetShownDialogues(CurrentDialogueGraph->GetPrimaryAssetId(), ShownDialogueGuids);
	}

	UGameplayStatics::SaveGameToSlot(DialogueHistorySaveData, ShownDialogueSaveSlot, DialogueHistorySaveIndex);
}
