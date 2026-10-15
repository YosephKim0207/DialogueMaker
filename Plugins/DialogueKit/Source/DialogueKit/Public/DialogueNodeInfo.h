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
	
	// 명시적인 편집에서 누락/중복 선택 ID만 생성한다. 조회·로드에는 호출하지 않는다.
	void EnsureChoiceIds();

	// 현재 Line의 열람 버전을 조회한다. 저장 형식/콘텐츠 버전과 구분한다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|DialogueNodeInfo|Identity")
	int32 GetReadVersion() const { return ReadVersion; }

	// 명시적인 미열람 갱신에서만 호출한다. 최대값에서는 false로 실패하고 상태를 유지한다.
	bool AdvanceReadVersion();
	// 새 노드 복제는 열람 버전과 구형 shown 값을 초기화한다.
	void ResetReadIdentityForDuplicate();
	// 연속 발화의 초기값만 복사한다. 본문/액션/퀘스트/열람 기록은 복사하지 않는다.
	void CopySpeakerPresentationFrom(const UDialogueNodeInfo& Source);

	bool IsDialogueAlreadyShown() const;
	void SetShownCondition(const bool NewCondition);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueNodeInfo|SpeakerEmote")
    const FSpeakerEmotePair GetSpeakerEmotePair() const;

    UFUNCTION(Category = "DialogueKit|DialogueNodeInfo|Portrait")
    const FPortraitData GetPortraitData() const;

    UFUNCTION(Category = "DialogueKit|DialogueNodeInfo|Portrait")
    const TArray<FPortraitActionData>& GetPortraitActionDatas() const;
	
protected:
	UPROPERTY()
	bool bStandaloneLine = false;

private:
	UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueNodeInfo|Identity")
	int32 ReadVersion = 1;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (AllowPrivateAccess = "true", EditCondition = "!bStandaloneLine", EditConditionHides))
    TArray<FDialogueChoice> DialogueChoices;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|State", meta = (AllowPrivateAccess = "true"))
    bool bIsShown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Quest", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UQuestBase> QuestToGive;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Quest", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UQuestBase> QuestToClear;
};
