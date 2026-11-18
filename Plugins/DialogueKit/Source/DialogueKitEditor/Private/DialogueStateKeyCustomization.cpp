#include "DialogueStateKeyCustomization.h"
#include "DialogueSettings.h"
#include "DialogueStateProvider.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "Editor.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Script.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

namespace
{
DECLARE_DELEGATE_OneParam(FOnDialogueIdSelected, FName);
// 메뉴가 검색 결과와 선택 수명을 소유하고 Details는 약한 콜백으로만 참조한다.
class SDialogueIdMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogueIdMenu) {}
		SLATE_ARGUMENT(TArray<FDialogueIdOption>, Options)
		SLATE_EVENT(FOnDialogueIdSelected, OnSelected)
	SLATE_END_ARGS()

	// 검색 목록을 한 번 복사하고 화면 내 검색만 수행한다.
	void Construct(const FArguments& Args)
	{
		OnSelected = Args._OnSelected;
		for (const FDialogueIdOption& Option : Args._Options)
		{
			All.Add(MakeShared<FDialogueIdOption>(Option));
		}
		Filtered = All;
		ChildSlot
		[
			SNew(SBox).WidthOverride(400).HeightOverride(300)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SSearchBox).HintText(FText::FromString(TEXT("이름 또는 ID 검색")))
					.OnTextChanged(this, &SDialogueIdMenu::Search)
				]
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SAssignNew(List, SListView<TSharedPtr<FDialogueIdOption>>)
					.ListItemsSource(&Filtered)
					.OnGenerateRow_Lambda([](TSharedPtr<FDialogueIdOption> Option, const TSharedRef<STableViewBase>& Owner)
					{
						return SNew(STableRow<TSharedPtr<FDialogueIdOption>>, Owner)
						[SNew(STextBlock).Text(Option->Label).ToolTipText(Option->Label)];
					})
					.OnKeyDownHandler_Lambda([this](const FGeometry&, const FKeyEvent& Event)
					{
						if (Event.GetKey() == EKeys::Enter)
						{
							const TArray<TSharedPtr<FDialogueIdOption>> Selected = List->GetSelectedItems();
							if (!Selected.IsEmpty())
							{
								OnSelected.ExecuteIfBound(Selected[0]->Id);
								FSlateApplication::Get().DismissAllMenus();
								return FReply::Handled();
							}
						}
						return FReply::Unhandled();
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FDialogueIdOption> Option, ESelectInfo::Type Reason)
					{
						if (Option.IsValid() && Reason == ESelectInfo::OnMouseClick)
						{
							OnSelected.ExecuteIfBound(Option->Id);
							FSlateApplication::Get().DismissAllMenus();
						}
					})
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text_Lambda([this]()
					{
						return Filtered.IsEmpty() ? FText::FromString(TEXT("일치하는 정의가 없습니다. 고급 직접 입력을 사용할 수 있습니다.")) : FText::GetEmpty();
					}).AutoWrapText(true)
				]
			]
		];
	}

private:
	// 필터 변경은 선택이나 원본 ID 변경을 발생시키지 않는다.
	void Search(const FText& Text)
	{
		Filtered.Reset();
		for (const TSharedPtr<FDialogueIdOption>& Option : All)
		{
			if (FDialogueDefinitionOptions::Matches(*Option, Text.ToString()))
			{
				Filtered.Add(Option);
			}
		}
		List->RequestListRefresh();
	}
	FOnDialogueIdSelected OnSelected;
	TArray<TSharedPtr<FDialogueIdOption>> All;
	TArray<TSharedPtr<FDialogueIdOption>> Filtered;
	TSharedPtr<SListView<TSharedPtr<FDialogueIdOption>>> List;
};

// 모든 편집 경로에서 실제 PIE 중 변경을 거부한다.
bool IsDialogueIdEditingAllowed()
{
	return GEditor == nullptr || (GEditor->PlayWorld == nullptr && !GEditor->bIsSimulatingInEditor);
}
}

// 설정 원본은 목록 준비 시에만 동기 로드한다.
bool FDialogueDefinitionOptions::LoadConfigured()
{
	const TSoftObjectPtr<UObject>& Source = GetDefault<UDialogueSettings>()->ConditionDefinitionSource;
	if (Source.IsNull())
	{
		Definitions.Reset();
		Error = FText::FromString(TEXT("정의 원본 없음: Project Settings → DialogueKit Settings → Condition Definition Source를 지정하세요. 고급 직접 입력도 가능합니다."));
		return false;
	}
	TStrongObjectPtr<UObject> Loaded(Source.LoadSynchronous());
	return ReadSource(Loaded.Get());
}

