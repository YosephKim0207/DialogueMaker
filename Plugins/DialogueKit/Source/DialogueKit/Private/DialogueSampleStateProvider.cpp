#include "DialogueSampleStateProvider.h"

// 같은 키는 표시 이름이 달라도 중복 정의다.
bool UDialogueSampleStateProvider::AddDefinition(const FDialogueStateDefinition& Definition)
{
	if (Definition.Key.IsValid() == false || Definitions.ContainsByPredicate([&](const FDialogueStateDefinition& Item) { return Item.Key == Definition.Key; }))
	{
		return false;
	}
	Definitions.Add(Definition);
	return true;
}

// 실제 게임은 이 샘플 대신 자신의 상태 원본을 조회하는 제공자를 구현한다.
bool UDialogueSampleStateProvider::SetSampleState(const FDialogueSampleState& NewState)
{
	FDialogueSampleState Current;
	if (GetSampleState(NewState.Key, Current) == false ||
		(NewState.Key.Kind == EDialogueStateKind::Count && NewState.Count < 0) ||
		(NewState.Key.Kind == EDialogueStateKind::Value && FMath::IsFinite(NewState.Value) == false))
	{
		return false;
	}
	const int32 Index = States.IndexOfByPredicate([&](const FDialogueSampleState& Item) { return Item.Key == NewState.Key; });
	if (Index == INDEX_NONE)
	{
		States.Add(NewState);
	}
	else
	{
		States[Index] = NewState;
	}
	return true;
}

// 조회 실패와 정상 기본값을 분리한다.
bool UDialogueSampleStateProvider::GetSampleState(const FDialogueStateKey& Key, FDialogueSampleState& OutState) const
{
	OutState = FDialogueSampleState();
	OutState.Key = Key;
	if (Key.IsValid() == false || Definitions.ContainsByPredicate([&](const FDialogueStateDefinition& Item) { return Item.Key == Key; }) == false)
	{
		return false;
	}
	if (const FDialogueSampleState* Found = States.FindByPredicate([&](const FDialogueSampleState& Item) { return Item.Key == Key; }))
	{
		OutState = *Found;
	}
	return true;
}

// 목록 제공은 상태를 변경하지 않는다.
bool UDialogueSampleStateProvider::GetDialogueDefinitions_Implementation(TArray<FDialogueStateDefinition>& OutDefinitions) const
{
	OutDefinitions = Definitions;
	return true;
}

// Tag의 부재는 정상 false다.
bool UDialogueSampleStateProvider::QueryDialogueTag_Implementation(FName TargetId, FGameplayTag Tag, bool& OutValue) const
{
	FDialogueStateKey Key;
	Key.TargetId = TargetId;
	Key.Tag = Tag;
	FDialogueSampleState State;
	const bool bFound = GetSampleState(Key, State);
	OutValue = State.bTagValue;
	return bFound;
}

// Value 종류만 조회한다.
bool UDialogueSampleStateProvider::QueryDialogueValue_Implementation(FName TargetId, FName ItemId, double& OutValue) const
{
	FDialogueStateKey Key;
	Key.Kind = EDialogueStateKind::Value;
	Key.TargetId = TargetId;
	Key.ItemId = ItemId;
	FDialogueSampleState State;
	const bool bFound = GetSampleState(Key, State);
	OutValue = State.Value;
	return bFound;
}

// int64 수량을 double로 변환하지 않는다.
bool UDialogueSampleStateProvider::QueryDialogueCount_Implementation(FName TargetId, FName ItemId, int64& OutValue) const
{
	FDialogueStateKey Key;
	Key.Kind = EDialogueStateKind::Count;
	Key.TargetId = TargetId;
	Key.ItemId = ItemId;
	FDialogueSampleState State;
	const bool bFound = GetSampleState(Key, State);
	OutValue = State.Count;
	return bFound;
}
