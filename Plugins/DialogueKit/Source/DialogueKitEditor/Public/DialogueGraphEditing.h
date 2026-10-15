#pragma once
#include "CoreMinimal.h"
class UEdGraph;
class UEdGraphNode;

// 선택한 노드만 복제하고 선택 집합 내부 연결만 새 ID에 대응시킨다.
class FDialogueGraphEditing
{
public:
	static TArray<UEdGraphNode*> Duplicate(UEdGraph* Graph, const TSet<UObject*>& Selection);
};
