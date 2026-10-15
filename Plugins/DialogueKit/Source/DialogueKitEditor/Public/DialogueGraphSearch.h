#pragma once
#include "CoreMinimal.h"
class UEdGraph;
class UEdGraphNode;
struct FDialogueSearchResult
{
	TWeakObjectPtr<UEdGraphNode> Node;
	FGuid ChoiceId;
	FText Label;
};
class FDialogueGraphSearch
{
public:
	// 빈 검색/선택지와 발화 혼합은 실패한다. 미저장 작업 그래프만 조회한다.
	static bool Search(UEdGraph* Graph, FString Speaker, FString Text, FString Choice, TArray<FDialogueSearchResult>& Results, FText& Error);
};
