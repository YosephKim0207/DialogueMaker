#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueConditionLibrary.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueGraph.h"
#include "DialogueCandidateSelector.h"
#include "NativeGameplayTags.h"
#include "UObject/StrongObjectPtr.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GroupTarget, "DialogueKit.Test.GroupTarget");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueConditionGroupsTest, "DialogueKit.Contracts.Conditions.Groups", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 16개 조합과 그룹별 오류·빈 조건·기존 조건의 추가 적용을 검증한다.
bool FDialogueConditionGroupsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDialogueSampleStateProvider> Provider(NewObject<UDialogueSampleStateProvider>());
	TArray<FDialogueCondition> Atoms;
	for (const FName Id : {FName(TEXT("A")), FName(TEXT("B")), FName(TEXT("C")), FName(TEXT("D"))})
	{
		FDialogueCondition Atom;
		Atom.Key.Kind = EDialogueStateKind::Count;
		Atom.Key.ItemId = Id;
		Atom.CountOperand = 1;
		FDialogueStateDefinition Definition;
		Definition.Key = Atom.Key;
		Provider->AddDefinition(Definition);
		Atoms.Add(Atom);
	}
	FDialogueConditionSet Set;
	Set.RequiredConditions = {Atoms[0]};
	Set.AnyConditions = {Atoms[1], Atoms[2]};
	Set.BlockedConditions = {Atoms[3]};
	auto Evaluate = [&]() { return UDialogueConditionLibrary::EvaluateConditions(Set, Provider.Get(), false, FDialogueDiagnosticContext(), nullptr); };
	for (int32 Mask = 0; Mask < 16; ++Mask)
	{
		for (int32 Index = 0; Index < 4; ++Index)
		{
			FDialogueSampleState State;
			State.Key = Atoms[Index].Key;
			State.Count = (Mask >> Index) & 1;
			Provider->SetSampleState(State);
		}
		const bool Expected = (Mask & 1) && ((Mask & 2) || (Mask & 4)) && !(Mask & 8);
		const FDialogueConditionResult Result = Evaluate();
		TestEqual(*FString::Printf(TEXT("A AND (B OR C) AND NOT D: %d"), Mask), Result.bPassed, Expected);
		TestFalse(TEXT("Known states no error"), Result.bHadErrors);
	}
	// 마지막 mask에서 네 항목은 모두 참이다.
	Set.BlockedConditions.Reset();
	TestTrue(TEXT("Empty blocked does not restrict"), Evaluate().bPassed);
	Set.AnyConditions.Reset();
	TestTrue(TEXT("Empty alternatives do not restrict"), Evaluate().bPassed);
	Set.RequiredConditions.Reset();
	TestTrue(TEXT("All empty normal allows"), Evaluate().bPassed);
	TestTrue(TEXT("All empty branch diagnoses"), UDialogueConditionLibrary::EvaluateConditions(Set, Provider.Get(), true, FDialogueDiagnosticContext(), nullptr).bHadErrors);
	Set.RequiredConditions = {Atoms[0]};
	TestFalse(TEXT("New-only branch is not empty"), UDialogueConditionLibrary::EvaluateConditions(Set, Provider.Get(), true, FDialogueDiagnosticContext(), nullptr).bHadErrors);
	FDialogueCondition Missing = Atoms[0];
	Missing.Key.ItemId = TEXT("Unregistered");
	Set.RequiredConditions = {Missing};
	TestTrue(TEXT("Required error allows with diagnosis"), Evaluate().bPassed && Evaluate().bHadErrors);
	Set.RequiredConditions.Reset();
	Set.AnyConditions = {Missing};
	TestTrue(TEXT("Alternative error allows"), Evaluate().bPassed && Evaluate().bHadErrors);
	Set.AnyConditions.Reset();
	Set.BlockedConditions = {Missing};
	const auto BlockError = Evaluate();
	TestTrue(TEXT("Blocked error does not block"), BlockError.bPassed && BlockError.bHadErrors);
	TestEqual(TEXT("Blocked diagnostic explains handling"), BlockError.Diagnostics[0].Handling, FString(TEXT("ContinueWithoutBlock")));
	Set.BlockedConditions.Add(Atoms[3]);
	TestFalse(TEXT("Normal forbidden true still blocks"), Evaluate().bPassed);
	Set.Combine = static_cast<EDialogueConditionCombine>(255);
	TestFalse(TEXT("Invalid legacy mode does not skip new blocked"), Evaluate().bPassed);
	Set.Combine = EDialogueConditionCombine::Any;
	Set.BlockedConditions.Reset();
	FDialogueCondition FalseAtom = Atoms[0];
	FalseAtom.CountOperand = 0;
	Set.Conditions = {FalseAtom};
	Set.RequiredConditions = {Atoms[0]};
	TestFalse(TEXT("Legacy OR group also required"), Evaluate().bPassed);
	Set.Conditions.Add(Atoms[1]);
	TestTrue(TEXT("Legacy OR retained"), Evaluate().bPassed);
	Set.Combine = EDialogueConditionCombine::All;
	TestFalse(TEXT("Legacy AND retained"), Evaluate().bPassed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueConditionGroupsMetadataTest, "DialogueKit.Selection.GroupMetadataCompatibility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 새 payload의 그룹/유형 왕복과 실제 새 필드 없는 v1 텍스트의 이전 의미를 검증한다.
bool FDialogueConditionGroupsMetadataTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDialogueGraph> Graph(NewObject<UDialogueGraph>());
	Graph->InitializeBlockIdentity();
	Graph->DialogueTarget = TAG_GroupTarget;
	Graph->StoryScope.Mode = EDialogueStoryScope::Unrestricted;
	Graph->DialogueGraphType = EDialogueGraphType::DailyChat;
	FDialogueCondition Condition;
	Condition.Key.Kind = EDialogueStateKind::Count;
	Condition.Key.ItemId = TEXT("Ticket");
	Condition.CountOperand = MAX_int64 - 1;
	Graph->EntryConditions.RequiredConditions = {Condition};
	Graph->EntryConditions.AnyConditions = {Condition};
	Graph->EntryConditions.BlockedConditions = {Condition};
	FDialogueBlockMetadata Parsed;
	TestTrue(TEXT("v2 text roundtrip"), FDialogueBlockMetadata::FromRegistryValue(Graph->GetSelectionMetadata().ToRegistryValue(), Parsed));
	TestEqual(TEXT("New writer v2"), Parsed.MetadataVersion, 2);
	TestEqual(TEXT("Classification preserved"), Parsed.DialogueGraphType, EDialogueGraphType::DailyChat);
	TestEqual(TEXT("Required exact integer"), Parsed.EntryConditions.RequiredConditions[0].CountOperand, MAX_int64 - 1);
	TestEqual(TEXT("Alternative retained"), Parsed.EntryConditions.AnyConditions.Num(), 1);
	TestEqual(TEXT("Forbidden retained"), Parsed.EntryConditions.BlockedConditions.Num(), 1);
	FDialogueBlockMetadata WrongVersion = Parsed;
	WrongVersion.MetadataVersion = 1;
	TestFalse(TEXT("v1 must not silently carry new groups"), WrongVersion.IsValid());
	FDialogueBlockMetadata WrongType = Parsed;
	WrongType.DialogueGraphType = static_cast<EDialogueGraphType>(255);
	TestFalse(TEXT("Corrupt v2 type rejected"), WrongType.IsValid());
	TestFalse(TEXT("Corrupt type can be shown as unknown"), WrongType.HasKnownGraphType());
	FAssetDataTagMap IdentityTags;
	IdentityTags.Add(FDialogueBlockMetadata::RegistryKey(), Parsed.ToRegistryValue());
	FAssetData IdentityAsset(FName(TEXT("/Game/GroupIdentity")), FName(TEXT("/Game")), FName(TEXT("GroupIdentity")), UDialogueGraph::StaticClass()->GetClassPathName(), IdentityTags);
	FDialogueBlockReference Identity;
	TestTrue(TEXT("v2 metadata identity fallback"), FDialogueBlockMetadata::ReadIdentity(IdentityAsset, Identity));
	TestTrue(TEXT("v2 identity unchanged"), Identity == Parsed.Block);
	// 이전 필드만 있는 독립 fixture: 새 그룹·유형 이름 자체가 존재하지 않는다.
	const FString OldText = FString::Printf(TEXT("(MetadataVersion=1,Block=(BlockId=%s,ContentVersion=1),TargetTag=(TagName=\"DialogueKit.Test.GroupTarget\"),StoryScope=(Mode=Unrestricted),EntryConditions=(Combine=Any,Conditions=((Key=(Kind=Count,ItemId=Ticket),CountOperand=0))))"), *Graph->GetBlockId().ToString(EGuidFormats::Digits));
	TestFalse(TEXT("Fixture omits new members"), OldText.Contains(TEXT("RequiredConditions")) || OldText.Contains(TEXT("DialogueGraphType")));
	if (!TestTrue(TEXT("Old v1 fixture reads"), FDialogueBlockMetadata::FromRegistryValue(OldText, Parsed))) return false;
	TestTrue(TEXT("Old groups default empty"), Parsed.EntryConditions.RequiredConditions.IsEmpty() && Parsed.EntryConditions.AnyConditions.IsEmpty() && Parsed.EntryConditions.BlockedConditions.IsEmpty());
	TestEqual(TEXT("Legacy Any preserved"), Parsed.EntryConditions.Combine, EDialogueConditionCombine::Any);
	TStrongObjectPtr<UDialogueSampleStateProvider> Provider(NewObject<UDialogueSampleStateProvider>());
	FDialogueStateDefinition Definition;
	Definition.Key = Condition.Key;
	Provider->AddDefinition(Definition);
	TestTrue(TEXT("Old zero count condition unchanged"), UDialogueMetadataLibrary::IsEligible(Parsed, TAG_GroupTarget, nullptr, FDialogueStoryProgress(), Provider.Get(), nullptr));
	Parsed.MetadataVersion = 3;
	TestFalse(TEXT("Unknown version rejected"), Parsed.IsValid());
	for (const FString Name : {TEXT("Quest"), TEXT("Story"), TEXT("DailyChat"), TEXT("Broken"), TEXT("")})
	{
		FAssetDataTagMap Tags;
		Tags.Add(FDialogueBlockMetadata::RegistryKey(), OldText);
		if (!Name.IsEmpty()) Tags.Add(TEXT("DialogueGraphType"), Name);
		FAssetData OldAsset(FName(TEXT("/Game/OldType")), FName(TEXT("/Game")), FName(TEXT("OldType")), UDialogueGraph::StaticClass()->GetClassPathName(), Tags);
		FDialogueBlockMetadata OldMetadata;
		TestTrue(TEXT("v1 remains readable without classification"), FDialogueBlockMetadata::ReadAsset(OldAsset, OldMetadata));
		const int64 ExpectedType = StaticEnum<EDialogueGraphType>()->GetValueByNameString(Name);
		if (Name == TEXT("Broken") || Name.IsEmpty()) TestFalse(TEXT("Missing/broken v1 type is unknown"), OldMetadata.HasKnownGraphType());
		else TestEqual(TEXT("v1 original classification retained"), int64(OldMetadata.DialogueGraphType), ExpectedType);
	}
	// 유형이 달라도 같은 후보/우선순위/난수라면 새 선정 결과는 동일하다.
	FDialogueBlockCandidate Candidate;
	Candidate.Metadata = Graph->GetSelectionMetadata();
	Candidate.Metadata.EntryConditions = FDialogueConditionSet();
	Candidate.AssetPath = FSoftObjectPath(TEXT("/Game/TestBlock.TestBlock"));
	TStrongObjectPtr<UDialogueCandidateSelector> Selector(NewObject<UDialogueCandidateSelector>());
	for (const EDialogueGraphType Type : {EDialogueGraphType::Quest, EDialogueGraphType::Story, EDialogueGraphType::DailyChat})
	{
		Candidate.Metadata.DialogueGraphType = Type;
		Candidate.Metadata.Priority = 10;
		FDialogueBlockCandidate Lower = Candidate;
		Lower.Metadata.Block.BlockId = FGuid::NewGuid();
		Lower.Metadata.Priority = 0;
		Lower.Metadata.DialogueGraphType = EDialogueGraphType::Quest;
		Lower.AssetPath = FSoftObjectPath(TEXT("/Game/LowBlock.LowBlock"));
		Selector->EndSession();
		Selector->BeginSession(TAG_GroupTarget);
		const auto Result = Selector->Select({Lower, Candidate}, nullptr, FDialogueStoryProgress(), Provider.Get(), nullptr);
		TestTrue(TEXT("Classification does not block new selection"), Result.bSelected && Result.Candidate.Metadata.Block == Candidate.Metadata.Block);
	}
	return true;
}
#endif
