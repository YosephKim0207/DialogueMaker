#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueStateKeyCustomization.h"
#include "DialogueBlockPreview.h"
#include "DialogueSampleStateProvider.h"
#include "Editor.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "PackageTools.h"

namespace
{
class FGroupTestDetails : public IDetailCustomization
{
public:
	TSharedPtr<IPropertyHandle> Entry;
	// 실제 등록된 조건 구조체 UI를 유지하면서 시험 창에서 펼친다.
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override
	{
		Entry = Builder.GetProperty(TEXT("EntryConditions"));
		Builder.HideProperty(Entry);
		Builder.EditCategory(TEXT("GroupTest")).AddProperty(Entry).ShouldAutoExpand(true);
	}
};

// 실제 Slate 구성에 새 그룹과 기존 조건 안내가 나타나는지 확인한다.
bool ContainsGroupText(const TSharedRef<SWidget>& Widget, const FString& Text)
{
	if (Widget->GetType() == FName(TEXT("STextBlock")) && StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString().Contains(Text)) return true;
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		if (ContainsGroupText(Children->GetChildAt(Index), Text)) return true;
	}
	return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueConditionGroupsEditorTest, "DialogueKit.Editor.ConditionGroups", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 새 배열의 실제 ID 선택·Undo와 유형/그룹 파일 왕복 및 설명 툴팁을 검증한다.
bool FDialogueConditionGroupsEditorTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UDialogueGraph> OldAsset(LoadObject<UDialogueGraph>(nullptr, TEXT("/DialogueKit/Sample/DialogueGraph/NewDataAsset.NewDataAsset")));
	if (TestNotNull(TEXT("Pre-change shipped asset loads"), OldAsset.Get()))
	{
		TestTrue(TEXT("Old tagged asset defaults new arrays empty"), OldAsset->EntryConditions.RequiredConditions.IsEmpty() && OldAsset->EntryConditions.AnyConditions.IsEmpty() && OldAsset->EntryConditions.BlockedConditions.IsEmpty());
	}
	TStrongObjectPtr<UDialogueGraph> Graph(NewObject<UDialogueGraph>(GetTransientPackage(), NAME_None, RF_Transactional));
	Graph->InitializeBlockIdentity();
	Graph->DialogueGraphType = EDialogueGraphType::Story;
	FDialogueCondition Atom;
	Atom.Key.Kind = EDialogueStateKind::Count;
	Atom.Key.TargetId = TEXT("Player");
	Atom.Key.ItemId = TEXT("Item.Old");
	Graph->EntryConditions.RequiredConditions = {Atom};
	Graph->EntryConditions.AnyConditions = {Atom};
	Graph->EntryConditions.BlockedConditions = {Atom};
	Graph->EntryConditions.Conditions = {Atom};
	FDetailsViewArgs Args;
	TSharedRef<IDetailsView> View = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreateDetailView(Args);
	TSharedRef<FGroupTestDetails> Capture = MakeShared<FGroupTestDetails>();
	View->RegisterInstancedCustomPropertyLayout(UDialogueGraph::StaticClass(), FOnGetDetailCustomizationInstance::CreateLambda([Capture]() { return Capture; }));
	View->SetObject(Graph.Get());
	TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Condition Groups Automation"))).ClientSize(FVector2D(1200, 1400))[View];
	FSlateApplication::Get().AddWindow(Window);
	FSlateApplication::Get().Tick();
	FSlateApplication::Get().Tick();
	for (const FString Text : {TEXT("필수 조건"), TEXT("대안 조건"), TEXT("금지 조건"), TEXT("기존 조건 1개도 함께 적용")})
	{
		TestTrue(*Text, ContainsGroupText(View, Text));
	}
	FDialogueDefinitionOptions Options;
	if (TestTrue(TEXT("Sample source"), Options.LoadConfigured()))
	{
		const auto Required = Capture->Entry->GetChildHandle(TEXT("RequiredConditions"));
		const auto Key = Required->AsArray()->GetElement(0)->GetChildHandle(TEXT("Key"));
		TestTrue(TEXT("New group ID selector writes"), FDialogueStateKeyCustomization::ApplySelection(Key.ToSharedRef(), Options, false, TEXT("Item.HealthPotion"), true) == FPropertyAccess::Success);
		TestTrue(TEXT("Undo new group edit"), GEditor->UndoTransaction());
		TestEqual(TEXT("Undo preserves old ID"), Graph->EntryConditions.RequiredConditions[0].Key.ItemId, Atom.Key.ItemId);
		TestTrue(TEXT("Redo new group edit"), GEditor->RedoTransaction());
		TestEqual(TEXT("Redo exact selected ID"), Graph->EntryConditions.RequiredConditions[0].Key.ItemId, FName(TEXT("Item.HealthPotion")));
	}
	for (FName Name : {FName(TEXT("RequiredConditions")), FName(TEXT("AnyConditions")), FName(TEXT("BlockedConditions")), FName(TEXT("Combine")), FName(TEXT("Conditions"))})
	{
		const auto Property = Capture->Entry->GetChildHandle(Name);
		TestFalse(TEXT("Actual field tooltip present"), Property->GetToolTipText().IsEmpty());
	}
	const FProperty* Type = UDialogueGraph::StaticClass()->FindPropertyByName(TEXT("DialogueGraphType"));
	TestTrue(TEXT("Type tooltip separates old path"), Type->GetMetaData(TEXT("ToolTip")).Contains(TEXT("BeginDialogue")));
	for (int64 Value : {0, 10, 100}) TestFalse(TEXT("Each type enum has tooltip"), StaticEnum<EDialogueGraphType>()->GetToolTipTextByIndex(StaticEnum<EDialogueGraphType>()->GetIndexByValue(Value)).IsEmpty());
	View->SetObject(nullptr);
	FSlateApplication::Get().RequestDestroyWindow(Window);
	FSlateApplication::Get().Tick();