// 인터페이스는 에셋 필터와 별도로 로드 후 다시 검증한다.
bool FDialogueDefinitionOptions::ReadSource(UObject* Source)
{
	Definitions.Reset();
	Error = FText::GetEmpty();
	if (!IsValid(Source) || !Source->GetClass()->ImplementsInterface(UDialogueDefinitionProvider::StaticClass()))
	{
		Error = FText::FromString(TEXT("정의 원본 로드 실패 또는 IDialogueDefinitionProvider 미구현. 설정과 에셋을 확인하세요."));
		return false;
	}
	TStrongObjectPtr<UObject> KeepAlive(Source);
	TArray<FDialogueStateDefinition> Loaded;
	FEditorScriptExecutionGuard ScriptGuard;
	if (!IDialogueDefinitionProvider::Execute_GetDialogueDefinitions(Source, Loaded))
	{
		Error = FText::FromString(TEXT("정의 제공자가 목록 읽기에 실패했습니다. 원본의 ID·표시 이름·테이블 형식을 확인하세요."));
		return false;
	}
	return SetDefinitions(Loaded);
}

// 한 행이라도 잘못되면 부분 선택 목록을 노출하지 않는다.
bool FDialogueDefinitionOptions::SetDefinitions(const TArray<FDialogueStateDefinition>& InDefinitions)
{
	Definitions.Reset();
	Error = FText::GetEmpty();
	TSet<FDialogueStateKey> Seen;
	for (const FDialogueStateDefinition& Definition : InDefinitions)
	{
		if (!Definition.Key.IsValid() || Definition.DisplayName.IsEmptyOrWhitespace() || Seen.Contains(Definition.Key))
		{
			Error = FText::FromString(TEXT("정의 오류: 잘못된 키·빈 표시 이름·중복 복합 키가 있습니다. 전체 목록을 거부했습니다."));
			return false;
		}
		Seen.Add(Definition.Key);
	}
	Definitions = InDefinitions;
	return true;
}

// Target의 표시명은 항목 표시명에서 추측하지 않는다.
TArray<FDialogueIdOption> FDialogueDefinitionOptions::GetOptions(EDialogueStateKind Kind, FName TargetId, bool bTarget) const
{
	TArray<FDialogueIdOption> Result;
	TSet<FName> Seen;
	for (const FDialogueStateDefinition& Definition : Definitions)
	{
		if (Definition.Key.Kind != Kind || (!bTarget && (Definition.Key.TargetId != TargetId || Kind == EDialogueStateKind::Tag)))
		{
			continue;
		}
		const FName Id = bTarget ? Definition.Key.TargetId : Definition.Key.ItemId;
		if (Seen.Contains(Id))
		{
			continue;
		}
		Seen.Add(Id);
		const FText Label = bTarget
			? FText::FromString(Id.IsNone() ? TEXT("전역 [None]") : Id.ToString())
			: FText::Format(FText::FromString(TEXT("{0} [{1}]")), Definition.DisplayName, FText::FromName(Id));
		Result.Add({Id, Label});
	}
	Result.Sort([](const FDialogueIdOption& A, const FDialogueIdOption& B) { return A.Id.LexicalLess(B.Id); });
	return Result;
}

// 활성 필드 기준의 기존 키 동일성을 그대로 사용한다.
bool FDialogueDefinitionOptions::Contains(const FDialogueStateKey& Key) const
{
	return Definitions.ContainsByPredicate([&Key](const FDialogueStateDefinition& Definition) { return Definition.Key == Key; });
}

// 검색은 표시 문자열과 안정 ID를 모두 지원한다.
bool FDialogueDefinitionOptions::Matches(const FDialogueIdOption& Option, const FString& Search)
{
	return Option.Label.ToString().Contains(Search) || Option.Id.ToString().Contains(Search);
}

// 공통 Key 레이아웃을 생성한다.
TSharedRef<IPropertyTypeCustomization> FDialogueStateKeyCustomization::MakeInstance()
{
	return MakeShared<FDialogueStateKeyCustomization>();
}

// 기본 헤더와 리셋/복사 경로는 프로퍼티 행에 맡긴다.
void FDialogueStateKeyCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils& Utils)
{
	Key = Handle;
	Utilities = Utils.GetPropertyUtilities();
	Row.NameContent()[Handle->CreatePropertyNameWidget()];
}

