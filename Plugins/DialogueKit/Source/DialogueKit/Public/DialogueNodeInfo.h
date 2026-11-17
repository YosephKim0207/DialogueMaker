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
	UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueNodeInfo|Identity", meta = (ToolTip = "이 Line의 누적 읽음 판정 버전입니다. 본문 수정만으로 바뀌지 않습니다. 미열람 갱신 버튼으로 올리면 기존 플레이어에게 다시 미열람으로 취급됩니다."))
	int32 ReadVersion = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (ToolTip = "편집 그래프의 노드 제목입니다. 플레이어에게 표시할 대사 본문은 Dialogue Text에 작성합니다.", AllowPrivateAccess = "true"))
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (ToolTip = "이 대사를 말하는 화자입니다. 화자 이름 표시와 화자·표정 기반 초상화 조회에 사용합니다. Block 진입 대상과 다를 수 있습니다.", AllowPrivateAccess = "true"))
    ESpeakerID SpeakerID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (ToolTip = "화자의 표정과 화면 배치 등 초상화 설정입니다. 신규 Line의 직접 이미지·오프셋·반전은 별도 Portrait 항목에서 설정합니다.", AllowPrivateAccess = "true"))
    FPortraitData PortraitData;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueNodeInfo|PortraitActions", meta = (ToolTip = "구형 초상화 연출 경로에 전달하는 동작 목록입니다. 신규 Block 기본 화면이 이 액션 배열을 자동 실행하지는 않습니다."))
    TArray<FPortraitActionData> PortraitActionDatas;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (ToolTip = "플레이어에게 표시할 대사 본문입니다. 신규 Line에서는 글자당 시간 또는 전체 시간 설정을 적용하며 구형 일반 노드는 기존 UI 표시 경로를 사용합니다.", AllowPrivateAccess = "true"))
    FText DialogueText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Default", meta = (ToolTip = "구형 대사 노드에 포함된 선택지 목록입니다. 신규 Line에서는 숨겨지며 독립 Choice 노드의 Options를 사용합니다.", AllowPrivateAccess = "true", EditCondition = "!bStandaloneLine", EditConditionHides))
    TArray<FDialogueChoice> DialogueChoices;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|State", meta = (ToolTip = "구형 노드의 열람 표시입니다. 신규 실행의 누적 읽음은 별도 기록과 Read Version으로 판정합니다.", AllowPrivateAccess = "true"))
    bool bIsShown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Quest", meta = (ToolTip = "구형 대사 자료의 퀘스트 지정 필드입니다. 신규 Block 실행기가 자동 지급하지 않습니다. 게임 효과 연동을 사용하세요.", AllowPrivateAccess = "true"))
    TSubclassOf<UQuestBase> QuestToGive;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueNodeInfo|Quest", meta = (ToolTip = "구형 대사 자료의 퀘스트 지정 필드입니다. 신규 Block 실행기가 자동 완료하지 않습니다. 게임 효과 연동을 사용하세요.", AllowPrivateAccess = "true"))
    TSubclassOf<UQuestBase> QuestToClear;
};
