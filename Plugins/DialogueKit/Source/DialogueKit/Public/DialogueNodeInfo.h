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

        UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Portrait")
        ESpeakerID Speaker;

        UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Portrait")
        EEmoteType EmoteType;

	FSpeakerEmotePair(){ Speaker = ESpeakerID::TestNPC, EmoteType = EEmoteType::None; };
	FSpeakerEmotePair(ESpeakerID Speaker, EEmoteType EmoteType) : Speaker(Speaker), EmoteType(EmoteType){};
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()

public:
        UFUNCTION(BlueprintCallable, Category = "Dialogue|Content")
        const FText& GetTitle() const;

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Content")
        const FText& GetDialogueText() const;

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Content")
        const TArray<FDialogueChoice>& GetDialogueChoices() const;

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Content")
        void AddDialogueChoice(const FDialogueChoice& DialogueChoice);

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Content")
        void RemoveDialogueChoiceAt(int32 Index);
	
	bool IsDialogueAlreadyShown() const;
	void SetShownCondition(const bool NewCondition);

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Portrait")
        const FSpeakerEmotePair GetSpeakerEmotePair() const;
	
	UFUNCTION(Category= "Portrait")
	const FPortraitData GetPortraitData() const;

	UFUNCTION(Category = "Portrait")
	const TArray<FPortraitActionData>& GetPortraitActionDatas() const;
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default", meta = (AllowPrivateAccess = "true"))
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default", meta = (AllowPrivateAccess = "true"))
	ESpeakerID SpeakerID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default", meta = (AllowPrivateAccess = "true"))
	FPortraitData PortraitData;

	UPROPERTY(EditAnywhere, Category = "Portrait Actions")
	TArray<FPortraitActionData> PortraitActionDatas;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default", meta = (AllowPrivateAccess = "true"))
	FText DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default", meta = (AllowPrivateAccess = "true"))
	TArray<FDialogueChoice> DialogueChoices;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|State", meta = (AllowPrivateAccess = "true"))
        bool bIsShown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UQuestBase> QuestToGive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UQuestBase> QuestToClear;
};
