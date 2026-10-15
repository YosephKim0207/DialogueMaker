#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueSampleStateProvider.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_MetadataTarget, "DialogueKit.Test.MetadataTarget");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_MetadataChild, "DialogueKit.Test.MetadataTarget.Child");
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueMetadataTest, "DialogueKit.Selection.Metadata", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 조건 정의와 int64 수량이 손실 없이 왕복하며 대상은 정확히 일치해야 한다.
bool FDialogueMetadataTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Graph = NewObject<UDialogueGraph>();
	Graph->InitializeBlockIdentity();
	Graph->DialogueTarget = TAG_MetadataTarget;
	Graph->StoryScope.Mode = EDialogueStoryScope::Unrestricted;
	FDialogueCondition Condition;
	Condition.Key.Kind = EDialogueStateKind::Count;
	Condition.Key.ItemId = TEXT("Item_With_Punctuation,=()");
	Condition.CountOperand = MAX_int64 - 1;
	Graph->EntryConditions.Conditions.Add(Condition);
	FDialogueBlockMetadata Metadata;
	TestTrue(TEXT("Registry text roundtrip"), FDialogueBlockMetadata::FromRegistryValue(Graph->GetSelectionMetadata().ToRegistryValue(), Metadata));
	TestEqual(TEXT("int64 preserved"), Metadata.EntryConditions.Conditions[0].CountOperand, MAX_int64 - 1);
	TestEqual(TEXT("Item ID preserved"), Metadata.EntryConditions.Conditions[0].Key.ItemId, Condition.Key.ItemId);
	UDialogueSampleStateProvider* Provider = NewObject<UDialogueSampleStateProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key = Condition.Key;
	Provider->AddDefinition(Definition);
	FDialogueSampleState State;
	State.Key = Condition.Key;
	State.Count = Condition.CountOperand;
	Provider->SetSampleState(State);
	TestTrue(TEXT("Eligible"), UDialogueMetadataLibrary::IsEligible(Metadata, TAG_MetadataTarget, nullptr, FDialogueStoryProgress(), Provider, nullptr));
	TestFalse(TEXT("Child is not exact target"), UDialogueMetadataLibrary::IsEligible(Metadata, TAG_MetadataChild, nullptr, FDialogueStoryProgress(), Provider, nullptr));
	Metadata.Availability = EDialogueBlockAvailability::RestoreOnly;
	TestFalse(TEXT("No new exposure"), UDialogueMetadataLibrary::IsEligible(Metadata, TAG_MetadataTarget, nullptr, FDialogueStoryProgress(), Provider, nullptr));
	Metadata.Availability = EDialogueBlockAvailability::Active;
	Metadata.StoryScope.Mode = EDialogueStoryScope::Unassigned;
	TestFalse(TEXT("Unassigned excluded"), UDialogueMetadataLibrary::IsEligible(Metadata, TAG_MetadataTarget, nullptr, FDialogueStoryProgress(), Provider, nullptr));
	const FGuid Original = Metadata.Block.BlockId;
	TestFalse(TEXT("Malformed rejected"), FDialogueBlockMetadata::FromRegistryValue(TEXT("not metadata"), Metadata));
	TestEqual(TEXT("Failed read preserves output"), Metadata.Block.BlockId, Original);
	Graph->RequiredAllTags.AddTag(TAG_MetadataTarget);
	TestFalse(TEXT("Legacy conditions require explicit rewrite"), Graph->GetSelectionMetadata().IsValid());
	return true;
}
#endif
