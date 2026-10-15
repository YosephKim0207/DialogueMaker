#include "DialogueStoryEditor.h"
#include "DialogueStoryDefinition.h"
#include "DialogueSettings.h"
#include "DialogueGraph.h"
#include "Editor.h"
#include "EditorUndoClient.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "ScopedTransaction.h"
#include "Misc/MessageDialog.h"
#include "UObject/StrongObjectPtr.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SWindow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"

class SDialogueStoryboard : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SDialogueStoryboard) {}
	SLATE_END_ARGS()

	// 정의 에셋은 저작 데이터일 뿐 게임 진행 상태를 변경하지 않는다.
	void Construct(const FArguments& Args)
	{
		Definition.Reset(GetDefault<UDialogueSettings>()->StoryDefinition.LoadSynchronous());
		FDetailsViewArgs DetailsArgs;
		DetailsArgs.bAllowSearch = true;
		Details = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreateDetailView(DetailsArgs);
		GEditor->RegisterForUndo(this);
		ChildSlot[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)[SNew(SObjectPropertyEntryBox).AllowedClass(UDialogueStoryDefinition::StaticClass())
					.ObjectPath_Lambda([this]() { return Definition.IsValid() ? Definition->GetPathName() : FString(); })
					.OnObjectChanged_Lambda([this](const FAssetData& Asset)
					{
						Definition.Reset(Cast<UDialogueStoryDefinition>(Asset.GetAsset()));
						auto* Settings = GetMutableDefault<UDialogueSettings>();
						Settings->StoryDefinition = Definition.Get();
						Settings->SaveConfig();
						ChapterId.Invalidate(); SceneId.Invalidate(); Selected.Reset(); Rebuild();
					})]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("새로고침"))).OnClicked_Lambda([this]() { Rebuild(); return FReply::Handled(); })]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("정의 에셋 열기/저장"))).OnClicked_Lambda([this]()
				{
					if (Definition.IsValid()) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Definition.Get());
					return FReply::Handled();
				})]]
			+ SVerticalBox::Slot().FillHeight(1)[SNew(SSplitter)
				+ SSplitter::Slot().Value(.2f)[SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Chapters, SVerticalBox)]]
				+ SSplitter::Slot().Value(.45f)[SAssignNew(Center, SVerticalBox)]
				+ SSplitter::Slot().Value(.35f)[SAssignNew(Right, SVerticalBox)]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("정의와 Block 변경은 각 에셋에서 저장하세요. 소속 이동/정의 제거는 Undo 가능, 에셋 물리 삭제는 Undo로 복구할 수 없습니다.")))]];
		Rebuild();
	}

	virtual ~SDialogueStoryboard() override
	{
		if (GEditor) GEditor->UnregisterForUndo(this);
	}
	virtual void PostUndo(bool bSuccess) override { if (bSuccess) Rebuild(); }
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

