// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueSubsystem.h"

#include "DialogueStructure.h"

// 해당 NPC Actor와 대화 시작
void UDialogueSubsystem::StartDialogue(AActor* NPC, UDataTable* DialogueDataTable)
{
	// NPC와 대화 진행 상황 체크
	// 첫 대화인 경우 DataTable의 첫부분 출력
}

// 대화 종료시 호출. 현재 대화 진행 상황을 저장한다.
void UDialogueSubsystem::EndDialogue()
{
	
}

/* 현재 진행 중인 대화 정보를 반환
 * StartDialogue, SaveCurrentDialogue를 위해 호출
*/
FDialogueStructure UDialogueSubsystem::GetCurrentDialogue()
{
	return FDialogueStructure();
}

// 현재 진행 중인 대화문에서 Choices가 있는지 여부 반환
bool UDialogueSubsystem::HasChoicesInCurrentDialogue()
{
	return false;
}

// 현재 진행 중인 대화문에서 Choices들이 있다면 반환
TArray<FDialogueChoice> UDialogueSubsystem::GetSelectableChoices()
{
	return TArray<FDialogueChoice>();
}

// UI에서 Choices 중 하나 선택시 호출 및 선택지에 맞는 다음 Dialogue로 전환
void UDialogueSubsystem::SelectChoice(FGuid ChoiceDialogueGuid)
{
	
}

// Player가 Dialogue를 볼 수 있는 조건인지 점검
bool UDialogueSubsystem::IsDialogueShowPossibleCondition()
{
	return false;
}

// 현재 대화 진행 상황 저장
void UDialogueSubsystem::SaveDialogueProgress(UDataTable* DialogueDataTable, FGuid CurrentDialogueGuid)
{
	
}

// StartDialogue등의 상황에서 현재까지 진행된 Dialogue를 확인하기 위해 사용
FGuid UDialogueSubsystem::GetCurrentDialogueGuid(UDataTable* DialogueDataTable)
{
	return FGuid();
}
