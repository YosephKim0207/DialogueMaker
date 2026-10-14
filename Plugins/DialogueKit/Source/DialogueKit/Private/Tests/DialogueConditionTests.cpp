#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueConditionLibrary.h"
#include "DialogueSampleStateProvider.h"
#include "NativeGameplayTags.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include <limits>

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DialogueConditionSample, "DialogueKit.Automation.ConditionSample");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueConditionContractTest,
	"DialogueKit.Contracts.Conditions.Evaluation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 혼합 조건, 정상 미보유/조회 오류, 항목별 True 대체, 정수 정밀도를 검사한다.
bool FDialogueConditionContractTest::RunTest(const FString& Parameters)
{
	UDialogueSampleStateProvider* Provider = NewObject<UDialogueSampleStateProvider>();
	FDialogueCondition Tag;
	Tag.Key.Tag = TAG_DialogueConditionSample;
	FDialogueCondition Value;
	Value.Key.Kind = EDialogueStateKind::Value;
	Value.Key.ItemId = TEXT("Charm");
	Value.Comparison = EDialogueComparison::GreaterOrEqual;
	Value.ValueOperand = 5.0;
	FDialogueCondition Count;
	Count.Key.Kind = EDialogueStateKind::Count;
	Count.Key.ItemId = TEXT("Gift");
	for (const FDialogueCondition& Condition : {Tag, Value, Count})
	{
		FDialogueStateDefinition Definition;
		Definition.Key = Condition.Key;
		TestTrue(TEXT("Register definition"), Provider->AddDefinition(Definition));
	}
	FDialogueSampleState State;
	State.Key = Value.Key;
	State.Value = 5.0;
	TestTrue(TEXT("Set value"), Provider->SetSampleState(State));
	FDialogueConditionSet Set;
	Set.Conditions = {Tag, Value, Count};
	const FDialogueDiagnosticContext Context;
	auto Evaluate = [&]() { return UDialogueConditionLibrary::EvaluateConditions(Set, Provider, false, Context, nullptr); };
	FDialogueConditionResult Result = Evaluate();
	TestFalse(TEXT("Known absent tag is false"), Result.bPassed);
	TestFalse(TEXT("Absent tag/count are not errors"), Result.bHadErrors);
	State.Key = Tag.Key;
	State.bTagValue = true;
	Provider->SetSampleState(State);
	TestTrue(TEXT("Mixed conditions all pass"), Evaluate().bPassed);
	FDialogueCondition Missing = Count;
	Missing.Key.ItemId = TEXT("Unknown");
	Count.CountOperand = 1;
	Set.Conditions = {Missing, Count};
	Result = Evaluate();
	TestTrue(TEXT("Missing definition is error"), Result.bHadErrors);
	TestFalse(TEXT("Error true does not hide another AND false"), Result.bPassed);
	Set.Combine = EDialogueConditionCombine::Any;
	TestTrue(TEXT("Error true participates in OR"), Evaluate().bPassed);
	Set.Combine = EDialogueConditionCombine::All;
	Count.CountOperand = 9007199254740993LL;
	Set.Conditions = {Count};
	State.Key = Count.Key;
	State.Count = Count.CountOperand;
	Provider->SetSampleState(State);
	TestTrue(TEXT("Exact int64 equality"), Evaluate().bPassed);
	Set.Conditions[0].CountOperand -= 1;
	TestFalse(TEXT("Adjacent int64 not rounded equal"), Evaluate().bPassed);
	Set.Conditions.Reset();
	Result = Evaluate();
	TestTrue(TEXT("Empty normal set allows"), Result.bPassed && Result.bHadErrors == false);
	Result = UDialogueConditionLibrary::EvaluateConditions(Set, nullptr, true, Context, nullptr);
	TestTrue(TEXT("Empty branch allows with diagnostic"), Result.bPassed && Result.bHadErrors);
	Set.Conditions = {Value};
	Result = UDialogueConditionLibrary::EvaluateConditions(Set, nullptr, false, Context, nullptr);
	TestTrue(TEXT("Missing provider is reported, not sample fallback"), Result.bPassed && Result.bHadErrors);
	Set.Conditions = {Value};
	const bool Expected[] = {true, false, false, true, false, true};
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Set.Conditions[0].Comparison = static_cast<EDialogueComparison>(Index);
		TestEqual(TEXT("Comparison boundary"), Evaluate().bPassed, Expected[Index]);
	}
	Set.Conditions[0].ValueOperand = std::numeric_limits<double>::quiet_NaN();
	TestTrue(TEXT("NaN operand is diagnosed"), Evaluate().bHadErrors);
	Set.Conditions = {Value};
	Set.Conditions[0].Key.TargetId = TEXT("OtherActor");
	TestTrue(TEXT("Targets are isolated"), Evaluate().bHadErrors);
	Set.Combine = static_cast<EDialogueConditionCombine>(255);
	Result = Evaluate();
	TestTrue(TEXT("Invalid combine true with error"), Result.bHadErrors && Result.bPassed);
	Set.Combine = EDialogueConditionCombine::All;
	Set.Conditions = {Value};
	int64 UnimplementedCount = 9;
	TestFalse(TEXT("Base missing implementation fails"), Provider->IDialogueStateProvider::QueryDialogueCount_Implementation(NAME_None, TEXT("Gift"), UnimplementedCount));
	TestEqual(TEXT("Base missing output initialized"), UnimplementedCount, int64(0));
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FDialogueConditionSet::StaticStruct()->SerializeItem(Writer, &Set, nullptr);
	FDialogueConditionSet Loaded;
	FMemoryReader Reader(Bytes);
	FDialogueConditionSet::StaticStruct()->SerializeItem(Reader, &Loaded, nullptr);
	TestEqual(TEXT("Serialized condition key"), Loaded.Conditions[0].Key.ItemId, Value.Key.ItemId);
	TestTrue(TEXT("Serialized condition evaluates"), UDialogueConditionLibrary::EvaluateConditions(Loaded, Provider, false, Context, nullptr).bPassed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueDefinitionTableTest,
	"DialogueKit.Contracts.Conditions.DefinitionTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// DataTable 정의와 현재 상태가 분리되며 중복 행은 전체 실패로 반환된다.
bool FDialogueDefinitionTableTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FDialogueStateDefinition::StaticStruct();
	FDialogueStateDefinition Definition;
	Definition.Key.Kind = EDialogueStateKind::Count;
	Definition.Key.ItemId = TEXT("Gift");
	Definition.DisplayName = FText::FromString(TEXT("Gift item"));
	Table->AddRow(TEXT("Gift"), Definition);
	TArray<FDialogueStateDefinition> Definitions;
	TestTrue(TEXT("Common definition table"), UDialogueConditionLibrary::ReadDefinitionTable(Table, Definitions));
	TestEqual(TEXT("Definition count"), Definitions.Num(), 1);
	Table->AddRow(TEXT("Duplicate"), Definition);
	TestFalse(TEXT("Duplicate keys rejected"), UDialogueConditionLibrary::ReadDefinitionTable(Table, Definitions));
	TestTrue(TEXT("No partial output"), Definitions.IsEmpty());
	TestFalse(TEXT("Missing table rejected"), UDialogueConditionLibrary::ReadDefinitionTable(nullptr, Definitions));
	return true;
}
#endif