	TStrongObjectPtr<UDialogueSampleStateProvider> Provider(NewObject<UDialogueSampleStateProvider>());
	FDialogueStateDefinition Definition;
	Definition.Key = Atom.Key;
	Provider->AddDefinition(Definition);
	Graph->EntryConditions.RequiredConditions[0] = Atom;
	FString Explanation;
	TestFalse(TEXT("True forbidden condition blocks preview"), FDialogueBlockPreview::CheckEntry(Graph.Get(), Provider.Get(), Explanation));
	for (const FString Label : {TEXT("[필수]"), TEXT("[대안]"), TEXT("[금지:"), TEXT("[기존]")}) TestTrue(TEXT("Preview explains every group"), Explanation.Contains(Label));

	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueGroupsTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/ConditionGroupsTests") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	UPackage* Package = CreatePackage(*(Mount + TEXT("Graph")));
	auto* Asset = NewObject<UDialogueGraph>(Package, TEXT("Graph"), RF_Public | RF_Standalone);
	Asset->InitializeBlockIdentity();
	Asset->EntryConditions = Graph->EntryConditions;
	Asset->DialogueGraphType = Graph->DialogueGraphType;
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!TestTrue(TEXT("Save new groups"), UPackage::SavePackage(Package, Asset, *(Folder + TEXT("Graph.uasset")), SaveArgs))) return false;
	TArray<UPackage*> Packages = {Package};
	if (TestTrue(TEXT("Unload groups"), UPackageTools::UnloadPackages(Packages)))
	{
		UPackage* Reloaded = LoadPackage(nullptr, *(Mount + TEXT("Graph")), LOAD_None);
		const auto* Loaded = Reloaded ? FindObject<UDialogueGraph>(Reloaded, TEXT("Graph")) : nullptr;
		if (TestNotNull(TEXT("Reload groups"), Loaded))
		{
			TestEqual(TEXT("Four arrays persisted"), Loaded->EntryConditions.GetAllConditions().Num(), 4);
			TestEqual(TEXT("Type persisted"), Loaded->DialogueGraphType, EDialogueGraphType::Story);
			TestFalse(TEXT("Reloaded preview same verdict"), FDialogueBlockPreview::CheckEntry(const_cast<UDialogueGraph*>(Loaded), Provider.Get(), Explanation));
		}
	}
	return true;
}
#endif
