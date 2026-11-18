#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Samples/DialogueSampleDefinitions.h"
#include "DialogueConditionLibrary.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

namespace
{
	const TCHAR* CatalogPath = TEXT("/DialogueKit/Sample/GameDefinitions/DA_SampleDefinitionCatalog.DA_SampleDefinitionCatalog");

	// 배포 에셋을 수정하지 않고 잘못된 입력을 만드는 독립 메모리 원본이다.
	struct FSampleSources
	{
		TStrongObjectPtr<UDialogueSampleDefinitionCatalog> Catalog{NewObject<UDialogueSampleDefinitionCatalog>()};
		TStrongObjectPtr<UDataTable> Items{NewObject<UDataTable>()};
		TStrongObjectPtr<UDataTable> Skills{NewObject<UDataTable>()};
		TStrongObjectPtr<UDialogueSampleNpcDefinition> Npc{NewObject<UDialogueSampleNpcDefinition>()};

		// 최소 유효 원본을 만들고 별도 상태를 아직 설정하지 않는다.
		FSampleSources()
		{
			Items->RowStruct = FDialogueSampleItemRow::StaticStruct();
			FDialogueSampleItemRow Item;
			Item.Id = TEXT("Item.Test");
			Item.DisplayName = FText::FromString(TEXT("시험 아이템"));
			Items->AddRow(TEXT("EditorRow"), Item);
			Skills->RowStruct = FDialogueSampleSkillRow::StaticStruct();
			FDialogueSampleSkillRow Skill;
			Skill.Id = TEXT("Skill.Test");
			Skill.DisplayName = FText::FromString(TEXT("시험 스킬"));
			Skills->AddRow(TEXT("EditorRow"), Skill);
			Npc->Id = TEXT("NPC.Test");
			Npc->DisplayName = FText::FromString(TEXT("시험 NPC"));
			Catalog->Items = Items.Get();
			Catalog->Skills = Skills.Get();
			Catalog->Npcs.Add(Npc.Get());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueDefinitionSampleAssetsTest, "DialogueKit.Samples.DefinitionAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 생성기와 별도 프로세스에서 배포된 다섯 에셋과 실제 저장된 원본-정의 대응을 검증한다.
bool FDialogueDefinitionSampleAssetsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDialogueSampleDefinitionCatalog> Catalog(LoadObject<UDialogueSampleDefinitionCatalog>(nullptr, CatalogPath));
	if (TestNotNull(TEXT("Saved sample catalog loads"), Catalog.Get()) == false) return false;
	const UDataTable* Items = Catalog->Items.LoadSynchronous();
	const UDataTable* Skills = Catalog->Skills.LoadSynchronous();
	if (TestNotNull(TEXT("Saved items load"), Items) == false || TestNotNull(TEXT("Saved skills load"), Skills) == false) return false;
	TestTrue(TEXT("Item row type"), Items->GetRowStruct() == FDialogueSampleItemRow::StaticStruct());
	TestTrue(TEXT("Skill row type"), Skills->GetRowStruct() == FDialogueSampleSkillRow::StaticStruct());
	TestEqual(TEXT("Six item examples"), Items->GetRowNames().Num(), 6);
	TestEqual(TEXT("Six skill examples"), Skills->GetRowNames().Num(), 6);
	TestEqual(TEXT("Two NPC source assets"), Catalog->Npcs.Num(), 2);
	TArray<FDialogueStateDefinition> Definitions;
	FString Error;
	if (TestTrue(TEXT("Saved catalog converts"), Catalog->BuildDefinitions(Definitions, Error)) == false) return false;
	TestTrue(TEXT("Success clears error"), Error.IsEmpty());
	TestEqual(TEXT("14 distinct state definitions"), Definitions.Num(), 14);

	auto Find = [&](EDialogueStateKind Kind, FName Target, FName Item)
	{
		return Definitions.FindByPredicate([&](const FDialogueStateDefinition& Entry)
		{
			return Entry.Key.Kind == Kind && Entry.Key.TargetId == Target && Entry.Key.ItemId == Item;
		});
	};
	for (const auto& NpcReference : Catalog->Npcs)
	{
		const auto* Npc = NpcReference.LoadSynchronous();
		if (TestNotNull(TEXT("Saved NPC source loads"), Npc) == false) return false;
		const auto* Definition = Find(EDialogueStateKind::Value, Npc->Id, TEXT("Affection"));
		if (TestNotNull(TEXT("NPC ID maps to target"), Definition))
		{
			TestTrue(TEXT("NPC display name included"), Definition->DisplayName.ToString().Contains(Npc->DisplayName.ToString()));
		}
	}
	for (FName RowName : Items->GetRowNames())
	{
		const auto* Row = Items->FindRow<FDialogueSampleItemRow>(RowName, TEXT("Sample test"), false);
		if (TestNotNull(TEXT("Item row"), Row) == false) return false;
		const auto* Definition = Find(EDialogueStateKind::Count, TEXT("Player"), Row->Id);
		if (TestNotNull(TEXT("Item explicit ID maps to key"), Definition)) TestEqual(TEXT("Item name preserved"), Definition->DisplayName.ToString(), Row->DisplayName.ToString());
	}
	for (FName RowName : Skills->GetRowNames())
	{
		const auto* Row = Skills->FindRow<FDialogueSampleSkillRow>(RowName, TEXT("Sample test"), false);
		if (TestNotNull(TEXT("Skill row"), Row) == false) return false;
		TestNotNull(TEXT("Skill explicit ID maps to Value key"), Find(EDialogueStateKind::Value, TEXT("Player"), Row->Id));
	}
	TArray<FDialogueStateDefinition> InterfaceDefinitions;
	TestTrue(TEXT("Public definition interface"), IDialogueDefinitionProvider::Execute_GetDialogueDefinitions(Catalog.Get(), InterfaceDefinitions));
	if (TestEqual(TEXT("Same interface size"), InterfaceDefinitions.Num(), Definitions.Num()))
	{
		for (int32 Index = 0; Index < Definitions.Num(); ++Index)
		{
			TestTrue(TEXT("Stable interface key order"), Definitions[Index].Key == InterfaceDefinitions[Index].Key);
			TestEqual(TEXT("Same interface labels"), Definitions[Index].DisplayName.ToString(), InterfaceDefinitions[Index].DisplayName.ToString());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueDefinitionSampleStateTest, "DialogueKit.Samples.DefinitionStateSeparation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 정의의 최대값을 현재 값으로 사용하지 않고 독립 제공자에서 조건을 평가한다.
bool FDialogueDefinitionSampleStateTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDialogueSampleDefinitionCatalog> Catalog(LoadObject<UDialogueSampleDefinitionCatalog>(nullptr, CatalogPath));
	if (TestNotNull(TEXT("Catalog"), Catalog.Get()) == false) return false;
	FString Error;
	TStrongObjectPtr<UDialogueSampleStateProvider> Provider(Catalog->CreateMemoryProvider(GetTransientPackage(), Error));
	TStrongObjectPtr<UDialogueSampleStateProvider> Other(Catalog->CreateMemoryProvider(GetTransientPackage(), Error));
	if (TestNotNull(TEXT("New provider"), Provider.Get()) == false || TestNotNull(TEXT("Independent provider"), Other.Get()) == false) return false;
	int64 Count = -1;
	double Level = -1.0;
	TestTrue(TEXT("Known item query"), IDialogueStateProvider::Execute_QueryDialogueCount(Provider.Get(), TEXT("Player"), TEXT("Item.HealthPotion"), Count));
	TestEqual(TEXT("Initial count is zero, not MaxStack"), Count, int64(0));
	TestTrue(TEXT("Known skill query"), IDialogueStateProvider::Execute_QueryDialogueValue(Provider.Get(), TEXT("Player"), TEXT("Skill.Firebolt"), Level));
	TestEqual(TEXT("Initial level is zero, not MaxLevel"), Level, 0.0);
	TestFalse(TEXT("Unknown target is not inferred"), IDialogueStateProvider::Execute_QueryDialogueCount(Provider.Get(), TEXT("Other"), TEXT("Item.HealthPotion"), Count));
	FDialogueSampleState State;
	State.Key.Kind = EDialogueStateKind::Count;
	State.Key.TargetId = TEXT("Player");
	State.Key.ItemId = TEXT("Item.HealthPotion");
	State.Count = 3;
	TestTrue(TEXT("Set current sample state"), Provider->SetSampleState(State));
	FDialogueCondition Condition;
	Condition.Key = State.Key;
	Condition.Comparison = EDialogueComparison::GreaterOrEqual;
	Condition.CountOperand = 3;
	FDialogueConditionSet Conditions;
	Conditions.Conditions.Add(Condition);
	TestTrue(TEXT("Current count satisfies condition"), UDialogueConditionLibrary::EvaluateConditions(Conditions, Provider.Get(), false, FDialogueDiagnosticContext(), nullptr).bPassed);
	TestFalse(TEXT("Another provider retains zero"), UDialogueConditionLibrary::EvaluateConditions(Conditions, Other.Get(), false, FDialogueDiagnosticContext(), nullptr).bPassed);
	const auto* Item = Catalog->Items.LoadSynchronous()->FindRow<FDialogueSampleItemRow>(TEXT("HealthPotion"), TEXT("State separation"), false);
	if (TestNotNull(TEXT("Original item definition"), Item)) TestEqual(TEXT("Definition MaxStack unchanged"), Item->MaxStack, 99);
	TestNull(TEXT("Invalid outer rejected"), Catalog->CreateMemoryProvider(nullptr, Error));
	TestFalse(TEXT("Failure reason provided"), Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueDefinitionSampleValidationTest, "DialogueKit.Samples.DefinitionValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 잘못된 원본의 부분 출력과 상태 자동 생성, 표시명/행 이름에 의한 ID 변화를 방지한다.
bool FDialogueDefinitionSampleValidationTest::RunTest(const FString& Parameters)
{
	FSampleSources Sources;
	const UFunction* BuildFunction = UDialogueSampleDefinitionCatalog::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UDialogueSampleDefinitionCatalog, BuildDefinitions));
	const UFunction* CreateFunction = UDialogueSampleDefinitionCatalog::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UDialogueSampleDefinitionCatalog, CreateMemoryProvider));
	if (TestNotNull(TEXT("Blueprint build API"), BuildFunction)) TestFalse(TEXT("Synchronous source loading is not BlueprintPure"), BuildFunction->HasAnyFunctionFlags(FUNC_BlueprintPure));
	if (TestNotNull(TEXT("Blueprint create API"), CreateFunction)) TestFalse(TEXT("Provider allocation is not BlueprintPure"), CreateFunction->HasAnyFunctionFlags(FUNC_BlueprintPure));
	TArray<FDialogueStateDefinition> Definitions;
	FString Error;
	TestTrue(TEXT("Valid in-memory sources"), Sources.Catalog->BuildDefinitions(Definitions, Error));
	const auto OriginalDefinitions = Definitions;
	auto Reject = [&](const TCHAR* Context)
	{
		Definitions = OriginalDefinitions;
		TestFalse(Context, Sources.Catalog->BuildDefinitions(Definitions, Error));
		TestEqual(TEXT("Failure never returns partial definitions"), Definitions.Num(), 0);
		TestFalse(TEXT("Reason is returned"), Error.IsEmpty());
		Definitions = OriginalDefinitions;
		TestFalse(TEXT("Interface rejects the same invalid source"), IDialogueDefinitionProvider::Execute_GetDialogueDefinitions(Sources.Catalog.Get(), Definitions));
		TestEqual(TEXT("Interface clears previous output on failure"), Definitions.Num(), 0);
	};
	Sources.Catalog->PlayerTargetId = NAME_None;
	Reject(TEXT("Empty player ID"));
	Sources.Catalog->PlayerTargetId = TEXT("Player");
	Sources.Catalog->NpcAffectionItemId = NAME_None;
	Reject(TEXT("Empty affection key"));
	Sources.Catalog->NpcAffectionItemId = TEXT("Affection");
	Sources.Catalog->Items = Sources.Skills.Get();
	Reject(TEXT("Wrong table type"));
	Sources.Catalog->Items.Reset();
	Reject(TEXT("Missing required table"));
	Sources.Catalog->Items = Sources.Items.Get();
	Sources.Catalog->Npcs.Add(Sources.Npc.Get());
	Reject(TEXT("Duplicate NPC ID"));
	Sources.Catalog->Npcs.Pop();
	Sources.Npc->Id = TEXT("Player");
	Reject(TEXT("NPC cannot impersonate player"));
	Sources.Npc->Id = NAME_None;
	Reject(TEXT("Empty NPC ID"));
	Sources.Npc->Id = TEXT("NPC.Test");
	Sources.Npc->DisplayName = FText::GetEmpty();
	Reject(TEXT("Empty NPC name"));
	Sources.Npc->DisplayName = FText::FromString(TEXT("시험 NPC"));

	auto* Item = Sources.Items->FindRow<FDialogueSampleItemRow>(TEXT("EditorRow"), TEXT("Validation"), false);
	auto* Skill = Sources.Skills->FindRow<FDialogueSampleSkillRow>(TEXT("EditorRow"), TEXT("Validation"), false);
	if (Item == nullptr || Skill == nullptr) return false;
	Item->Id = NAME_None;
	Reject(TEXT("Empty item ID"));
	Item->Id = TEXT("Item.Test");
	Item->DisplayName = FText::GetEmpty();
	Reject(TEXT("Empty item display name"));
	Item->DisplayName = FText::FromString(TEXT("시험 아이템"));
	Item->MaxStack = 0;
	Reject(TEXT("Invalid stack limit"));
	Item->MaxStack = 1;
	Sources.Items->AddRow(TEXT("Duplicate"), *Item);
	Reject(TEXT("Duplicate explicit item ID under different row names"));
	Sources.Items->RemoveRow(TEXT("Duplicate"));
	Skill->Id = NAME_None;
	Reject(TEXT("Empty skill ID"));
	Skill->Id = TEXT("Skill.Test");
	Skill->DisplayName = FText::GetEmpty();
	Reject(TEXT("Empty skill name"));
	Skill->DisplayName = FText::FromString(TEXT("시험 스킬"));
	Skill->MaxLevel = 0;
	Reject(TEXT("Invalid max level"));
	Skill->MaxLevel = 1;
	Skill->CooldownSeconds = -1.0f;
	Reject(TEXT("Negative cooldown"));
	Skill->CooldownSeconds = std::numeric_limits<float>::quiet_NaN();
	Reject(TEXT("Non-finite cooldown"));
	Skill->CooldownSeconds = 0.0f;
	Sources.Skills->AddRow(TEXT("Duplicate"), *Skill);
	Reject(TEXT("Duplicate skill ID"));
	Sources.Skills->RemoveRow(TEXT("Duplicate"));

	// 행 이름과 표시 이름을 바꿔도 원본의 명시 ID는 유지된다.
	Item = Sources.Items->FindRow<FDialogueSampleItemRow>(TEXT("EditorRow"), TEXT("Rename"), false);
	FDialogueSampleItemRow Renamed = *Item;
	Renamed.DisplayName = FText::FromString(TEXT("변경된 표시명"));
	// 해석되지 않는 soft 경로를 넣어 이미지 로드를 요구하지 않는지 확인한다.
	Renamed.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NotLoadedSampleIcon.NotLoadedSampleIcon")));
	Sources.Items->RemoveRow(TEXT("EditorRow"));
	Sources.Items->AddRow(TEXT("RenamedEditorRow"), Renamed);
	TestTrue(TEXT("Rename and unloaded icon do not break conversion"), Sources.Catalog->BuildDefinitions(Definitions, Error));
	if (TestEqual(TEXT("Same number after rename"), Definitions.Num(), OriginalDefinitions.Num()))
	{
		for (int32 Index = 0; Index < Definitions.Num(); ++Index)
		{
			TestTrue(TEXT("Same stable keys and sorted order"), Definitions[Index].Key == OriginalDefinitions[Index].Key);
		}
	}
	const auto* Updated = Definitions.FindByPredicate([](const FDialogueStateDefinition& Definition) { return Definition.Key.ItemId == TEXT("Item.Test"); });
	if (TestNotNull(TEXT("Updated item definition"), Updated)) TestEqual(TEXT("Fresh label from original"), Updated->DisplayName.ToString(), Renamed.DisplayName.ToString());
	Sources.Catalog->PlayerTargetId = TEXT("Party.Leader");
	Sources.Catalog->NpcAffectionItemId = TEXT("Trust");
	TestTrue(TEXT("Custom sample mapping"), Sources.Catalog->BuildDefinitions(Definitions, Error));
	TestTrue(TEXT("Player target is not hardcoded"), Definitions.ContainsByPredicate([](const FDialogueStateDefinition& Definition)
	{
		return Definition.Key.TargetId == TEXT("Party.Leader") && Definition.Key.ItemId == TEXT("Item.Test");
	}));
	TestTrue(TEXT("NPC attribute is not hardcoded"), Definitions.ContainsByPredicate([](const FDialogueStateDefinition& Definition)
	{
		return Definition.Key.TargetId == TEXT("NPC.Test") && Definition.Key.ItemId == TEXT("Trust");
	}));
	return true;
}
#endif
