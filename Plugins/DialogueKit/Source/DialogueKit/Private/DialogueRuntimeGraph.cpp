#include "DialogueRuntimeGraph.h"

#include "DialogueNodeInfo.h"
#include "DialogueChoiceNodeInfo.h"

// 같은 Graph 안에서 노드/핀/선택 식별자의 유일성과 직렬화 대응을 검사한다.
bool UDialogueRuntimeGraph::ValidateIdentifiers(TArray<FString>& OutIssues) const
{
	OutIssues.Reset();
	TSet<FGuid> NodeIds;
	TSet<FGuid> PinIds;
	TSet<FGuid> ChoiceIds;
	for (const UDialogueRuntimeNode* Node : Nodes)
	{
		if (IsValid(Node) == false)
		{
			OutIssues.Add(TEXT("MissingNode"));
			continue;
		}
		if (Node->NodeGuid.IsValid() == false || NodeIds.Contains(Node->NodeGuid))
		{
			OutIssues.Add(TEXT("InvalidOrDuplicateNodeId"));
		}
		NodeIds.Add(Node->NodeGuid);
		TArray<UDialogueRuntimePin*> Pins = Node->OutputPins;
		if (Node->InputPin != nullptr)
		{
			Pins.Add(Node->InputPin);
		}
		else if (Node->DialogueNodeType != EDialogueType::StartNode)
		{
			OutIssues.Add(TEXT("MissingInputPin"));
		}
		for (const UDialogueRuntimePin* Pin : Pins)
		{
			if (IsValid(Pin) == false)
			{
				OutIssues.Add(TEXT("MissingPin"));
				continue;
			}
			if (Pin->PinId.IsValid() == false || PinIds.Contains(Pin->PinId) || Pin->OwnerNodeGuid != Node->NodeGuid)
			{
				OutIssues.Add(TEXT("InvalidPinIdentityOrOwner"));
			}
			PinIds.Add(Pin->PinId);
		}
		if (const UDialogueNodeInfo* Info = Node->DialogueNodeType == EDialogueType::DialogueNode ? Cast<UDialogueNodeInfo>(Node->NodeInfo) : nullptr)
		{
			if (Info->GetReadVersion() < 1)
			{
				OutIssues.Add(TEXT("InvalidReadVersion"));
			}
			const TArray<FDialogueChoice>& Choices = Info->GetDialogueChoices();
			if (Choices.Num() > 0 && Choices.Num() != Node->OutputPins.Num())
			{
				OutIssues.Add(TEXT("ChoicePinCountMismatch"));
			}
			for (int32 Index = 0; Index < Choices.Num(); ++Index)
			{
				const FGuid Id = Choices[Index].ChoiceId;
				if (Id.IsValid() == false || ChoiceIds.Contains(Id))
				{
					OutIssues.Add(TEXT("InvalidOrDuplicateChoiceId"));
				}
				ChoiceIds.Add(Id);
				if (Node->OutputPins.IsValidIndex(Index) == false || IsValid(Node->OutputPins[Index]) == false || Node->OutputPins[Index]->ChoiceId != Id)
				{
					OutIssues.Add(TEXT("ChoicePinIdentityMismatch"));
				}
			}
		}
		if (Node->DialogueNodeType == EDialogueType::ChoiceNode)
		{
			const UDialogueChoiceNodeInfo* Info = Cast<UDialogueChoiceNodeInfo>(Node->NodeInfo);
			if (Info == nullptr || Info->HasValidOptions() == false || Info->Options.Num() != Node->OutputPins.Num())
			{
				OutIssues.Add(TEXT("InvalidChoiceOptions"));
				continue;
			}
			for (int32 Index = 0; Index < Info->Options.Num(); ++Index)
			{
				const FGuid Id = Info->Options[Index].ChoiceId;
				if (ChoiceIds.Contains(Id) || IsValid(Node->OutputPins[Index]) == false || Node->OutputPins[Index]->ChoiceId != Id)
				{
					OutIssues.Add(TEXT("InvalidChoicePinIdentity"));
				}
				ChoiceIds.Add(Id);
			}
		}
	}
	return OutIssues.IsEmpty();
}