// 종류 및 기본 태그 편집기는 유지하고 ID 행만 선택기로 바꾼다.
void FDialogueStateKeyCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils)
{
	Options.Error = FText::FromString(TEXT("정의 목록을 열거나 새로고침하여 등록 상태를 확인하세요. 기존 ID는 유지됩니다."));
	Builder.AddProperty(Handle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDialogueStateKey, Kind)).ToSharedRef());
	AddIdRows(Builder, true);
	AddIdRows(Builder, false);
	Builder.AddProperty(Handle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDialogueStateKey, Tag)).ToSharedRef());
	IDetailGroup& Manual = Builder.AddGroup(TEXT("ManualIds"), FText::FromString(TEXT("고급: ID 직접 입력")));
	for (bool bTarget : {true, false})
	{
		TSharedRef<IPropertyHandle> Property = Key->GetChildHandle(bTarget ? TEXT("TargetId") : TEXT("ItemId")).ToSharedRef();
		Manual.AddWidgetRow()
		.NameContent()[SNew(STextBlock).Text(FText::FromString(bTarget ? TEXT("Target ID (직접 입력)") : TEXT("Item ID (직접 입력)"))).ToolTipText(Property->GetToolTipText())]
		.ValueContent().MinDesiredWidth(260)
		[
			SNew(SBox).IsEnabled(this, &FDialogueStateKeyCustomization::CanEdit)
			[Property->CreatePropertyValueWidget()]
		];
	}

	Builder.AddCustomRow(FText::FromString(TEXT("정의 목록 상태 새로고침"))).WholeRowContent()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text(this, &FDialogueStateKeyCustomization::GetStatus).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton).Text(FText::FromString(TEXT("정의 목록 새로고침")))
			.IsEnabled(this, &FDialogueStateKeyCustomization::CanEdit)
			.OnClicked_Lambda([this]()
			{
				if (CanEdit()) Options.LoadConfigured();
				return FReply::Handled();
			})
		]
	];
}

// 고급 행은 원래 property widget을 사용해 직접 입력/여러 값/알림을 보존한다.
void FDialogueStateKeyCustomization::AddIdRows(IDetailChildrenBuilder& Builder, bool bTarget)
{
	TSharedRef<IPropertyHandle> Property = Key->GetChildHandle(bTarget ? TEXT("TargetId") : TEXT("ItemId")).ToSharedRef();
	Builder.AddCustomRow(Property->GetPropertyDisplayName())
	.NameContent()[Property->CreatePropertyNameWidget()]
	.ValueContent().MinDesiredWidth(260)
	[
		SNew(SComboButton)
		.IsEnabled_Lambda([this, bTarget]()
		{
			EDialogueStateKind Kind;
			FName Target;
			return CanEdit() && GetFilter(Kind, Target, bTarget) && (bTarget || Kind != EDialogueStateKind::Tag);
		})
		.OnGetMenuContent(this, &FDialogueStateKeyCustomization::MakeMenu, bTarget)
		.ButtonContent()[SNew(STextBlock).Text(this, &FDialogueStateKeyCustomization::GetLabel, bTarget)]
	];

}

// 전체 Details 잠금과 실제 PIE 상태를 함께 확인한다.
bool FDialogueStateKeyCustomization::CanEdit() const
{
	const TSharedPtr<IPropertyUtilities> Pinned = Utilities.Pin();
	return Key.IsValid() && !Key->IsEditConst() && Pinned.IsValid() && Pinned->IsPropertyEditingEnabled() && IsDialogueIdEditingAllowed();
}

// 다중 객체의 기준 값이 다르면 선택기를 잠근다.
bool FDialogueStateKeyCustomization::GetFilter(EDialogueStateKind& Kind, FName& Target, bool bTarget) const
{
	uint8 Value = 0;
	if (Key->GetChildHandle(TEXT("Kind"))->GetValue(Value) != FPropertyAccess::Success) return false;
	Kind = static_cast<EDialogueStateKind>(Value);
	return bTarget || Key->GetChildHandle(TEXT("TargetId"))->GetValue(Target) == FPropertyAccess::Success;
}

// 메뉴는 값 스냅샷을 소유하고 선택 시 현재 필터를 다시 확인한다.
TSharedRef<SWidget> FDialogueStateKeyCustomization::MakeMenu(bool bTarget)
{
	Options.LoadConfigured();
	EDialogueStateKind Kind;
	FName Target;
	if (!CanEdit() || !GetFilter(Kind, Target, bTarget) || !Options.Error.IsEmpty())
	{
		return SNew(STextBlock).Text(GetStatus()).WrapTextAt(400);
	}
	TWeakPtr<FDialogueStateKeyCustomization> Weak = StaticCastSharedRef<FDialogueStateKeyCustomization>(AsShared());
	return SNew(SDialogueIdMenu).Options(Options.GetOptions(Kind, Target, bTarget))
		.OnSelected_Lambda([Weak, bTarget](FName Id)
		{
			if (const TSharedPtr<FDialogueStateKeyCustomization> Self = Weak.Pin())
			{
				// 메뉴 표시 후 원본이 수정된 경우에도 오래된 ID를 새로 쓰지 않는다.
				if (Self->CanEdit() && Self->Options.LoadConfigured())
				{
					ApplySelection(Self->Key.ToSharedRef(), Self->Options, bTarget, Id, Self->CanEdit());
				}
			}
		});
}

