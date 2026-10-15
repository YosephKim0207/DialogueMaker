#include "DialogueGraphSearch.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueChoiceNodeInfo.h"
#include "EdGraph/EdGraph.h"

// 검색은 조건 평가나 대화 실행 없이 저작 텍스트만 조회한다.
bool FDialogueGraphSearch::Search(UEdGraph* Graph, FString Speaker, FString Text, FString Choice, TArray<FDialogueSearchResult>& Results, FText& Error)
{
	Speaker.TrimStartAndEndInline();
	Text.TrimStartAndEndInline();
	Choice.TrimStartAndEndInline();
	if (Graph == nullptr || (Speaker.IsEmpty() && Text.IsEmpty() && Choice.IsEmpty()))
	{
		Error = FText::FromString(TEXT("검색할 내용을 입력하세요."));
		return false;
	}
	if (Choice.IsEmpty() == false && (Speaker.IsEmpty() == false || Text.IsEmpty() == false))
	{
		Error = FText::FromString(TEXT("선택지는 단독 검색만 가능합니다. 화자와 대사 입력을 비워 주세요."));
		return false;
	}
	Results.Reset();
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		auto* Typed = Cast<UDialogueEdGraphNodeBase>(Node);
		if (Typed == nullptr) continue;
		if (Choice.IsEmpty())
		{
			const auto* Line = Cast<UDialogueLineNodeInfo>(Typed->GetNodeInfo());
			if (Line == nullptr) continue;
			const FString Name = StaticEnum<ESpeakerID>()->GetDisplayNameTextByValue(int64(Line->GetSpeakerEmotePair().Speaker)).ToString();
			const FString Body = Line->GetDialogueText().ToString();
			if ((Speaker.IsEmpty() || Name.Contains(Speaker)) && (Text.IsEmpty() || Body.Contains(Text)))
			{
				Results.Add({Node, FGuid(), FText::FromString(FString::Printf(TEXT("Line | %s | %s"), *Name, *Body.Left(120)))});
			}
		}
		else if (const auto* Info = Cast<UDialogueChoiceNodeInfo>(Typed->GetNodeInfo()))
		{
			for (const FDialogueOption& Option : Info->Options)
			{
				if (Option.Text.ToString().Contains(Choice))
				{
					Results.Add({Node, Option.ChoiceId, FText::FromString(TEXT("Choice | ") + Option.Text.ToString().Left(120))});
				}
			}
		}
	}
	Error = Results.IsEmpty() ? FText::FromString(TEXT("검색 결과가 없습니다.")) : FText();
	return true;
}
