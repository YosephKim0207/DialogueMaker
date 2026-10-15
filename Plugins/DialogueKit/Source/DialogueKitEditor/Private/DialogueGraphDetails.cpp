#include "DialogueGraphDetails.h"
#include "DialogueGraph.h"
#include "DialogueSettings.h"
#include "DialogueStoryDefinition.h"
#include "GameplayTagsManager.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "ScopedTransaction.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/AssetRegistryTagsContext.h"

// 선택 대상의 클래스 전용 Details를 생성한다.
TSharedRef<IDetailCustomization> FDialogueGraphDetails::MakeInstance()
{
	return MakeShared<FDialogueGraphDetails>();
}

// 루트 필터는 UI 상태이며 실제 타겟은 등록된 태그 하나만 저장한다.
void FDialogueGraphDetails::CustomizeDetails(IDetailLayoutBuilder& Builder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	Builder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1) return;
	TWeakObjectPtr<UDialogueGraph> Graph = Cast<UDialogueGraph>(Objects[0].Get());
	if (Graph.IsValid() == false) return;
	Builder.EditCategory("DialogueKit|Legacy 안내").AddCustomRow(FText::FromString(TEXT("구형 조건"))).WholeRowContent()
	[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("신규 Block 진입 조건은 Entry Conditions에서 편집합니다. 구형 Required All/Any/Blocked Tags가 남으면 수동 재작성 대상으로 신규 후보에서 제외됩니다. 내용을 옮긴 뒤 구형 태그를 비우세요. SpeakerID/ChapterID는 신규 대상·소속을 대신하지 않습니다.")))];
	Builder.HideProperty(GET_MEMBER_NAME_CHECKED(UDialogueGraph, DialogueTarget));
	Builder.HideProperty(GET_MEMBER_NAME_CHECKED(UDialogueGraph, StoryScope));
	TSharedRef<FString> Root = MakeShared<FString>();
	TSharedRef<FString> Search = MakeShared<FString>();
	auto DisplayName = [](FGameplayTag Tag)
	{
		if (const FText* Name = GetDefault<UDialogueSettings>()->TargetDisplayNames.Find(Tag))
		{
			return Name->ToString() + TEXT(" [") + Tag.ToString() + TEXT("]");
		}
		return Tag.ToString();
	};
	auto Update = [Graph]()
	{
		if (Graph.IsValid())
		{
			Graph->PostEditChange();
			Graph->MarkPackageDirty();
			FAssetRegistryModule::GetRegistry().AssetUpdateTags(Graph.Get(), EAssetRegistryTagsCaller::Fast);
		}
	};
	Builder.EditCategory("DialogueKit|Selection").AddCustomRow(FText::FromString(TEXT("대상 태그"))).WholeRowContent()
	[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(GetDefault<UDialogueSettings>()->TargetFieldLabel)]
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SComboButton).OnGetMenuContent_Lambda([Root]()
		{
			FGameplayTagContainer Tags;
			UGameplayTagsManager::Get().RequestAllGameplayTags(Tags, true);
			TSet<FString> Roots;
			for (const FGameplayTag& Tag : Tags) { FString Left, Right; Tag.ToString().Split(TEXT("."), &Left, &Right); Roots.Add(Left.IsEmpty() ? Tag.ToString() : Left); }
			FMenuBuilder Menu(true, nullptr);
			Menu.AddMenuEntry(FText::FromString(TEXT("전체")), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([Root]() { Root->Reset(); })));
			for (const FString& Name : Roots) Menu.AddMenuEntry(FText::FromString(Name), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([Root, Name]() { *Root = Name; })));
			return Menu.MakeWidget();
		}) .ButtonContent()[SNew(STextBlock).Text_Lambda([Root]() { return FText::FromString(Root->IsEmpty() ? TEXT("태그 분류: 전체") : *Root); })]]
		+ SHorizontalBox::Slot().FillWidth(1)[SNew(SComboButton).OnGetMenuContent_Lambda([Graph, Root, Search, Update, DisplayName]()
		{
			TSharedRef<SScrollBox> List = SNew(SScrollBox);
			FGameplayTagContainer Tags;
			UGameplayTagsManager::Get().RequestAllGameplayTags(Tags, true);
			for (const FGameplayTag& Tag : Tags)
			{
				const FString Name = Tag.ToString();
				const FString Label = DisplayName(Tag);
				if (Root->IsEmpty() == false && Name != *Root && Name.StartsWith(*Root + TEXT(".")) == false) continue;
				List->AddSlot()[SNew(SButton).Visibility_Lambda([Search, Label]() { return Search->IsEmpty() || Label.Contains(*Search) ? EVisibility::Visible : EVisibility::Collapsed; })
					.Text(FText::FromString(Label)).OnClicked_Lambda([Graph, Tag, Update]()
					{
						if (Graph.IsValid()) { const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "TargetTag", "Block 대상 태그 설정")); Graph->Modify(); Graph->DialogueTarget = Tag; Update(); }
						return FReply::Handled();
					})];
			}
			return SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(SSearchBox).OnTextChanged_Lambda([Search](const FText& Text) { *Search = Text.ToString(); })]
				+ SVerticalBox::Slot().MaxHeight(320)[List];
		}) .ButtonContent()[SNew(STextBlock).Text_Lambda([Graph, DisplayName]() { return Graph.IsValid() ? FText::FromString(DisplayName(Graph->DialogueTarget)) : FText(); })]]];
	Builder.EditCategory("DialogueKit|Story").AddCustomRow(FText::FromString(TEXT("소속"))).WholeRowContent()
	[SNew(SComboButton).OnGetMenuContent_Lambda([Graph, Update]()
	{
		FMenuBuilder Menu(true, nullptr);
		auto Add = [&Menu, Graph, Update](const FText& Label, const FDialogueStoryScope Scope)
		{
			Menu.AddMenuEntry(Label, FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([Graph, Scope, Update]()
			{
				if (Graph.IsValid()) { const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "StoryScope", "Block 소속 설정")); Graph->Modify(); Graph->StoryScope = Scope; Update(); }
			})));
		};
		FDialogueStoryScope Scope;
		Add(FText::FromString(TEXT("소속 없음")), Scope);
		Scope.Mode = EDialogueStoryScope::Unrestricted;
		Add(FText::FromString(TEXT("제한 없음")), Scope);
		if (const auto* Definition = GetDefault<UDialogueSettings>()->StoryDefinition.LoadSynchronous())
		{
			for (const auto& Chapter : Definition->Chapters)
			{
				Scope = FDialogueStoryScope(); Scope.Mode = EDialogueStoryScope::Chapter; Scope.ChapterId = Chapter.ChapterId;
				Add(FText::FromString(Chapter.DisplayName.ToString() + TEXT(" / 전체 Scene")), Scope);
				for (const auto& Scene : Definition->Scenes)
				{
					if (Scene.ChapterId != Chapter.ChapterId) continue;
					Scope = FDialogueStoryScope(); Scope.Mode = EDialogueStoryScope::Scene; Scope.SceneId = Scene.SceneId;
					Add(FText::FromString(Chapter.DisplayName.ToString() + TEXT(" / ") + Scene.DisplayName.ToString()), Scope);
				}
			}
		}
		return Menu.MakeWidget();
	}).ButtonContent()[SNew(STextBlock).Text_Lambda([Graph]()
	{
		if (Graph.IsValid() == false) return FText();
		const auto& Scope = Graph->StoryScope;
		if (Scope.Mode == EDialogueStoryScope::Unassigned) return FText::FromString(TEXT("소속 없음"));
		if (Scope.Mode == EDialogueStoryScope::Unrestricted) return FText::FromString(TEXT("제한 없음"));
		if (const auto* Definition = GetDefault<UDialogueSettings>()->StoryDefinition.LoadSynchronous())
		{
			if (Scope.Mode == EDialogueStoryScope::Chapter) { if (const auto* Chapter = Definition->FindChapter(Scope.ChapterId)) return Chapter->DisplayName; }
			if (Scope.Mode == EDialogueStoryScope::Scene) { if (const auto* Scene = Definition->FindScene(Scope.SceneId)) return Scene->DisplayName; }
		}
		return FText::FromString(TEXT("정의 미설정 또는 삭제된 소속"));
	})]];
}
