#include "DialogueStoryDefinitionDetails.h"
#include "DialogueStoryDefinition.h"
#include "DialogueStoryEditor.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// 정의 에셋 전용 Details를 생성한다.
TSharedRef<IDetailCustomization> FDialogueStoryDefinitionDetails::MakeInstance()
{
	return MakeShared<FDialogueStoryDefinitionDetails>();
}

// 재구성으로 사라진 Details가 Undo 알림을 받지 않도록 해제한다.
FDialogueStoryDefinitionDetails::~FDialogueStoryDefinitionDetails()
{
	if (bRegisteredUndo && GEditor) GEditor->UnregisterForUndo(this);
}

// 배열 길이와 ID가 바뀐 Undo 결과를 다음 프레임에 다시 구성한다.
void FDialogueStoryDefinitionDetails::PostUndo(bool bSuccess)
{
	if (bSuccess) if (const auto Pinned = Utilities.Pin()) Pinned->RequestForceRefresh();
}

// Redo도 같은 정의 자료에서 목록을 재구성한다.
void FDialogueStoryDefinitionDetails::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

// 원시 배열/부모 GUID 입력을 숨기고 명시적인 생성 및 이름 선택을 제공한다.
void FDialogueStoryDefinitionDetails::CustomizeDetails(IDetailLayoutBuilder& Builder)
{
	Utilities = Builder.GetPropertyUtilities();
	if (bRegisteredUndo == false && GEditor)
	{
		GEditor->RegisterForUndo(this);
		bRegisteredUndo = true;
	}
	const auto Chapters = Builder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogueStoryDefinition, Chapters));
	const auto Scenes = Builder.GetProperty(GET_MEMBER_NAME_CHECKED(UDialogueStoryDefinition, Scenes));
	Builder.HideProperty(Chapters);
	Builder.HideProperty(Scenes);
	auto& Category = Builder.EditCategory("DialogueKit|Story");
	TArray<TWeakObjectPtr<UObject>> Objects;
	Builder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1)
	{
		Category.AddCustomRow(FText::FromString(TEXT("스토리 정의"))).WholeRowContent()
		[SNew(STextBlock).Text(FText::FromString(TEXT("스토리 정의 에셋을 하나씩 열어 편집하세요.")))];
		return;
	}
	TWeakObjectPtr<UDialogueStoryDefinition> Definition = Cast<UDialogueStoryDefinition>(Objects[0].Get());
	if (Definition.IsValid() == false) return;
	const auto WeakUtilities = Utilities;
	auto Refresh = [WeakUtilities]()
	{
		if (const auto Pinned = WeakUtilities.Pin()) Pinned->RequestForceRefresh();
	};
	Category.AddCustomRow(FText::FromString(TEXT("안내"))).WholeRowContent()
	[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("아래 버튼으로 생성하면 고유 ID가 자동 발급됩니다. ID는 직접 수정하지 않습니다. 제거는 스토리보드에서 하위 참조를 확인한 뒤 수행하세요.")))];
	Category.AddCustomRow(FText::FromString(TEXT("정의 검사"))).WholeRowContent()
	[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Definition]()
	{
		TArray<FString> Issues;
		if (Definition.IsValid() == false || Definition->ValidateDefinitions(Issues)) return FText();
		return FText::FromString(TEXT("정의 오류: ") + FString::Join(Issues, TEXT(", ")) + TEXT(". 0 ID는 누락 ID 생성 후 소속을 지정하세요. 유효 중복 ID/삭제된 부모는 자동 보정하지 않습니다."));
	})];
	TSharedRef<SHorizontalBox> Buttons = SNew(SHorizontalBox);
	for (bool bScene : {false, true})
	{
		Buttons->AddSlot().AutoWidth().Padding(2)
		[SNew(SButton).Text(FText::FromString(bScene ? TEXT("+ Scene") : TEXT("+ Chapter")))
			.OnClicked_Lambda([Definition, bScene, Refresh]()
			{
				if (FDialogueStoryEditor::AddDefinitionItem(Definition.Get(), bScene).IsValid() == false)
				{
					FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("추가하지 못했습니다. 기존 정의의 누락/중복 ID 또는 잘못된 부모를 먼저 확인하세요.")));
				}
				Refresh();
				return FReply::Handled();
			})];
	}
	Buttons->AddSlot().AutoWidth().Padding(2)
	[SNew(SButton).Text(FText::FromString(TEXT("누락 ID 생성")))
		.OnClicked_Lambda([Definition, Refresh]()
		{
			FDialogueStoryEditor::AssignMissingIds(Definition.Get());
			Refresh();
			return FReply::Handled();
		})];
	Buttons->AddSlot().AutoWidth().Padding(2)
	[SNew(SButton).Text(FText::FromString(TEXT("목록 새로고침"))).OnClicked_Lambda([Refresh]() { Refresh(); return FReply::Handled(); })];
	Category.AddCustomRow(FText::FromString(TEXT("Chapter Scene 추가"))).WholeRowContent()[Buttons];

	// 일반 필드는 PropertyHandle로 편집하여 트랜잭션과 프로퍼티 알림을 유지한다.
	for (bool bScene : {false, true})
	{
		const auto Array = (bScene ? Scenes : Chapters)->AsArray();
		uint32 Count = 0;
		Array->GetNumElements(Count);
		for (uint32 Index = 0; Index < Count; ++Index)
		{
			const auto Element = Array->GetElement(Index);
			const FGuid Id = bScene ? Definition->Scenes[Index].SceneId : Definition->Chapters[Index].ChapterId;
			const FString Label = FString::Printf(TEXT("%s %u"), bScene ? TEXT("Scene") : TEXT("Chapter"), Index + 1);
			auto& Group = Category.AddGroup(FName(*Label), FText::FromString(Label), false, true);
			Group.AddWidgetRow().NameContent()[SNew(STextBlock).Text(FText::FromString(bScene ? TEXT("Scene ID") : TEXT("Chapter ID")))]
			.ValueContent().MinDesiredWidth(300)[SNew(STextBlock).Text(FText::FromString(Id.ToString())).ToolTipText(FText::FromString(TEXT("고유 ID — 이름 변경과 저장으로 바뀌지 않습니다.")))];
			for (const FName Field : {FName(TEXT("DisplayName")), FName(TEXT("Description")), FName(TEXT("SortOrder"))})
			{
				Group.AddPropertyRow(Element->GetChildHandle(Field).ToSharedRef());
			}
			if (bScene == false) continue;
			Group.AddWidgetRow().NameContent()[SNew(STextBlock).Text(FText::FromString(TEXT("소속 Chapter")))]
			.ValueContent().MinDesiredWidth(300)
			[SNew(SComboButton).OnGetMenuContent_Lambda([Definition, Id, Refresh]()
			{
				FMenuBuilder Menu(true, nullptr);
				auto AddParent = [&Menu, Definition, Id, Refresh](FGuid Parent, FText Name)
				{
					Menu.AddMenuEntry(Name, FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([Definition, Id, Parent, Refresh]()
					{
						if (FDialogueStoryEditor::MoveScene(Definition.Get(), Id, Parent))
						{
							Definition->PostEditChange();
						}
						else FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("소속 변경 실패: 정의의 누락/중복 ID와 부모 상태를 확인하세요.")));
						Refresh();
					})));
				};
				AddParent(FGuid(), FText::FromString(TEXT("소속 없음")));
				if (Definition.IsValid()) for (const auto& Chapter : Definition->Chapters)
				{
					if (Definition->FindChapter(Chapter.ChapterId) == nullptr) continue;
					AddParent(Chapter.ChapterId, FText::FromString(Chapter.DisplayName.ToString() + TEXT(" [") + Chapter.ChapterId.ToString() + TEXT("]")));
				}
				return Menu.MakeWidget();
			}).ButtonContent()[SNew(STextBlock).Text_Lambda([Definition, Id]()
			{
				const auto* Scene = Definition.IsValid() ? Definition->FindScene(Id) : nullptr;
				if (Scene == nullptr) return FText::FromString(TEXT("Scene ID 확인 필요"));
				if (Scene->ChapterId.IsValid() == false) return FText::FromString(TEXT("소속 없음"));
				const auto* Chapter = Definition->FindChapter(Scene->ChapterId);
				return FText::FromString((Chapter ? Chapter->DisplayName.ToString() : TEXT("삭제되었거나 중복된 Chapter")) + FString(TEXT(" [")) + Scene->ChapterId.ToString() + TEXT("]"));
			})]];
		}
	}
}