// 명시적으로 선택한 한 필드만 변경하고 UE의 단일 트랜잭션을 사용한다.
FPropertyAccess::Result FDialogueStateKeyCustomization::ApplySelection(const TSharedRef<IPropertyHandle>& KeyHandle, const FDialogueDefinitionOptions& Source, bool bTarget, FName Id, bool bEditingEnabled)
{
	if (!bEditingEnabled || !IsDialogueIdEditingAllowed() || KeyHandle->IsEditConst() || !Source.Error.IsEmpty()) return FPropertyAccess::Fail;
	uint8 Kind = 0;
	FName Target;
	if (KeyHandle->GetChildHandle(TEXT("Kind"))->GetValue(Kind) != FPropertyAccess::Success) return FPropertyAccess::Fail;
	if (!bTarget && KeyHandle->GetChildHandle(TEXT("TargetId"))->GetValue(Target) != FPropertyAccess::Success) return FPropertyAccess::Fail;
	const TArray<FDialogueIdOption> Available = Source.GetOptions(static_cast<EDialogueStateKind>(Kind), Target, bTarget);
	if (!Available.ContainsByPredicate([Id](const FDialogueIdOption& Option) { return Option.Id == Id; })) return FPropertyAccess::Fail;
	const TSharedPtr<IPropertyHandle> Property = KeyHandle->GetChildHandle(bTarget ? TEXT("TargetId") : TEXT("ItemId"));
	if (!Property.IsValid() || Property->IsEditConst()) return FPropertyAccess::Fail;
	return Property->SetValue(Id);
}

// 미등록 ID도 원문 그대로 보여주고 자동 선택으로 대체하지 않는다.
FText FDialogueStateKeyCustomization::GetLabel(bool bTarget) const
{
	FName Id;
	if (Key->GetChildHandle(bTarget ? TEXT("TargetId") : TEXT("ItemId"))->GetValue(Id) != FPropertyAccess::Success)
	{
		return FText::FromString(TEXT("여러 값"));
	}
	EDialogueStateKind Kind;
	FName Target;
	if (GetFilter(Kind, Target, bTarget))
	{
		if (!bTarget && Kind == EDialogueStateKind::Tag) return FText::FromString(TEXT("Tag 종류에서는 사용하지 않음"));
		if (bTarget) return FText::FromString(Id.IsNone() ? TEXT("전역 [None]") : Id.ToString());
		for (const FDialogueStateDefinition& Definition : Options.Definitions)
		{
			if (Definition.Key.Kind == Kind && Definition.Key.TargetId == Target && Definition.Key.ItemId == Id)
			{
				return FText::Format(FText::FromString(TEXT("{0} [{1}]")), Definition.DisplayName, FText::FromName(Id));
			}
		}
	}
	return FText::FromName(Id);
}

// 진단은 원본 부재/실패와 정상 빈 목록/미등록 키를 구별한다.
FText FDialogueStateKeyCustomization::GetStatus() const
{
	if (!Options.Error.IsEmpty()) return Options.Error;
	FDialogueStateKey Current;
	if (!GetFilter(Current.Kind, Current.TargetId, false)) return FText::FromString(TEXT("여러 Kind/Target 값입니다. 기준을 통일하거나 고급 직접 입력을 사용하세요."));
	if (Current.Kind == EDialogueStateKind::Tag)
	{
		FName TagName;
		const TSharedPtr<IPropertyHandle> Tag = Key->GetChildHandle(TEXT("Tag"));
		const TSharedPtr<IPropertyHandle> Name = Tag.IsValid() ? Tag->GetChildHandle(TEXT("TagName")) : nullptr;
		if (!Name.IsValid() || Name->GetValue(TagName) != FPropertyAccess::Success) return FText::FromString(TEXT("여러 Tag 값이거나 태그를 읽을 수 없습니다."));
		Current.Tag = FGameplayTag::RequestGameplayTag(TagName, false);
	}
	else if (Key->GetChildHandle(TEXT("ItemId"))->GetValue(Current.ItemId) != FPropertyAccess::Success)
	{
		return FText::FromString(TEXT("여러 Item ID 값입니다."));
	}
	if (Options.Definitions.IsEmpty()) return FText::FromString(TEXT("정의 원본은 정상이나 등록된 항목이 없습니다. 기존 ID는 유지됩니다."));
	if (!Options.Contains(Current)) return FText::FromString(TEXT("미등록 상태 키입니다. Kind·Target ID·Item ID/Tag 조합과 원본을 확인하세요. 기존 값은 유지됩니다."));
	return FText::FromString(TEXT("등록된 상태 키입니다. 정의는 선택 목록이며 현재 게임 값과 별개입니다."));
}
