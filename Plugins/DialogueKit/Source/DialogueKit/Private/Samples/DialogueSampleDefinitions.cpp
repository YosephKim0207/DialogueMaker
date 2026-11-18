#include "Samples/DialogueSampleDefinitions.h"

// 임시 목록을 완성한 뒤에만 공개하므로 손상된 원본에서 부분 목록이 나오지 않는다.
bool UDialogueSampleDefinitionCatalog::BuildDefinitions(TArray<FDialogueStateDefinition>& OutDefinitions, FString& OutError) const
{
	OutDefinitions.Reset();
	OutError.Reset();
	if (PlayerTargetId.IsNone() || NpcAffectionItemId.IsNone())
	{
		OutError = TEXT("샘플 PlayerTargetId와 NpcAffectionItemId는 비어 있을 수 없습니다.");
		return false;
	}
	const UDataTable* ItemTable = Items.LoadSynchronous();
	const UDataTable* SkillTable = Skills.LoadSynchronous();
	if (ItemTable == nullptr || ItemTable->GetRowStruct() != FDialogueSampleItemRow::StaticStruct() ||
		SkillTable == nullptr || SkillTable->GetRowStruct() != FDialogueSampleSkillRow::StaticStruct())
	{
		OutError = TEXT("Items/Skills 원본이 없거나 샘플 행 형식과 다릅니다.");
		return false;
	}

	TArray<FDialogueStateDefinition> Definitions;
	TSet<FDialogueStateKey> Keys;
	// 정의의 키는 표시명/행 이름과 독립적이며 종류와 대상까지 포함하여 중복을 판정한다.
	auto Add = [&](EDialogueStateKind Kind, FName TargetId, FName ItemId, const FText& DisplayName)
	{
		FDialogueStateDefinition Definition;
		Definition.Key.Kind = Kind;
		Definition.Key.TargetId = TargetId;
		Definition.Key.ItemId = ItemId;
		Definition.DisplayName = DisplayName;
		if (Definition.Key.IsValid() == false || DisplayName.IsEmptyOrWhitespace() || Keys.Contains(Definition.Key))
		{
			OutError = FString::Printf(TEXT("비어 있거나 중복된 샘플 정의: %s / %s"), *TargetId.ToString(), *ItemId.ToString());
			return false;
		}
		Keys.Add(Definition.Key);
		Definitions.Add(Definition);
		return true;
	};

	for (const auto& NpcReference : Npcs)
	{
		const UDialogueSampleNpcDefinition* Npc = NpcReference.LoadSynchronous();
		if (Npc == nullptr || Npc->Id.IsNone() || Npc->Id == PlayerTargetId || Npc->DisplayName.IsEmptyOrWhitespace())
		{
			OutError = TEXT("NPC 원본이 없거나 Id/DisplayName이 비어 있거나 플레이어 대상 ID와 같습니다.");
			return false;
		}
		if (Add(EDialogueStateKind::Value, Npc->Id, NpcAffectionItemId,
			FText::Format(NSLOCTEXT("DialogueKit", "SampleNpcAffection", "{0} 호감도"), Npc->DisplayName)) == false)
		{
			return false;
		}
	}
	for (FName RowName : ItemTable->GetRowNames())
	{
		const auto* Row = ItemTable->FindRow<FDialogueSampleItemRow>(RowName, TEXT("Sample item definitions"), false);
		if (Row == nullptr || Row->MaxStack < 1)
		{
			OutError = FString::Printf(TEXT("아이템 행 %s의 MaxStack은 1 이상이어야 합니다."), *RowName.ToString());
			return false;
		}
		if (Add(EDialogueStateKind::Count, PlayerTargetId, Row->Id, Row->DisplayName) == false)
		{
			return false;
		}
	}
	for (FName RowName : SkillTable->GetRowNames())
	{
		const auto* Row = SkillTable->FindRow<FDialogueSampleSkillRow>(RowName, TEXT("Sample skill definitions"), false);
		if (Row == nullptr || Row->MaxLevel < 1 || FMath::IsFinite(Row->CooldownSeconds) == false || Row->CooldownSeconds < 0.0f)
		{
			OutError = FString::Printf(TEXT("스킬 행 %s의 MaxLevel/CooldownSeconds가 유효하지 않습니다."), *RowName.ToString());
			return false;
		}
		if (Row->DisplayName.IsEmptyOrWhitespace() || Row->Id.IsNone())
		{
			OutError = FString::Printf(TEXT("스킬 행 %s의 Id/DisplayName이 비어 있습니다."), *RowName.ToString());
			return false;
		}
		if (Add(EDialogueStateKind::Value, PlayerTargetId, Row->Id,
			FText::Format(NSLOCTEXT("DialogueKit", "SampleSkillLevel", "{0} 레벨"), Row->DisplayName)) == false)
		{
			return false;
		}
	}
	Definitions.Sort([](const FDialogueStateDefinition& A, const FDialogueStateDefinition& B)
	{
		if (A.Key.Kind != B.Key.Kind) return uint8(A.Key.Kind) < uint8(B.Key.Kind);
		if (A.Key.TargetId != B.Key.TargetId) return A.Key.TargetId.LexicalLess(B.Key.TargetId);
		return A.Key.ItemId.LexicalLess(B.Key.ItemId);
	});
	OutDefinitions = MoveTemp(Definitions);
	return true;
}

// 상세 실패 이유가 필요한 호출자는 BuildDefinitions를 사용한다.
bool UDialogueSampleDefinitionCatalog::GetDialogueDefinitions_Implementation(TArray<FDialogueStateDefinition>& OutDefinitions) const
{
	FString Error;
	return BuildDefinitions(OutDefinitions, Error);
}

// 기존 제공자를 부분 변경하지 않고 새 샘플 제공자에만 검증된 정의를 등록한다.
UDialogueSampleStateProvider* UDialogueSampleDefinitionCatalog::CreateMemoryProvider(UObject* Outer, FString& OutError) const
{
	OutError.Reset();
	if (IsValid(Outer) == false)
	{
		OutError = TEXT("샘플 제공자를 보관할 유효한 Outer가 필요합니다.");
		return nullptr;
	}
	TArray<FDialogueStateDefinition> Definitions;
	if (BuildDefinitions(Definitions, OutError) == false)
	{
		return nullptr;
	}
	UDialogueSampleStateProvider* Provider = NewObject<UDialogueSampleStateProvider>(Outer);
	for (const auto& Definition : Definitions)
	{
		if (Provider->AddDefinition(Definition) == false)
		{
			OutError = TEXT("검증된 샘플 정의를 등록할 수 없습니다.");
			return nullptr;
		}
	}
	return Provider;
}
