#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "DialogueTextPresentation.h"
#include "DialoguePresentationWidget.generated.h"

class UDialogueSubsystem;
class UImage;
class UTextBlock;
class UVerticalBox;
class UDialoguePresentationWidget;

// 한 선택 버튼의 ID와 생성 시점 revision을 보존한다.
UCLASS()
class DIALOGUEKIT_API UDialoguePresentationChoiceButton : public UButton
{
	GENERATED_BODY()
public:
	UDialoguePresentationChoiceButton();
	void Configure(UDialoguePresentationWidget* Owner, FGuid Id, int64 Revision);
private:
	UFUNCTION() void Select();
	UPROPERTY() TWeakObjectPtr<UDialoguePresentationWidget> View;
	FGuid ChoiceId;
	int64 ExpectedRevision = 0;
};

// opt-in 기본 화면이다. 기존 프로젝트의 DialogueWidgetClass/에셋을 자동 대체하지 않는다.
UCLASS(BlueprintType, NotBlueprintable)
class DIALOGUEKIT_API UDialoguePresentationWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	void BindRunner(UDialogueBlockRunner* NewRunner);
	// 게임에서는 지역화가 끝난 Subsystem 표시 이벤트를 사용한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	void BindSubsystem(UDialogueSubsystem* NewSubsystem);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	void DetachPresentation();
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	bool SubmitPresentationAdvance();
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Presentation")
	bool SubmitPresentationChoice(FGuid ChoiceId, int64 ExpectedRevision);
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Presentation")
	UDialogueTextPresentation* GetTextPresentation() const { return TextPresentation; }
	// 기본 화면은 고정 구성이다. 사용자 UI는 TextPresentation과 Subsystem 이벤트로 별도 구성한다.
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	UFUNCTION() void ReceivePresentation(const FDialoguePresentation& Presentation);
	void RefreshDisplay();
	void ApplyPortrait();
	void StopVoice();
	void StopMedia();
	void ApplyMedia();
	bool ApplyInput(EDialoguePresentationInput Action, FGuid ChoiceId);
	UPROPERTY() TObjectPtr<UDialogueTextPresentation> TextPresentation;
	UPROPERTY() TObjectPtr<UDialogueBlockRunner> Runner;
	UPROPERTY() TObjectPtr<UDialogueSubsystem> Subsystem;
	UPROPERTY() FDialoguePresentation Current;
	UPROPERTY() TObjectPtr<UImage> Portrait;
	UPROPERTY() TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY() TObjectPtr<UTextBlock> DialogueText;
	UPROPERTY() TObjectPtr<UVerticalBox> ChoiceBox;
	UPROPERTY() TObjectPtr<UTexture2D> LoadedPortrait;
	UPROPERTY() TObjectPtr<class UAudioComponent> VoicePlayer;
	UPROPERTY() TObjectPtr<UAudioComponent> MusicPlayer;
	UPROPERTY() TObjectPtr<UAudioComponent> EffectPlayer;
	UPROPERTY() TObjectPtr<class USoundBase> CurrentMusic;
	int64 VoiceRevision = 0;
	bool bSubmitting = false;
	bool bChoicesDisplayed = false;
};
