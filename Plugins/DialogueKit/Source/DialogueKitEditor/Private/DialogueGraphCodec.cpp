#include "DialogueGraphCodec.h"
#include "DialogueGraph.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueBranchNodeInfoBase.h"
#include "DialogueEndNodeInfo.h"
#include "DialogueEdGraphNode.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "EdGraph/EdGraph.h"

// 저장 가능한 노드의 Editor 클래스를 찾는다. 알 수 없는 타입을 조용히 버리지 않는다.
static UClass* GetEditorNodeClass(EDialogueType Type)
{
	switch (Type)
	{
	case EDialogueType::StartNode: return UDialogueEdStartGraphNode::StaticClass();
	case EDialogueType::DialogueNode: return UDialogueEdGraphNode::StaticClass();
	case EDialogueType::LineNode: return UDialogueFlowEdGraphNode::StaticClass();
	case EDialogueType::ChoiceNode: return UDialogueChoiceEdGraphNode::StaticClass();
	case EDialogueType::BranchNode: return UDialogueBranchEdGraphNode::StaticClass();
	case EDialogueType::EndNode: return UDialogueEdEndGraphNode::StaticClass();
	default: return nullptr;
	}
}

// 노드의 선언 타입과 본문/핀 모양이 다르면 저장이나 재열기에서 정보를 버리지 않고 실패한다.
static bool HasValidPayload(EDialogueType Type, UDialogueNodeInfoBase* Info, int32 Inputs, int32 Outputs)
{
	if (Inputs != (Type == EDialogueType::StartNode ? 0 : 1)) return false;
	switch (Type)
	{
	case EDialogueType::StartNode: return Info == nullptr && Outputs == 1;
	case EDialogueType::LineNode:
	{
		const UDialogueLineNodeInfo* Line = Cast<UDialogueLineNodeInfo>(Info);
		return Line && Line->GetReadVersion() > 0 && Line->GetDialogueChoices().IsEmpty() && Outputs == 1;
	}
	case EDialogueType::ChoiceNode:
	{
		const UDialogueChoiceNodeInfo* Choice = Cast<UDialogueChoiceNodeInfo>(Info);
		return Choice && Choice->HasValidOptions() && Outputs == Choice->Options.Num();
	}
	case EDialogueType::DialogueNode: return Cast<UDialogueNodeInfo>(Info) != nullptr && Cast<UDialogueLineNodeInfo>(Info) == nullptr;
	case EDialogueType::BranchNode: return Cast<UDialogueBranchNodeInfoBase>(Info) && Outputs == 2;
	case EDialogueType::EndNode: return Cast<UDialogueEndNodeInfo>(Info) && Outputs == 0;
	default: return false;
	}
}

// 먼저 전체 구조를 검사하고 성공한 새 RuntimeGraph만 에셋에 설치한다.
bool FDialogueGraphCodec::ExportGraph(UEdGraph* EditorGraph, UDialogueGraph* Asset, FString& OutError)
{
	OutError = TEXT("Invalid editor graph");
	if (IsValid(EditorGraph) == false || IsValid(Asset) == false)
	{
		return false;
	}
	TSet<FGuid> NodeIds;
	TSet<FGuid> PinIds;
	for (UEdGraphNode* Node : EditorGraph->Nodes)
	{
		UDialogueEdGraphNodeBase* Typed = Cast<UDialogueEdGraphNodeBase>(Node);
		if (Typed == nullptr || GetEditorNodeClass(Typed->GetDialogueNodeType()) == nullptr ||
			Node->NodeGuid.IsValid() == false || NodeIds.Contains(Node->NodeGuid))
		{
			return false;
		}
		NodeIds.Add(Node->NodeGuid);
		int32 Inputs = 0;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin == nullptr || Pin->PinId.IsValid() == false || PinIds.Contains(Pin->PinId) ||
				(Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 1))
			{
				return false;
			}
			PinIds.Add(Pin->PinId);
			Inputs += Pin->Direction == EGPD_Input ? 1 : 0;
			for (UEdGraphPin* Other : Pin->LinkedTo)
			{
				if (Other == nullptr || Other->Direction == Pin->Direction ||
					EditorGraph->Nodes.Contains(Other->GetOwningNode()) == false ||
					Other->GetOwningNode()->Pins.Contains(Other) == false || Other->LinkedTo.Contains(Pin) == false)
				{
					return false;
				}
			}
		}
		if (HasValidPayload(Typed->GetDialogueNodeType(), Typed->GetNodeInfo(), Inputs, Node->Pins.Num() - Inputs) == false)
		{
			return false;
		}
	}
	UDialogueRuntimeGraph* RuntimeGraph = NewObject<UDialogueRuntimeGraph>(Asset);
	TMap<UEdGraphPin*, UDialogueRuntimePin*> Pins;
	for (UEdGraphNode* Node : EditorGraph->Nodes)
	{
		UDialogueEdGraphNodeBase* Typed = CastChecked<UDialogueEdGraphNodeBase>(Node);
		UDialogueRuntimeNode* RuntimeNode = NewObject<UDialogueRuntimeNode>(RuntimeGraph);
		RuntimeNode->NodeGuid = Node->NodeGuid;
		RuntimeNode->DialogueNodeType = Typed->GetDialogueNodeType();
		RuntimeNode->Position = FVector2D(Node->NodePosX, Node->NodePosY);
		RuntimeNode->NodeInfo = Typed->GetNodeInfo() ? DuplicateObject(Typed->GetNodeInfo(), RuntimeNode) : nullptr;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			UDialogueRuntimePin* RuntimePin = NewObject<UDialogueRuntimePin>(RuntimeNode);
			RuntimePin->PinId = Pin->PinId;
			RuntimePin->PinName = Pin->PinName;
			RuntimePin->ChoiceId = Pin->PersistentGuid;
			RuntimePin->OwnerNodeGuid = Node->NodeGuid;
			Pins.Add(Pin, RuntimePin);
			if (Pin->Direction == EGPD_Input)
			{
				RuntimeNode->InputPin = RuntimePin;
			}
			else
			{
				RuntimeNode->OutputPins.Add(RuntimePin);
			}
		}
		RuntimeGraph->Nodes.Add(RuntimeNode);
	}
	for (const TPair<UEdGraphPin*, UDialogueRuntimePin*>& Pair : Pins)
	{
		if (Pair.Key->Direction == EGPD_Output && Pair.Key->LinkedTo.Num() == 1)
		{
			UDialogueRuntimePin* Target = Pins.FindChecked(Pair.Key->LinkedTo[0]);
			Pair.Value->Connections.Add(Target);
			Pair.Value->LinkedToNodeGuid = Target->OwnerNodeGuid;
		}
	}
	TArray<FString> Issues;
	if (RuntimeGraph->ValidateIdentifiers(Issues) == false)
	{
		OutError = FString::Join(Issues, TEXT(", "));
		return false;
	}
	Asset->Modify();
	Asset->Graph = RuntimeGraph;
	OutError.Reset();
	return true;
}

