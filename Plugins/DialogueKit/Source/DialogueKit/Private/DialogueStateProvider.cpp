#include "DialogueStateProvider.h"

// 게임이 목록 함수를 구현하지 않았으면 실패와 빈 목록을 반환한다.
bool IDialogueDefinitionProvider::GetDialogueDefinitions_Implementation(TArray<FDialogueStateDefinition>& OutDefinitions) const
{
	OutDefinitions.Reset();
	return false;
}

// 미구현 태그 조회를 정상 부재로 간주하지 않는다.
bool IDialogueStateProvider::QueryDialogueTag_Implementation(FName TargetId, FGameplayTag Tag, bool& OutValue) const
{
	OutValue = false;
	return false;
}

// 미구현 수치 조회는 실패이며 출력값을 초기화한다.
bool IDialogueStateProvider::QueryDialogueValue_Implementation(FName TargetId, FName ItemId, double& OutValue) const
{
	OutValue = 0.0;
	return false;
}

// 미구현 수량 조회는 정상 미보유가 아니다.
bool IDialogueStateProvider::QueryDialogueCount_Implementation(FName TargetId, FName ItemId, int64& OutValue) const
{
	OutValue = 0;
	return false;
}
