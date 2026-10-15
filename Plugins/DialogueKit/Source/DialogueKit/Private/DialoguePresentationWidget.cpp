#include "DialoguePresentationWidget.h"
#include "DialogueSubsystem.h"
#include "DialogueLineNodeInfo.h"
#include "DialoguePortraitData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "CoreGlobals.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

// 선택 버튼은 일반 진행 키의 포커스를 가져가지 않는다.
UDialoguePresentationChoiceButton::UDialoguePresentationChoiceButton()
{
	InitIsFocusable(false);
}

// 버튼은 생성 당시 화면의 식별자만 전달한다.
void UDialoguePresentationChoiceButton::Configure(UDialoguePresentationWidget* Owner, FGuid Id, int64 Revision)
{
	View = Owner;
	ChoiceId = Id;
	ExpectedRevision = Revision;
	OnClicked.AddUniqueDynamic(this, &UDialoguePresentationChoiceButton::Select);
}

// 이전 화면 버튼의 지연 콜백은 Submit 경계에서 거부한다.
void UDialoguePresentationChoiceButton::Select()
{
	if (View.IsValid())
	{
		View->SubmitPresentationChoice(ChoiceId, ExpectedRevision);
	}
}

// 별도 바이너리 UI 에셋 없이 기본 대화 화면을 만든다.
TSharedRef<SWidget> UDialoguePresentationWidget::RebuildWidget()
{
	SetIsFocusable(true);
	if (TextPresentation == nullptr)
	{
		TextPresentation = NewObject<UDialogueTextPresentation>(this);
	}
	if (WidgetTree == nullptr)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	Portrait = WidgetTree->ConstructWidget<UImage>();
	auto* PortraitSlot = Canvas->AddChildToCanvas(Portrait);
	PortraitSlot->SetAnchors(FAnchors(.5f, .45f));
	PortraitSlot->SetAlignment(FVector2D(.5f, .5f));
	PortraitSlot->SetSize(FVector2D(480, 600));
	Portrait->SetVisibility(ESlateVisibility::Hidden);
	auto* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(.025f, .035f, .05f, .95f));
	Background->SetPadding(FMargin(24));
	auto* PanelSlot = Canvas->AddChildToCanvas(Background);
	PanelSlot->SetAnchors(FAnchors(.08f, .66f, .92f, .96f));
	PanelSlot->SetOffsets(FMargin(0));
	auto* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Background->SetContent(Content);
	SpeakerText = WidgetTree->ConstructWidget<UTextBlock>();
	Content->AddChild(SpeakerText);
	DialogueText = WidgetTree->ConstructWidget<UTextBlock>();
	DialogueText->SetAutoWrapText(true);
	Content->AddChild(DialogueText);
	auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Content->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>();
	Scroll->AddChild(ChoiceBox);
	bChoicesDisplayed = false;
	RefreshDisplay();
	ApplyPortrait();
	return Super::RebuildWidget();
}

// 구독 교체에서 이전 수명과 입력 상태를 정리한다.
void UDialoguePresentationWidget::BindRunner(UDialogueBlockRunner* NewRunner)
{
	DetachPresentation();
	Runner = NewRunner;
	if (Runner)
	{
		Runner->OnPresentationChangedNative.AddUObject(this, &UDialoguePresentationWidget::ReceivePresentation);
		ReceivePresentation(Runner->GetPresentation());
	}
}

// Subsystem의 지역화 복사본을 사용하며 이중 Runner 구독을 만들지 않는다.
void UDialoguePresentationWidget::BindSubsystem(UDialogueSubsystem* NewSubsystem)
{
	DetachPresentation();
	Subsystem = NewSubsystem;
	if (Subsystem)
	{
		Runner = Subsystem->GetBlockRunner();
		Subsystem->OnBlockPresentationChanged.AddUniqueDynamic(this, &UDialoguePresentationWidget::ReceivePresentation);
		ReceivePresentation(Subsystem->GetBlockPresentation());
	}
}

// 화면 종료는 저장 큐나 게임 상태를 취소하지 않는다.
void UDialoguePresentationWidget::DetachPresentation()
{
	if (Runner)
	{
		Runner->OnPresentationChangedNative.RemoveAll(this);
	}
	if (Subsystem)
	{
		Subsystem->OnBlockPresentationChanged.RemoveDynamic(this, &UDialoguePresentationWidget::ReceivePresentation);
	}
	Runner = nullptr;
	Subsystem = nullptr;
	StopMedia();
	SetVisibility(ESlateVisibility::Collapsed);
	bChoicesDisplayed = false;
	Current = FDialoguePresentation();
	if (TextPresentation)
	{
		TextPresentation->SetPresentation(Current);
	}
}

