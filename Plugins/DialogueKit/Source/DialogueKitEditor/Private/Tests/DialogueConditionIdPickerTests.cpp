#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueStateKeyCustomization.h"
#include "DialogueSettings.h"
#include "DialogueGraph.h"
#include "Samples/DialogueSampleDefinitions.h"
#include "Editor.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "IDetailsView.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "IPropertyUtilities.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "PackageTools.h"

namespace
{
// 실제 Graph의 property handle을 확보하되 Key 자체는 등록된 제품 커스터마이징을 사용한다.
class FPickerTestDetails : public IDetailCustomization
{
public:
	TSharedPtr<IPropertyHandle> Key;
	TSharedPtr<IPropertyUtilities> Utilities;

	// 중첩 Key만 펼쳐 자동 검사 창에서 실제 선택기 위젯을 생성한다.
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override
	{
		Utilities = Builder.GetPropertyUtilities();
		TSharedRef<IPropertyHandle> Entry = Builder.GetProperty(TEXT("EntryConditions"));
		const TSharedPtr<IPropertyHandle> Array = Entry->GetChildHandle(TEXT("RequiredConditions"));
		Key = Array->AsArray()->GetElement(0)->GetChildHandle(TEXT("Key"));
		Builder.HideProperty(Entry);
		Builder.EditCategory(TEXT("PickerTest")).AddProperty(Key).ShouldAutoExpand(true);
	}
};

// 화면 없는 Slate 구성 검사이며 실제 마우스 조작 검증과 구분한다.
bool ContainsPickerText(const TSharedRef<SWidget>& Widget, const FString& Text)
{
	if (Widget->GetType() == FName(TEXT("STextBlock")) && StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Text) return true;
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		if (ContainsPickerText(Children->GetChildAt(Index), Text)) return true;
	}
	return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueConditionIdOptionsTest, "DialogueKit.Editor.ConditionIdPicker.Options", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 원본 변환·필터·검색·중복 거부·실패 원자성을 검증한다.
bool FDialogueConditionIdOptionsTest::RunTest(const FString& Parameters)
{
	FDialogueDefinitionOptions Options;
	if (!TestTrue(TEXT("Project configured sample source loads"), Options.LoadConfigured())) return false;
	const FProperty* SourceProperty = UDialogueSettings::StaticClass()->FindPropertyByName(TEXT("ConditionDefinitionSource"));
	const TArray<const UClass*> Allowed = PropertyCustomizationHelpers::GetClassesFromMetadataString(SourceProperty->GetMetaData(TEXT("AllowedClasses")));
	TestTrue(TEXT("Actual asset picker metadata permits native catalog"), Allowed.Contains(UDialogueSampleDefinitionCatalog::StaticClass()));
	TestFalse(TEXT("Raw DataTable is not a definition adapter"), Allowed.Contains(UDataTable::StaticClass()));
	TestEqual(TEXT("Sample definitions"), Options.Definitions.Num(), 14);
	TestEqual(TEXT("Count has only Player"), Options.GetOptions(EDialogueStateKind::Count, NAME_None, true).Num(), 1);
	TestEqual(TEXT("Value has two NPCs and Player"), Options.GetOptions(EDialogueStateKind::Value, NAME_None, true).Num(), 3);
	const auto Items = Options.GetOptions(EDialogueStateKind::Count, TEXT("Player"), false);
	TestEqual(TEXT("Six items"), Items.Num(), 6);
	TestEqual(TEXT("Six skills"), Options.GetOptions(EDialogueStateKind::Value, TEXT("Player"), false).Num(), 6);
	const auto Npc = Options.GetOptions(EDialogueStateKind::Value, TEXT("NPC.Merchant"), false);
	if (TestEqual(TEXT("NPC has affection only"), Npc.Num(), 1)) TestEqual(TEXT("NPC item ID"), Npc[0].Id, FName(TEXT("Affection")));
	TestEqual(TEXT("Unknown target empty"), Options.GetOptions(EDialogueStateKind::Count, TEXT("Unknown"), false).Num(), 0);
	TestEqual(TEXT("No invented tag targets"), Options.GetOptions(EDialogueStateKind::Tag, NAME_None, true).Num(), 0);
	if (!Items.IsEmpty())
	{
		TestTrue(TEXT("Search name"), FDialogueDefinitionOptions::Matches(Items[0], Items[0].Label.ToString()));
		TestTrue(TEXT("Search case insensitive ID"), FDialogueDefinitionOptions::Matches(Items[0], Items[0].Id.ToString().ToLower()));
		TestFalse(TEXT("Search miss"), FDialogueDefinitionOptions::Matches(Items[0], TEXT("nonexistent id text")));
	}
	TArray<FDialogueStateDefinition> Definitions = Options.Definitions;
	const FDialogueStateDefinition Duplicate = Definitions[0];
	Definitions.Add(Duplicate);
	TestFalse(TEXT("Duplicate rejected"), Options.SetDefinitions(Definitions));
	TestTrue(TEXT("No partial rows after error"), Options.Definitions.IsEmpty());
	TestFalse(TEXT("Error is visible"), Options.Error.IsEmpty());
	Definitions.SetNum(1);
	Definitions[0].Key.TargetId = NAME_None;
	TestTrue(TEXT("Global target supported"), Options.SetDefinitions(Definitions));
	TestTrue(TEXT("Global target explicit label"), Options.GetOptions(Definitions[0].Key.Kind, NAME_None, true)[0].Label.ToString().Contains(TEXT("None")));
	Definitions[0].DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Missing label rejected"), Options.SetDefinitions(Definitions));
	TestFalse(TEXT("Wrong source class rejected"), Options.ReadSource(GetTransientPackage()));
	TestFalse(TEXT("Missing source rejected"), Options.ReadSource(nullptr));
	TestTrue(TEXT("Normal empty source distinguished"), Options.SetDefinitions({}));
	TestTrue(TEXT("Normal empty has no source error"), Options.Error.IsEmpty());
	const TSoftObjectPtr<UObject> Previous = GetDefault<UDialogueSettings>()->ConditionDefinitionSource;
	GetMutableDefault<UDialogueSettings>()->ConditionDefinitionSource.Reset();
	TestFalse(TEXT("Unconfigured source rejected"), Options.LoadConfigured());
	GetMutableDefault<UDialogueSettings>()->ConditionDefinitionSource = Previous;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueConditionIdDetailsTest, "DialogueKit.Editor.ConditionIdPicker.DetailsAndPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 property handle 선택·잠금·Undo/Redo·기존 값 보존과 새 파일 왕복을 검증한다.
bool FDialogueConditionIdDetailsTest::RunTest(const FString& Parameters)
{
	FDialogueDefinitionOptions Options;
	if (!TestTrue(TEXT("Source loads"), Options.LoadConfigured())) return false;
	TStrongObjectPtr<UDialogueGraph> Graph(NewObject<UDialogueGraph>(GetTransientPackage(), NAME_None, RF_Transactional));
	Graph->EntryConditions.RequiredConditions.AddDefaulted();
	FDialogueStateKey& Initial = Graph->EntryConditions.RequiredConditions[0].Key;
	Initial.Kind = EDialogueStateKind::Count;
	Initial.TargetId = TEXT("LegacyTarget");
	Initial.ItemId = TEXT("LegacyItem");
	TSharedRef<FPickerTestDetails> Capture = MakeShared<FPickerTestDetails>();
	FDetailsViewArgs ViewArgs;
	TSharedRef<IDetailsView> View = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreateDetailView(ViewArgs);
	View->RegisterInstancedCustomPropertyLayout(UDialogueGraph::StaticClass(), FOnGetDetailCustomizationInstance::CreateLambda([Capture]() { return Capture; }));
	View->SetObject(Graph.Get());
	TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Condition ID Picker Automation"))).ClientSize(FVector2D(1000, 1000))[View];
	FSlateApplication::Get().AddWindow(Window);
	FSlateApplication::Get().Tick();
	FSlateApplication::Get().Tick();
	TestTrue(TEXT("Actual Key details has refresh"), ContainsPickerText(View, TEXT("정의 목록 새로고침")));
	TestTrue(TEXT("Actual Key details has advanced manual group"), ContainsPickerText(View, TEXT("고급: ID 직접 입력")));
	TestEqual(TEXT("Opening details preserves unknown Target"), Graph->EntryConditions.RequiredConditions[0].Key.TargetId, FName(TEXT("LegacyTarget")));
	TestEqual(TEXT("Opening details preserves unknown Item"), Graph->EntryConditions.RequiredConditions[0].Key.ItemId, FName(TEXT("LegacyItem")));
	const TSharedRef<IPropertyHandle> Key = Capture->Key.ToSharedRef();
	// 참조는 Undo 시 컨테이너 재할당될 수 있어 매번 객체에서 읽는다.
	auto Apply = [&](bool bTarget, FName Id)
	{
		return FDialogueStateKeyCustomization::ApplySelection(Key, Options, bTarget, Id, Capture->Utilities->IsPropertyEditingEnabled());
	};
	TestTrue(TEXT("Select Player"), Apply(true, TEXT("Player")) == FPropertyAccess::Success);
	TestEqual(TEXT("Target selection preserves legacy Item"), Graph->EntryConditions.RequiredConditions[0].Key.ItemId, FName(TEXT("LegacyItem")));
	TestTrue(TEXT("One Undo restores Target"), GEditor->UndoTransaction());
	TestEqual(TEXT("Legacy Target restored"), Graph->EntryConditions.RequiredConditions[0].Key.TargetId, FName(TEXT("LegacyTarget")));
	TestTrue(TEXT("One Redo restores selection"), GEditor->RedoTransaction());
	TestEqual(TEXT("Player restored"), Graph->EntryConditions.RequiredConditions[0].Key.TargetId, FName(TEXT("Player")));
	TestTrue(TEXT("Select item"), Apply(false, TEXT("Item.HealthPotion")) == FPropertyAccess::Success);
	TestTrue(TEXT("Undo item"), GEditor->UndoTransaction());
	TestEqual(TEXT("Legacy Item restored"), Graph->EntryConditions.RequiredConditions[0].Key.ItemId, FName(TEXT("LegacyItem")));
	TestTrue(TEXT("Redo item"), GEditor->RedoTransaction());
	View->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda([]() { return false; }));
	TestTrue(TEXT("Details editing lock rejects callback"), Apply(true, TEXT("Player")) == FPropertyAccess::Fail);
	View->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda([]() { return true; }));
	{
		const bool bPreviousSimulating = GEditor->bIsSimulatingInEditor;
		GEditor->bIsSimulatingInEditor = true;
		TestTrue(TEXT("PIE simulation lock rejects callback"), Apply(false, TEXT("Item.HealthPotion")) == FPropertyAccess::Fail);
		GEditor->bIsSimulatingInEditor = bPreviousSimulating;
	}
	TestTrue(TEXT("Unknown option rejected"), Apply(false, TEXT("Unknown")) == FPropertyAccess::Fail);
	TestTrue(TEXT("Wrong kind item rejected"), Apply(false, TEXT("Skill.Firebolt")) == FPropertyAccess::Fail);
	TestTrue(TEXT("Change Kind through property"), Key->GetChildHandle(TEXT("Kind"))->SetValue(uint8(EDialogueStateKind::Value)) == FPropertyAccess::Success);
	TestEqual(TEXT("Kind change preserves Item"), Graph->EntryConditions.RequiredConditions[0].Key.ItemId, FName(TEXT("Item.HealthPotion")));
	TestTrue(TEXT("Select NPC"), Apply(true, TEXT("NPC.Merchant")) == FPropertyAccess::Success);
	TestEqual(TEXT("NPC change preserves old Item"), Graph->EntryConditions.RequiredConditions[0].Key.ItemId, FName(TEXT("Item.HealthPotion")));
	TestTrue(TEXT("Select Affection"), Apply(false, TEXT("Affection")) == FPropertyAccess::Success);
	const FDialogueStateKey Expected = Graph->EntryConditions.RequiredConditions[0].Key;
	Options.ReadSource(nullptr);
	TestTrue(TEXT("Source error rejects selection"), Apply(false, TEXT("Affection")) == FPropertyAccess::Fail);
	TestTrue(TEXT("Source error preserves key"), Graph->EntryConditions.RequiredConditions[0].Key == Expected);
	Options.LoadConfigured();
	TStrongObjectPtr<UDialogueGraph> Other(NewObject<UDialogueGraph>(GetTransientPackage(), NAME_None, RF_Transactional));
	Other->EntryConditions = Graph->EntryConditions;
	Other->EntryConditions.RequiredConditions[0].Key.Kind = EDialogueStateKind::Count;
	const TArray<UObject*> Selected = {Graph.Get(), Other.Get()};
	View->SetObjects(Selected);
	TestTrue(TEXT("Mixed Kind cannot choose Target"), FDialogueStateKeyCustomization::ApplySelection(Capture->Key.ToSharedRef(), Options, true, TEXT("Player"), true) == FPropertyAccess::Fail);
	Capture->Key->GetChildHandle(TEXT("Kind"))->SetValue(uint8(EDialogueStateKind::Value));
	Other->EntryConditions.RequiredConditions[0].Key.TargetId = TEXT("Player");
	View->ForceRefresh();
	TestTrue(TEXT("Mixed Target cannot choose Item"), FDialogueStateKeyCustomization::ApplySelection(Capture->Key.ToSharedRef(), Options, false, TEXT("Affection"), true) == FPropertyAccess::Fail);
	TestTrue(TEXT("Mixed edit preserves first key"), Graph->EntryConditions.RequiredConditions[0].Key == Expected);
	View->SetObject(nullptr);
	FSlateApplication::Get().RequestDestroyWindow(Window);
	FSlateApplication::Get().Tick();

	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialoguePickerTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/ConditionPickerTests") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	UPackage* Package = CreatePackage(*(Mount + TEXT("Graph")));
	auto* Asset = NewObject<UDialogueGraph>(Package, TEXT("Graph"), RF_Public | RF_Standalone);
	Asset->EntryConditions = Graph->EntryConditions;
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (!TestTrue(TEXT("Save selected key"), UPackage::SavePackage(Package, Asset, *(Folder + TEXT("Graph.uasset")), Args))) return false;
	TArray<UPackage*> Packages = {Package};
	if (TestTrue(TEXT("Unload saved graph"), UPackageTools::UnloadPackages(Packages)))
	{
		UPackage* Reloaded = LoadPackage(nullptr, *(Mount + TEXT("Graph")), LOAD_None);
		const auto* Loaded = Reloaded ? FindObject<UDialogueGraph>(Reloaded, TEXT("Graph")) : nullptr;
		if (TestNotNull(TEXT("Reload selected key"), Loaded))
		{
			if (TestEqual(TEXT("Condition retained"), Loaded->EntryConditions.RequiredConditions.Num(), 1))
			{
				TestTrue(TEXT("Exact selected key retained"), Loaded->EntryConditions.RequiredConditions[0].Key == Expected);
			}
		}
	}
	return true;
}
#endif