private:
	TStrongObjectPtr<UDialogueStoryDefinition> Definition;
	TStrongObjectPtr<UDialogueGraph> SelectedGraph;
	TSharedPtr<SVerticalBox> Chapters, Center, Right;
	TSharedPtr<IDetailsView> Details;
	FGuid ChapterId, SceneId;
	bool bUnrestricted = false;
	TSet<FSoftObjectPath> Selected;
	FString Search;

	// 두 단계 확인은 한국어로 명시하며 첫 취소에서는 어떠한 변경도 하지 않는다.
	bool ConfirmRemoval(const FString& Name, bool bScene, FGuid Id)
	{
		FString Affected;
		if (Definition.IsValid())
		{
			if (bScene) { if (const auto* Item = Definition->FindScene(Id)) Affected = TEXT(" : ") + Item->DisplayName.ToString(); }
			else if (const auto* Item = Definition->FindChapter(Id)) Affected = TEXT(" : ") + Item->DisplayName.ToString();
		}
		TSet<FGuid> SceneIds;
		if (bScene) SceneIds.Add(Id);
		else if (Definition.IsValid()) for (const auto& Scene : Definition->Scenes) if (Scene.ChapterId == Id)
		{
			SceneIds.Add(Scene.SceneId);
			Affected += TEXT("\nScene: ") + Scene.DisplayName.ToString();
		}
		for (const auto& Row : FDialogueStoryEditor::CollectBlocks())
		{
			const auto& Scope = Row.Metadata.StoryScope;
			if (Row.bMetadataReadable && ((Scope.Mode == EDialogueStoryScope::Scene && SceneIds.Contains(Scope.SceneId)) ||
				(bScene == false && Scope.Mode == EDialogueStoryScope::Chapter && Scope.ChapterId == Id)))
			{
				Affected += TEXT("\nBlock: ") + Row.Asset.GetObjectPathString();
			}
		}
		if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(Name + TEXT(" 대상만 제거하시겠습니까? 아래 하위 Scene/Block은 삭제하지 않고 소속 없음으로 보존합니다.\n") + Affected)) != EAppReturnType::Yes) return false;
		return FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(TEXT("정말 제거하시겠습니까? 하위 항목은 소속 없음에서 다시 배치할 수 있습니다."))) == EAppReturnType::Yes;
	}

	// 선택 식별자로 매번 원본을 찾으며 배열 포인터를 Slate 콜백에 보관하지 않는다.
	void ShowDefinitionDetails(bool bScene)
	{
		Right->ClearChildren();
		SelectedGraph.Reset();
		Details->SetObject(nullptr);
		if (Definition.IsValid() == false) return;
		const FGuid Id = bScene ? SceneId : ChapterId;
		auto Read = [this, Id, bScene](int32 Field) -> FText
		{
			if (Definition.IsValid() == false) return FText();
			if (bScene)
			{
				if (const auto* Item = Definition->FindScene(Id)) return Field == 0 ? Item->DisplayName : Field == 1 ? Item->Description : FText::AsNumber(Item->SortOrder);
			}
			else if (const auto* Item = Definition->FindChapter(Id)) return Field == 0 ? Item->DisplayName : Field == 1 ? Item->Description : FText::AsNumber(Item->SortOrder);
			return FText();
		};
		Right->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString((bScene ? TEXT("Scene ID: ") : TEXT("Chapter ID: ")) + Id.ToString()))];
		const TCHAR* Labels[] = { TEXT("이름"), TEXT("설명"), TEXT("정렬 순서") };
		for (int32 Field = 0; Field < 3; ++Field)
		{
			Right->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Labels[Field]))];
			Right->AddSlot().AutoHeight()[SNew(SEditableTextBox).Text_Lambda([Read, Field]() { return Read(Field); })
				.OnTextCommitted_Lambda([this, Id, bScene, Field](const FText& Value, ETextCommit::Type Commit)
				{
					if (Commit == ETextCommit::OnCleared || Definition.IsValid() == false) return;
					int32 Order = 0;
					if (Field == 2 && LexTryParseString(Order, *Value.ToString()) == false) return;
					const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "EditStoryDefinition", "스토리 정의 편집"));
					Definition->Modify();
					if (bScene)
					{
						for (auto& Item : Definition->Scenes) if (Item.SceneId == Id)
						{
							if (Field == 0) Item.DisplayName = Value; else if (Field == 1) Item.Description = Value; else Item.SortOrder = Order;
						}
					}
					else for (auto& Item : Definition->Chapters) if (Item.ChapterId == Id)
					{
						if (Field == 0) Item.DisplayName = Value; else if (Field == 1) Item.Description = Value; else Item.SortOrder = Order;
					}
					Definition->MarkPackageDirty();
				})];
		}
		if (bScene)
		{
			Right->AddSlot().AutoHeight()[SNew(SComboButton).OnGetMenuContent_Lambda([this, Id]()
			{
				FMenuBuilder Menu(true, nullptr);
				TWeakPtr<SDialogueStoryboard> WeakThis = SharedThis(this);
				auto Add = [WeakThis, Id, &Menu](FGuid Parent, FText Name)
				{
					Menu.AddMenuEntry(Name, FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([WeakThis, Id, Parent]()
					{
						if (const auto Self = WeakThis.Pin())
						{
							if (FDialogueStoryEditor::MoveScene(Self->Definition.Get(), Id, Parent))
							{
								Self->ChapterId = Parent;
								Self->Rebuild();
							}
						}
					})));
				};
				Add(FGuid(), FText::FromString(TEXT("소속 없음")));
				if (Definition.IsValid()) for (const auto& Chapter : Definition->Chapters) Add(Chapter.ChapterId, Chapter.DisplayName);
				return Menu.MakeWidget();
			}).ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("Scene을 다른 Chapter로 이동")))]];
		}
		Right->AddSlot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("제거"))).OnClicked_Lambda([this, Id, bScene]()
		{
			if (ConfirmRemoval(bScene ? TEXT("Scene") : TEXT("Chapter"), bScene, Id))
			{
				if (bScene) FDialogueStoryEditor::RemoveScene(Definition.Get(), Id); else FDialogueStoryEditor::RemoveChapter(Definition.Get(), Id);
				SceneId.Invalidate(); ChapterId.Invalidate(); bUnrestricted = false; Rebuild();
			}
			return FReply::Handled();
		})];
	}

	// 확인 창에 표시했던 정확한 선택 집합만 처리한다. 삭제 중 선택 변화는 반영하지 않는다.
	void DeleteSelected()
	{
		TArray<FAssetData> Assets;
		FString Names;
		for (const auto& Row : FDialogueStoryEditor::CollectBlocks())
		{
			if (Selected.Contains(Row.Asset.GetSoftObjectPath()) == false) continue;
			if (Row.bMetadataReadable == false || FDialogueStoryEditor::IsUnassigned(Row.Metadata.StoryScope, Definition.Get()) == false)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("선택에 소속 있는 Block 또는 읽을 수 없는 에셋이 포함되어 있습니다.")));
				return;
			}
			Assets.Add(Row.Asset); Names += Row.Asset.GetObjectPathString() + TEXT("\n");
		}
		if (Assets.IsEmpty() || Assets.Num() != Selected.Num()) return;
		if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(TEXT("다음 Block 에셋을 물리 삭제하시겠습니까? Undo로 복구할 수 없습니다.\n") + Names)) != EAppReturnType::Yes) return;
		if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(TEXT("선택한 모든 Block이 출시 전 콘텐츠임을 확인했습니까? 출시된 Block은 삭제하지 말고 RestoreOnly로 변경해야 합니다. 정말 모두 삭제하려면 예를 누르세요.\n") + Names)) != EAppReturnType::Yes) return;
		Details->SetObject(nullptr);
		SelectedGraph.Reset();
		FText Result;
		FDialogueStoryEditor::DeleteUnassigned(Definition.Get(), Assets, Result);
		FMessageDialog::Open(EAppMsgType::Ok, Result);
		Selected.Reset(); Rebuild();
	}

	// 선택 카테고리는 UI 범위이며 게임의 현재 Chapter/Scene을 갱신하지 않는다.
	void Rebuild()
	{
		Chapters->ClearChildren(); Center->ClearChildren(); Right->ClearChildren();
		Details->SetObject(nullptr); SelectedGraph.Reset();
		if (Definition.IsValid() == false)
		{
			Center->AddSlot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("콘텐츠 브라우저에서 DialogueStoryDefinition 데이터 에셋을 만들고 위에서 선택하세요.")))];
			return;
		}
		Chapters->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Chapter")))];
		auto AddCategory = [this](const FText& Label, FGuid Id, bool bAll)
		{
			Chapters->AddSlot().AutoHeight()[SNew(SButton).Text(Label).OnClicked_Lambda([this, Id, bAll]()
			{
				ChapterId = Id; SceneId.Invalidate(); bUnrestricted = bAll; Rebuild();
				if (Id.IsValid()) ShowDefinitionDetails(false);
				return FReply::Handled();
			})];
		};
		AddCategory(FText::FromString(TEXT("소속 없음 / 삭제된 소속")), FGuid(), false);
		AddCategory(FText::FromString(TEXT("제한 없음")), FGuid(), true);
		auto ChapterRows = Definition->Chapters;
		ChapterRows.Sort([](const auto& A, const auto& B) { return A.SortOrder < B.SortOrder; });
		for (const auto& Chapter : ChapterRows) AddCategory(Chapter.DisplayName, Chapter.ChapterId, false);
		Chapters->AddSlot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("+ Chapter"))).OnClicked_Lambda([this]()
		{
			TArray<FString> Issues;
			if (Definition->ValidateDefinitions(Issues))
			{
				const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "AddChapter", "Chapter 추가"));
				Definition->Modify(); ChapterId = Definition->AddChapter(FText::FromString(TEXT("새 Chapter"))); Definition->MarkPackageDirty();
				bUnrestricted = false; SceneId.Invalidate(); Rebuild(); ShowDefinitionDetails(false);
			}
			return FReply::Handled();
		})];
		Center->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Scene 카드 — 선택하면 아래 Block 목록이 바뀝니다")))];
		TSharedRef<SScrollBox> SceneList = SNew(SScrollBox).Orientation(Orient_Horizontal);
		const auto Rows = FDialogueStoryEditor::CollectBlocks();
		auto Scenes = Definition->Scenes;
		Scenes.Sort([](const auto& A, const auto& B) { return A.SortOrder < B.SortOrder; });
		for (const auto& Scene : Scenes)
		{
			if (bUnrestricted || Scene.ChapterId != ChapterId) continue;
			const int32 Count = Rows.FilterByPredicate([&Scene](const FDialogueStoryBlock& Row) { return Row.bMetadataReadable && Row.Metadata.StoryScope.Mode == EDialogueStoryScope::Scene && Row.Metadata.StoryScope.SceneId == Scene.SceneId; }).Num();
			SceneList->AddSlot()[SNew(SButton).Text(FText::FromString(FString::Printf(TEXT("%s (%d Block)"), *Scene.DisplayName.ToString(), Count))).OnClicked_Lambda([this, Id = Scene.SceneId]()
			{
				SceneId = Id; Rebuild(); ShowDefinitionDetails(true); return FReply::Handled();
			})];
		}
		Center->AddSlot().AutoHeight().MaxHeight(90)[SceneList];
		Center->AddSlot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("+ Scene"))).IsEnabled(bUnrestricted == false).OnClicked_Lambda([this]()
		{
			TArray<FString> Issues;
			if (Definition->ValidateDefinitions(Issues))
			{
				const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "AddScene", "Scene 추가"));
				Definition->Modify(); SceneId = Definition->AddScene(ChapterId, FText::FromString(TEXT("새 Scene"))); Definition->MarkPackageDirty(); Rebuild(); ShowDefinitionDetails(true);
			}
			return FReply::Handled();
		})];
		FDialogueStoryScope Destination;
		if (bUnrestricted) Destination.Mode = EDialogueStoryScope::Unrestricted;
		else if (SceneId.IsValid()) { Destination.Mode = EDialogueStoryScope::Scene; Destination.SceneId = SceneId; }
		else if (ChapterId.IsValid()) { Destination.Mode = EDialogueStoryScope::Chapter; Destination.ChapterId = ChapterId; }
		Center->AddSlot().AutoHeight()[SNew(SButton).Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("선택한 %d개 Block을 현재 소속으로 이동"), Selected.Num())); })
			.OnClicked_Lambda([this, Destination]()
			{
				TArray<FAssetData> Assets;
				for (const auto& Row : FDialogueStoryEditor::CollectBlocks()) if (Selected.Contains(Row.Asset.GetSoftObjectPath())) Assets.Add(Row.Asset);
				FString Names;
				for (const auto& Asset : Assets) Names += Asset.GetObjectPathString() + TEXT("\n");
				if (Assets.IsEmpty() || Assets.Num() != Selected.Num()) return FReply::Handled();
				if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(TEXT("다른 카테고리에서 선택한 Block도 포함됩니다. 다음 정확한 목록을 현재 소속으로 이동하시겠습니까?\n") + Names)) != EAppReturnType::Yes) return FReply::Handled();
				if (Assets.Num() == Selected.Num() && FDialogueStoryEditor::MoveBlocks(Definition.Get(), Assets, Destination)) { Selected.Reset(); Rebuild(); }
				else FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("이동 실패: 선택/정의/대상 소속을 확인하세요.")));
				return FReply::Handled();
			})];
		Center->AddSlot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("소속 없는 선택 Block 삭제"))).OnClicked_Lambda([this]() { DeleteSelected(); return FReply::Handled(); })];
		Center->AddSlot().AutoHeight()[SNew(SSearchBox).InitialText(FText::FromString(Search)).HintText(FText::FromString(TEXT("Block 이름 또는 대상 태그 검색"))).OnTextChanged_Lambda([this](const FText& Text) { Search = Text.ToString(); })];
		TSharedRef<SScrollBox> Blocks = SNew(SScrollBox);
		int32 BlockCount = 0;
		for (const auto& Row : Rows)
		{
			const auto& Scope = Row.Metadata.StoryScope;
			bool bShow = false;
			if (bUnrestricted) bShow = Row.bMetadataReadable && Scope.Mode == EDialogueStoryScope::Unrestricted;
			else if (SceneId.IsValid()) bShow = Row.bMetadataReadable && Scope.Mode == EDialogueStoryScope::Scene && Scope.SceneId == SceneId;
			else if (ChapterId.IsValid()) bShow = Row.bMetadataReadable && Scope.Mode == EDialogueStoryScope::Chapter && Scope.ChapterId == ChapterId;
			else bShow = Row.bMetadataReadable == false || FDialogueStoryEditor::IsUnassigned(Scope, Definition.Get());
			if (bShow == false) continue;
			++BlockCount;
			const FString Label = Row.Asset.AssetName.ToString() + TEXT(" | ") + Row.Metadata.TargetTag.ToString() + FString::Printf(TEXT(" | 우선순위 %d | %s | %s"), Row.Metadata.Priority, Row.Metadata.bDefault ? TEXT("기본") : Row.Metadata.bTopPriority ? TEXT("최우선") : TEXT("일반"), Row.Metadata.Availability == EDialogueBlockAvailability::RestoreOnly ? TEXT("복원 전용") : TEXT("신규 사용")) +
				(Row.bMetadataReadable == false ? TEXT(" | 메타데이터 확인 필요") : Scope.Mode != EDialogueStoryScope::Unassigned && FDialogueStoryEditor::IsUnassigned(Scope, Definition.Get()) ? TEXT(" | 삭제된 소속") : TEXT(""));
			Blocks->AddSlot()[SNew(SHorizontalBox).Visibility_Lambda([this, Label]() { return Search.IsEmpty() || Label.Contains(Search) ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SCheckBox).IsChecked_Lambda([this, Path = Row.Asset.GetSoftObjectPath()]() { return Selected.Contains(Path) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this, Path = Row.Asset.GetSoftObjectPath()](ECheckBoxState State) { if (State == ECheckBoxState::Checked) Selected.Add(Path); else Selected.Remove(Path); })]
				 + SHorizontalBox::Slot().FillWidth(1)[SNew(SBorder)
					.OnMouseButtonDown_Lambda([this, Asset = Row.Asset](const FGeometry&, const FPointerEvent& Event)
					{
						if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
						SelectedGraph.Reset(Cast<UDialogueGraph>(Asset.GetAsset()));
						Right->ClearChildren(); Details->SetObject(SelectedGraph.Get());
						Right->AddSlot().FillHeight(1)[Details.ToSharedRef()];
						return FReply::Handled();
					})
					.OnMouseDoubleClick_Lambda([Asset = Row.Asset](const FGeometry&, const FPointerEvent& Event)
					{
						if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
						if (UObject* Object = Asset.GetAsset()) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
						return FReply::Handled();
					})[SNew(STextBlock).Text(FText::FromString(Label)).ToolTipText(FText::FromString(Row.Asset.GetObjectPathString()))]]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("열기"))).OnClicked_Lambda([Asset = Row.Asset]()
				{
					if (UObject* Object = Asset.GetAsset()) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Object);
					return FReply::Handled();
				})]];
		}
		Center->AddSlot().FillHeight(1)[Blocks];
		const auto* Chapter = Definition->FindChapter(ChapterId);
		const auto* Scene = Definition->FindScene(SceneId);
		Center->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s / %s — %d Block"),
			Chapter ? *Chapter->DisplayName.ToString() : bUnrestricted ? TEXT("제한 없음") : TEXT("소속 없음"),
			Scene ? *Scene->DisplayName.ToString() : TEXT("전체 Scene 범위"), BlockCount)))];
	}
};

// 기존 창을 재사용하여 동일 정의를 중복된 편집 창에서 수정하지 않도록 한다.
void FDialogueStoryEditor::Open()
{
	static TWeakPtr<SWindow> Existing;
	if (const auto Window = Existing.Pin()) { Window->BringToFront(); return; }
	TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("DialogueMaker 스토리보드"))).ClientSize(FVector2D(1250, 760));
	Window->SetContent(SNew(SDialogueStoryboard));
	Existing = Window;
	FSlateApplication::Get().AddWindow(Window);
}
