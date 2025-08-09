// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueSubsystem.h"

#include "DialogueBranchNodeInfoBase.h"
#include "DialogueNodeInfo.h"
#include "DialogueProgressSaveData.h"
#include "QuestSubsystem.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueDubsSubsystem, Log, All);

// 해당 NPC Actor와 대화 시작
void UDialogueSubsystem::StartDialogue(AActor* NPC, UDialogueGraph* DialogueGraph)
{
	CurrentDialogueGraph = DialogueGraph;
	InitializeDialogueData();

	CurrentOngoingNodeGuid = GetCurrentDialogueGuid();
	UDialogueNodeInfo* DialogueNodeInfo = GetDialogueNodeInfo(CurrentOngoingNodeGuid);
	ShowDialogue(DialogueNodeInfo);
}

// 대화 종료시 호출. 현재 대화 진행 상황을 저장한다.
void UDialogueSubsystem::EndDialogue()
{
	
}

/* 현재 진행 중인 대화 정보를 반환
 * StartDialogue, SaveCurrentDialogue를 위해 호출
*/
UDialogueNodeInfo* UDialogueSubsystem::GetDialogueNodeInfo(FGuid DialogueNodeGuid)
{
	if (DialogueNodeGuid.IsValid() == false)
	{
		UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::GetDialogueNodeInfo : Dialogue Guid is invalid"));
		return GetFirstDialogueInfo();
	}

	UDialogueRuntimeNode* DialogueNode = GetDialogueNode(DialogueNodeGuid);
	
	if (DialogueNode == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetDialogueNodeInfo : Dialogue is not cached"));
		return GetFirstDialogueInfo();
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
UDialogueNodeInfo* UDialogueSubsystem::GetFirstDialogueInfo()
{
	FGuid FirstDialogueNodeGuid;
	for (UDialogueRuntimeNode* DialogueRuntimeNode : CurrentDialogueGraph->Graph->Nodes)
	{
		if (DialogueRuntimeNode->DialogueNodeType == EDialogueType::StartNode)
		{
			UDialogueRuntimePin* OutputPin = DialogueRuntimeNode->OutputPins[0];	// StartNode의 OutputPin은 항시 1개로 가정한다
			if (OutputPin->Connections.Num() == 0)
			{
				UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetFirstDialogue : There is no Connected pin with Start Node"));
				return nullptr;
			}

			UDialogueRuntimePin* LinkedPin = OutputPin->Connections[0];	// 1개의 OutputPin은 항상 1개의 InputPin과 연결됨을 가정한다
			FirstDialogueNodeGuid = LinkedPin->OwnerNodeGuid;
			break;
		}
	}

	if (IdToNodeMap.Contains(FirstDialogueNodeGuid))
	{
		UDialogueRuntimeNode* CachedDialogueRuntimeNode = IdToNodeMap[FirstDialogueNodeGuid];
		return Cast<UDialogueNodeInfo>(CachedDialogueRuntimeNode->NodeInfo);
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

// 현재 진행 중인 대화문에서 Choices들이 있다면 반환
TArray<FText> UDialogueSubsystem::GetSelectableChoicesText(UDialogueNodeInfo* DialogueNodeInfo) const
{
	return DialogueNodeInfo->DialogueResponses;
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

// UI에서 Choices 중 하나 선택시 호출 및 선택지에 맞는 다음 Dialogue로 전환
void UDialogueSubsystem::SelectChoice(FGuid ChoiceDialogueGuid)
{
	
}

// Player가 Dialogue를 볼 수 있는 조건인지 점검
bool UDialogueSubsystem::IsPossibleToShowTrueCondition(UDialogueRuntimeNode* BranchNode)
{
	UDialogueBranchNodeInfoBase* BranchNodeInfoBase = Cast<UDialogueBranchNodeInfoBase>(BranchNode->NodeInfo);
	if (BranchNodeInfoBase == nullptr)
	{
		return false;
	}

	BranchNodeInfoBase->RequiredLevel;
	UQuestBase* Quest = NewObject<UQuestBase>(this, BranchNodeInfoBase->RequiredQuest);
	if (Quest)
	{
		return Quest->IsCleared();
	}

	UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::IsPossibleToShowTrueCondition : Make Quest Instance Error"));
	return false;
}

// 현재 대화 진행 상황 저장
void UDialogueSubsystem::SaveDialogueProgress(FGuid CurrentDialogueGuid)
{
	if (CurrentDialogueGraph == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::SaveDialogueProgress : CurrentDialogueGraph is null"));
		return;
	}
	
	if (IsPossibleToLoadDialogueProgressData() == false)
	{
		CachedDialogueProgressSaveData = Cast<UDialogueProgressSaveData>(UGameplayStatics::CreateSaveGameObject(UDialogueProgressSaveData::StaticClass()));
	}

	CachedDialogueProgressSaveData->AddDialogueProgress(CurrentDialogueGraph, CurrentDialogueGuid);
	
	UGameplayStatics::SaveGameToSlot(CachedDialogueProgressSaveData, DialogueProgressSaveSlot, SaveIndex);
}

// StartDialogue등의 상황에서 현재까지 진행된 Dialogue를 확인하기 위해 사용
FGuid UDialogueSubsystem::GetCurrentDialogueGuid()
{
	if (CurrentDialogueGraph == nullptr)
	{
		UE_LOG(DialogueDubsSubsystem, Error, TEXT("UDialogueSubsystem::GetCurrentDialogueGuid : CurrentDialogueGraph is null"));
		return FGuid();
	}
	
	if (IsPossibleToLoadDialogueProgressData())
	{
		return CachedDialogueProgressSaveData->GetDialogueProgress(CurrentDialogueGraph);
	}
	
	return FGuid();
}

// Choices가 없는 단일 Response와 연결된 경우
UDialogueRuntimeNode* UDialogueSubsystem::GetNextNode()
{
	if (IdToNodeMap.Contains(CurrentOngoingNodeGuid))
	{
		UDialogueRuntimeNode* CurrentNode = IdToNodeMap[CurrentOngoingNodeGuid];
		if (CurrentNode == nullptr)
		{
			UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : CurrentNode is null"));
			return nullptr;
		}

		if (CurrentNode->DialogueNodeType == EDialogueType::DialogueNode)
		{
			if (CurrentNode->OutputPins.Num() > 0)
			{
				UDialogueRuntimePin* OutputPin = CurrentNode->OutputPins[0];
				const FGuid ConnectedNodeGuid = OutputPin->LinkedToNodeGuid;
				return GetDialogueNode(ConnectedNodeGuid);
			}

			UE_LOG(DialogueDubsSubsystem, Display, TEXT("UDialogueSubsystem::GetNextNode : No Linked Node"));
			return nullptr;
		}

		if (CurrentNode->DialogueNodeType == EDialogueType::BranchNode)
		{
			UDialogueBranchNodeInfoBase* BranchNodeInfo = Cast<UDialogueBranchNodeInfoBase>(CurrentNode->NodeInfo);
			const int32 RequiredLevel = BranchNodeInfo->RequiredLevel;
			const FName QuestID = BranchNodeInfo->RequiredQuest.GetDefaultObject()->GetQuestID();
			if (UWorld* World = GetWorld())
			{
				if (UQuestSubsystem* QuestSubsystem = World->GetGameInstance()->GetSubsystem<UQuestSubsystem>())
				{
					// TODO RequiredLevel 이용하기
					bool bIsPossibleToShowTrueNode = QuestSubsystem->IsClearedQuest(QuestID);
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
				}
			}

			UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : Return Post Branch Node Error"));
			return nullptr;
		}

		if (CurrentNode->DialogueNodeType == EDialogueType::EndNode)
		{
			// TODO 여기서 EndDialogue를 호출하나? 아니면 GetNextNode의 returnValue가 null인 경우 대화 종료?
			// TODO 이떄 발생할 수 있는 문제점은?
			return nullptr;
		}
	}
	
	UE_LOG(DialogueDubsSubsystem, Warning, TEXT("UDialogueSubsystem::GetNextNode : No Matched Guid"));
	return nullptr;
}

void UDialogueSubsystem::ShowDialogue(UDialogueNodeInfo* Dialogue)
{
	if (Dialogue == nullptr)
	{
		// TODO UI에 빈 텍스트 노출시키기
	}

	// TODO UI에 DialogueNodeInfo 기반 텍스트들 노출시키기
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
