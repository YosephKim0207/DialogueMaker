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

	UPROPERTY(BlueprintReadOnly)
	ESpeakerID Speaker;

	UPROPERTY(BlueprintReadOnly)
	EEmoteType EmoteType;

	FSpeakerEmotePair(){ Speaker = ESpeakerID::TestNPC, EmoteType = EEmoteType::None; };
	FSpeakerEmotePair(ESpeakerID Speaker, EEmoteType EmoteType) : Speaker(Speaker), EmoteType(EmoteType){};
};

UCLASS(BlueprintType)
class DIALOGUEMAKER_API UDialogueNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	const FText& GetTitle() const;
	
	UFUNCTION(BlueprintCallable)
	const FText& GetDialogueText() const;

	UFUNCTION(BlueprintCallable)
	const TArray<FDialogueChoice>& GetDialogueChoices() const;

	UFUNCTION(BlueprintCallable)
	void AddDialogueChoice(const FDialogueChoice& DialogueChoice);

	UFUNCTION(BlueprintCallable)
	void RemoveDialogueChoiceAt(int32 Index);
	
	bool IsDialogueAlreadyShown() const;
	void SetShownCondition(const bool NewCondition);

	UFUNCTION(BlueprintCallable)
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsShown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UQuestBase> QuestToGive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UQuestBase> QuestToClear;
};