// 선택 갱신에서는 음향과 출력 시간을 다시 시작하지 않는다.
void UDialoguePresentationWidget::ReceivePresentation(const FDialoguePresentation& Presentation)
{
	if (TextPresentation == nullptr)
	{
		TextPresentation = NewObject<UDialogueTextPresentation>(this);
	}
	Current = Presentation;
	TextPresentation->SetPresentation(Presentation);
	bChoicesDisplayed = false;
	const bool bActive = Current.State == EDialogueRunState::WaitingLine || Current.State == EDialogueRunState::WaitingChoice;
	SetVisibility(bActive || Current.State == EDialogueRunState::Failed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Current.bChoicesOnlyUpdate == false)
	{
		ApplyPortrait();
		ApplyMedia();
	}
	RefreshDisplay();
}

// 출력 애니메이션이 필요한 동안만 상태를 갱신한다.
void UDialoguePresentationWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (TextPresentation)
	{
		TextPresentation->SetVoicePlaying(VoicePlayer && VoicePlayer->IsPlaying(), VoiceRevision);
	}
	if (TextPresentation && TextPresentation->IsTextComplete() == false)
	{
		TextPresentation->AdvanceTime(DeltaSeconds);
		RefreshDisplay();
	}
}

// UI의 입력은 하나의 처리 중 guard와 frame token을 사용한다.
bool UDialoguePresentationWidget::SubmitPresentationAdvance()
{
	if (bSubmitting || Runner == nullptr || TextPresentation == nullptr)
	{
		return false;
	}
	TGuardValue<bool> Guard(bSubmitting, true);
	TextPresentation->SetVoicePlaying(VoicePlayer && VoicePlayer->IsPlaying(), VoiceRevision);
	return ApplyInput(TextPresentation->RequestAdvance(TextPresentation->GetRevision(), int64(GFrameCounter)), FGuid());
}

// 버튼 생성 시의 revision으로 이전 버튼 재진입까지 방어한다.
bool UDialoguePresentationWidget::SubmitPresentationChoice(FGuid ChoiceId, int64 ExpectedRevision)
{
	if (bSubmitting || Runner == nullptr || TextPresentation == nullptr)
	{
		return false;
	}
	TGuardValue<bool> Guard(bSubmitting, true);
	TextPresentation->SetVoicePlaying(VoicePlayer && VoicePlayer->IsPlaying(), VoiceRevision);
	return ApplyInput(TextPresentation->RequestChoice(ChoiceId, ExpectedRevision, int64(GFrameCounter)), ChoiceId);
}

// Reveal 분기는 진행/선택/열람/효과 호출을 하지 않는다.
bool UDialoguePresentationWidget::ApplyInput(EDialoguePresentationInput Action, FGuid ChoiceId)
{
	if (Action == EDialoguePresentationInput::Reveal)
	{
		StopVoice();
		RefreshDisplay();
		return true;
	}
	if (Action == EDialoguePresentationInput::Advance)
	{
		return Runner->AdvanceLine();
	}
	if (Action == EDialoguePresentationInput::Select)
	{
		return Runner->SelectChoice(ChoiceId);
	}
	return false;
}

// Choice 목록은 완료 경계/조건 변경에서만 다시 생성한다.
void UDialoguePresentationWidget::RefreshDisplay()
{
	if (DialogueText == nullptr || TextPresentation == nullptr)
	{
		return;
	}
	DialogueText->SetText(Current.State == EDialogueRunState::Failed ? FText::FromName(Current.ErrorCode) : TextPresentation->GetVisibleText());
	SpeakerText->SetText(Current.Line ? UEnum::GetDisplayValueAsText(Current.Line->GetSpeakerEmotePair().Speaker) : FText());
	if (bChoicesDisplayed == false)
	{
		ChoiceBox->ClearChildren();
		for (const auto& Choice : TextPresentation->GetVisibleChoices())
		{
			auto* Button = WidgetTree->ConstructWidget<UDialoguePresentationChoiceButton>();
			Button->Configure(this, Choice.ChoiceId, TextPresentation->GetRevision());
			auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(Choice.Text);
			Label->SetAutoWrapText(true);
			Button->SetContent(Label);
			ChoiceBox->AddChild(Button);
		}
		bChoicesDisplayed = TextPresentation->IsTextComplete();
	}
}

