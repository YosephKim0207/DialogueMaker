#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "DialoguePortraitData.h"
#include "QuestBase.h"
#include "Enum/PlayerProgress.h"
#include "Struct/DialogueStructure.h"
#include "DialogueNodeInfo.generated.h"

USTRUCT(BlueprintType)
struct FSpeakerEmotePair
{
	GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|FSpeakerEmotePair|Speaker")
    ESpeakerID Speaker;

    UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|FSpeakerEmotePair|Emote")
    EEmoteType EmoteType;

	FSpeakerEmotePair(){ Speaker = ESpeakerID::TestNPC, EmoteType = EEmoteType::None; };
	FSpeakerEmotePair(ESpeakerID Speaker, EEmoteType EmoteType) : Speaker(Speaker), EmoteType(EmoteType){};
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueNodeInfo|Dialogue")
    const FText& GetTitle() const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueNodeInfo|Dialogue")
    const FText& GetDialogueText() const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueNodeInfo|Choices")
    const TArray<FDialogueChoice>& GetDialogueChoices() const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueNodeInfo|Choices")
    void AddDialogueChoice(const FDialogueChoice& DialogueChoice);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueNodeInfo|Choices")
    void RemoveDialogueChoiceAt(int32 Index);
	
	bool IsDialogueAlreadyShown() const;
	void SetShownCondition(const bool NewCondition);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueNodeInfo|SpeakerEmote")
    const FSpeakerEmotePair GetSpeakerEmotePair() const;

    UFUNCTION(Category = "DialogueKit|DialogueNodeInfo|Portrait")
    const FPortraitData GetPortraitData() const;

    UFUNCTION(Category = "DialogueKit|DialogueNodeInfo|Portrait")
    const TArray<FPortraitActionData>& GetPortraitActionDatas() const;
	
private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (AllowPrivateAccess = "true"))
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (AllowPrivateAccess = "true"))
    ESpeakerID SpeakerID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (AllowPrivateAccess = "true"))
    FPortraitData PortraitData;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueNodeInfo|PortraitActions")
    TArray<FPortraitActionData> PortraitActionDatas;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (AllowPrivateAccess = "true"))
    FText DialogueText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (AllowPrivateAccess = "true"))
    TArray<FDialogueChoice> DialogueChoices;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|State", meta = (AllowPrivateAccess = "true"))
    bool bIsShown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Quest", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UQuestBase> QuestToGive;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Quest", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UQuestBase> QuestToClear;
};