// 임시 Editor 노드에서 연결을 완성한 뒤 기존 목록을 교체한다.
bool FDialogueGraphCodec::ImportGraph(const UDialogueGraph* Asset, UEdGraph* EditorGraph, FString& OutError)
{
	OutError = TEXT("Invalid runtime graph");
	if (IsValid(Asset) == false || IsValid(EditorGraph) == false)
	{
		return false;
	}
	if (Asset->Graph == nullptr)
	{
		EditorGraph->Nodes.Reset();
		EditorGraph->GetSchema()->CreateDefaultNodesForGraph(*EditorGraph);
		OutError.Reset();
		return true;
	}
	TArray<UEdGraphNode*> NewNodes;
	TMap<const UDialogueRuntimePin*, UEdGraphPin*> Pins;
	TSet<FGuid> NodeIds;
	TSet<FGuid> PinIds;
	for (UDialogueRuntimeNode* RuntimeNode : Asset->Graph->Nodes)
	{
		if (RuntimeNode == nullptr || RuntimeNode->NodeGuid.IsValid() == false || NodeIds.Contains(RuntimeNode->NodeGuid))
		{
			return false;
		}
		UClass* NodeClass = GetEditorNodeClass(RuntimeNode->DialogueNodeType);
		if (NodeClass == nullptr || HasValidPayload(RuntimeNode->DialogueNodeType, RuntimeNode->NodeInfo, RuntimeNode->InputPin ? 1 : 0, RuntimeNode->OutputPins.Num()) == false)
		{
			return false;
		}
		NodeIds.Add(RuntimeNode->NodeGuid);
		UDialogueEdGraphNodeBase* Node = NewObject<UDialogueEdGraphNodeBase>(EditorGraph, NodeClass, NAME_None, RF_Transactional);
		Node->NodeGuid = RuntimeNode->NodeGuid;
		Node->NodePosX = RuntimeNode->Position.X;
		Node->NodePosY = RuntimeNode->Position.Y;
		Node->SetDialogueNodeInfo(RuntimeNode->NodeInfo ? DuplicateObject(RuntimeNode->NodeInfo, Node) : nullptr);
		TArray<UDialogueRuntimePin*> AllPins = RuntimeNode->OutputPins;
		if (RuntimeNode->InputPin)
		{
			AllPins.Insert(RuntimeNode->InputPin, 0);
		}
		for (UDialogueRuntimePin* RuntimePin : AllPins)
		{
			if (RuntimePin == nullptr || RuntimePin->PinId.IsValid() == false || PinIds.Contains(RuntimePin->PinId) || RuntimePin->OwnerNodeGuid != RuntimeNode->NodeGuid)
			{
				return false;
			}
			PinIds.Add(RuntimePin->PinId);
			UEdGraphPin* Pin = Node->CreateCustomPin(RuntimePin == RuntimeNode->InputPin ? EGPD_Input : EGPD_Output, RuntimePin->PinName);
			Pin->PinId = RuntimePin->PinId;
			Pin->PersistentGuid = RuntimePin->ChoiceId;
			Pins.Add(RuntimePin, Pin);
		}
		NewNodes.Add(Node);
	}
	for (const TPair<const UDialogueRuntimePin*, UEdGraphPin*>& Pair : Pins)
	{
		if (Pair.Value->Direction != EGPD_Output)
		{
			continue;
		}
		if (Pair.Key->Connections.Num() > 1 || (Pair.Key->Connections.IsEmpty() && Pair.Key->LinkedToNodeGuid.IsValid()))
		{
			return false;
		}
		for (UDialogueRuntimePin* Target : Pair.Key->Connections)
		{
			UEdGraphPin* const* Input = Pins.Find(Target);
			if (Input == nullptr || (*Input)->Direction != EGPD_Input || Target->OwnerNodeGuid != Pair.Key->LinkedToNodeGuid)
			{
				return false;
			}
			Pair.Value->MakeLinkTo(*Input);
		}
	}
	EditorGraph->Modify();
	EditorGraph->Nodes.Reset();
	for (UEdGraphNode* Node : NewNodes)
	{
		EditorGraph->AddNode(Node, false, false);
	}
	OutError.Reset();
	return true;
}
