// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueEdGraphNode.h"

void UDialogueEdGraphNode::AllocateDefaultPins()
{
	// 입력핀, 서로 다른 대화로부터 해당 대화로 이어질  수 있음
	CreatePin(EGPD_Input, TEXT("MultipleNodes"), FName(), TEXT("In"));
	// 출력핀, 선택지가 없는 경우 사용
	CreatePin(EGPD_Output, TEXT("SingleNode"), FName(), TEXT("Out"));
	
}

FText UDialogueEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	// 노드 제목을 대사의 첫 부분으로 간략하게 보여줌
	if (Dialogue.DialogueText.IsEmpty())
	{
		return FText::FromString(TEXT("새 대화"));
	}
	return FText::FromString(Dialogue.DialogueText.ToString().Left(20) + TEXT("..."));
}