// Line 직접 이미지를 우선 사용하고 없으면 화자와 표정이 모두 일치하는 정의를 찾는다.
void UDialoguePresentationWidget::ApplyPortrait()
{
	if (Portrait == nullptr)
	{
		return;
	}
	LoadedPortrait = nullptr;
	const auto* Line = Cast<UDialogueLineNodeInfo>(Current.Line);
	if (Line)
	{
		LoadedPortrait = Line->PortraitImage.LoadSynchronous();
		if (Line->PortraitImage.IsNull())
		{
			const auto Pair = Line->GetSpeakerEmotePair();
			FARFilter Filter;
			Filter.ClassPaths.Add(UDialoguePortraitData::StaticClass()->GetClassPathName());
			Filter.TagsAndValues.Add(UDialoguePortraitData::GetSpeakerIDTag(), StaticEnum<ESpeakerID>()->GetNameStringByValue(int64(Pair.Speaker)));
			TArray<FAssetData> Assets;
			FAssetRegistryModule::GetRegistry().GetAssets(Filter, Assets);
			for (const FAssetData& Asset : Assets)
			{
				FString Emote;
				if (Asset.GetTagValue(UDialoguePortraitData::GetEmoteTypeTag(), Emote) && StaticEnum<EEmoteType>()->GetValueByNameString(Emote) == int64(Pair.EmoteType))
				{
					if (auto* Data = Cast<UDialoguePortraitData>(Asset.GetAsset()))
					{
						LoadedPortrait = Data->LoadPortraitTexture();
						break;
					}
				}
			}
		}
		auto* ImageSlot = Cast<UCanvasPanelSlot>(Portrait->Slot);
		const EPortraitSide Side = Line->GetPortraitData().SidePosition;
		ImageSlot->SetAnchors(FAnchors(Side == EPortraitSide::Left ? .2f : Side == EPortraitSide::Right ? .8f : .5f, .43f));
		ImageSlot->SetPosition(Line->PortraitOffset);
		Portrait->SetRenderScale(FVector2D(Line->bMirrorPortrait ? -1.0f : 1.0f, 1.0f));
	}
	Portrait->SetBrushFromTexture(LoadedPortrait, false);
	Portrait->SetVisibility(LoadedPortrait ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

// 펼치기에서는 음성만 중지하고 BGM/효과음은 계속 재생한다.
void UDialoguePresentationWidget::StopVoice()
{
	if (VoicePlayer)
	{
		VoicePlayer->Stop();
		VoicePlayer->DestroyComponent();
		VoicePlayer = nullptr;
	}
	if (TextPresentation)
	{
		TextPresentation->SetVoicePlaying(false, VoiceRevision);
	}
}

// 새 실행/취소/PIE 종료 시 자신이 만든 컴포넌트만 정리한다.
void UDialoguePresentationWidget::StopMedia()
{
	StopVoice();
	if (MusicPlayer)
	{
		MusicPlayer->Stop();
		MusicPlayer->DestroyComponent();
		MusicPlayer = nullptr;
	}
	if (EffectPlayer)
	{
		EffectPlayer->Stop();
		EffectPlayer->DestroyComponent();
		EffectPlayer = nullptr;
	}
	CurrentMusic = nullptr;
}

// Line에 명시된 음향만 재생한다. 재개 시 효과 지급과 무관하게 음향을 처음부터 표시한다.
void UDialoguePresentationWidget::ApplyMedia()
{
	StopVoice();
	if (EffectPlayer)
	{
		EffectPlayer->Stop();
		EffectPlayer->DestroyComponent();
		EffectPlayer = nullptr;
	}
	const auto* Line = Cast<UDialogueLineNodeInfo>(Current.Line);
	if (Line == nullptr || (Current.State != EDialogueRunState::WaitingLine && Current.State != EDialogueRunState::WaitingChoice))
	{
		StopMedia();
		return;
	}
	USoundBase* Music = Line->BackgroundMusic.LoadSynchronous();
	if (Music != CurrentMusic || (Music && MusicPlayer == nullptr))
	{
		if (MusicPlayer)
		{
			MusicPlayer->Stop();
			MusicPlayer->DestroyComponent();
		}
		CurrentMusic = Music;
		MusicPlayer = Music ? UGameplayStatics::SpawnSound2D(this, Music, 1.0f, 1.0f, 0.0f, nullptr, false, false) : nullptr;
	}
	if (USoundBase* Sound = Line->SoundEffect.LoadSynchronous())
	{
		EffectPlayer = UGameplayStatics::SpawnSound2D(this, Sound, 1.0f, 1.0f, 0.0f, nullptr, false, false);
	}
	VoiceRevision = TextPresentation->GetRevision();
	if (USoundBase* Sound = Line->Voice.LoadSynchronous())
	{
		VoicePlayer = UGameplayStatics::SpawnSound2D(this, Sound, 1.0f, 1.0f, 0.0f, nullptr, false, false);
	}
	TextPresentation->SetVoicePlaying(VoicePlayer && VoicePlayer->IsPlaying(), VoiceRevision);
}

// 키 반복은 하나의 물리 입력으로 간주하여 진행하지 않는다.
FReply UDialoguePresentationWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::SpaceBar || Event.GetKey() == EKeys::Enter)
	{
		if (Event.IsRepeat() == false) SubmitPresentationAdvance();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

// 선택 버튼이 처리하지 않은 배경 클릭만 일반 진행 입력으로 사용한다.
FReply UDialoguePresentationWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SubmitPresentationAdvance();
		return FReply::Handled().SetUserFocus(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}

// viewport/PIE 종료 시에도 콜백이 제거된다.
void UDialoguePresentationWidget::NativeDestruct()
{
	DetachPresentation();
	Super::NativeDestruct();
}
