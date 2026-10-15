// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.h"
#include "DialogueBlockRunner.h"
#include "DialogueCheckpointSession.h"
#include "DialogueNodeInfo.h"
#include "ShownDialogueSaveData.h"
#include "AssetRegistry/ARFilter.h"
#include "Struct/DialogueConditionEvalCriteria.h"
#include "Engine/StreamableManager.h"
#include "Enum/Portrait.h"
#include "Subsystems/Subsystem.h"
#include "DialogueSubsystem.generated.h"

DECLARE_DELEGATE_OneParam(FOnDialogueReady, UDialogueGraph*);
DECLARE_DELEGATE_OneParam(FOnCurrentDialogueNodeChange, FGuid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStopSkip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueNodeInfoChanged);

USTRUCT(BlueprintType)
struct FPortraitEmoteIDPair
{
	GENERATED_BODY()

	UPROPERTY()
	EEmoteType EmoteType;

	UPROPERTY()
	FPrimaryAssetId PortraitAssetId;

	FPortraitEmoteIDPair() {EmoteType = EEmoteType::None; PortraitAssetId = FPrimaryAssetId(); };
	FPortraitEmoteIDPair(EEmoteType EmoteType, const FPrimaryAssetId& PortraitAssetId) : EmoteType(EmoteType), PortraitAssetId(PortraitAssetId) {};
};

UCLASS()
class DIALOGUEKIT_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UDialogueSubsystem* Get(const UObject* WorldContextObject);
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void BeginDialogue(ESpeakerID SpeakerID);

	// 신규 Graph의 게임 제공자를 등록한다. 자동 후보 시작도 이 설정을 사용한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Block")
	void ConfigureBlockProviders(UObject* StateProvider, UObject* EffectProvider, UDialogueDiagnosticRouter* Router);

	// 이미 로드한 신규 Block을 실행한다. 실패 안내/후속 흐름은 반환값과 표시 상태로 게임이 처리한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Block")
	bool StartDialogueBlock(UDialogueGraph* Block);

	// 게임의 저장 제공자를 신규 실행 경로에 연결한다. 기존 수동 저장 구독은 관찰 전용으로 변경한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Block")
	bool ConfigureCheckpointSession(UObject* SaveProvider, UDialogueSaveQueue* SaveQueue);
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Block")
	UDialogueCheckpointSession* GetCheckpointSession() const { return CheckpointSession; }
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Block")
	bool ResumeDialogueBlock(UDialogueGraph* Block, const FDialogueCheckpointSnapshot& Snapshot);

	// 시작 전에 효과/완료 경계 구독에 사용한다. 반환 객체의 수명은 Subsystem이 유지한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Block")
	UDialogueBlockRunner* GetBlockRunner();

	// 신규 UI는 고정 ChoiceId로 선택한다. legacy 인덱스 기반 호출도 아래에서 변환한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Block")
	bool SelectBlockChoice(FGuid ChoiceId);

	UFUNCTION(BlueprintPure, Category = "DialogueKit|Block")
	FDialoguePresentation GetBlockPresentation() const { return BlockPresentation; }

	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Block")
	FDialoguePresentationEvent OnBlockPresentationChanged;
	
    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|Progression")
    class UDialogueNodeInfo* ProgressNextDialogue(const int32 SelectedChoiceIndex = 0, const bool bIsFirstDialogue = false);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|CurrentDialogue")
    const UDialogueNodeInfo* GetCurrentDialogueNodeInfo() const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|Choice")
    bool HasChoicesInCurrentDialogue(UDialogueNodeInfo* DialogueNodeInfo) const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|Choice")
    void GetSelectableChoiceTexts(UDialogueNodeInfo* DialogueNodeInfo, TArray<FText>& OutSelectableChoiceTexts, TArray<int32>& OutSelectableChoiceOriginalIndex) const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|ShownDialogue")
    void MakeCurrentDialogueNodeToShown();

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|ShownDialogue")
    bool IsAlreadyShownDialogue(UDialogueNodeInfo* DialogueNodeInfo) const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|SkipControl")
    FTimerHandle& GetSkipHandler();

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|SkipControl")
    void SetSkipHandler(const FTimerHandle& Handle);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|History")
    const TArray<UDialogueNodeInfo*>& GetDialogueHistory();

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|UI")
    void SetDialogueRecallWidget(UUserWidget* UserWidget);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|UI")
    UUserWidget* GetDialogueRecallWidget() const;
	
	FPlayerCondition GetPlayerEvalCondition() const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|Portrait")
    const TArray<FPortraitInitData> GetInitPortraitDatas() const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|Portrait")
    const TArray<FPortraitActionData> GetPortraitActionDatas() const;

	#if !UE_BUILD_SHIPPING
	void PlayDialogueGraph(UDialogueGraph* DialogueGraph);
	#endif
	
private:
	// 새 실행기의 표시만 반영하며 legacy EndDialogue의 Quest/Save를 호출하지 않는다.
	void HandleBlockPresentation(const FDialoguePresentation& NewPresentation);

	UPROPERTY()
	TObjectPtr<UDialogueBlockRunner> BlockRunner;
	UPROPERTY()
	TObjectPtr<UDialogueCheckpointSession> CheckpointSession;
	UPROPERTY()
	TObjectPtr<UObject> BlockStateProvider;
	UPROPERTY()
	TObjectPtr<UObject> BlockEffectProvider;
	UPROPERTY()
	TObjectPtr<UDialogueDiagnosticRouter> BlockRouter;
	UPROPERTY()
	FDialoguePresentation BlockPresentation;
	bool bUsingBlockRunner = false;

	void CheckDelegates();
	
	void GetDialogueGraph(ESpeakerID NPCID);
	void StartDialogue(UDialogueGraph* DialogueGraph);
	void CreateDialogueUI();
	UDialogueRuntimeNode* GetFirstNode();
	void UpdateCurrentDialogueNode(FGuid NewDialogueNodeGuid);
	void SetCurrentDialogueInfo();
	UDialogueNodeInfo* BuildLocalizedNodeInfo(UDialogueRuntimeNode* RuntimeNode);
	void SetInputSettings(bool bIsShowUI) const;
	void EndDialogue();

	UDialogueRuntimeNode* GetNextNode(const int32 SelectedChoiceIndex);
	TArray<FGuid> GetSelectableChoicesLinkedGuid(UDialogueRuntimeNode* DialogueRuntimeNode) const;
	bool IsPossibleToShowTrueCondition(UDialogueRuntimeNode* BranchNode) const;
	UDialogueRuntimeNode* GetDialogueNode(FGuid DialogueNodeGuid);

	FARFilter GetDialogueGraphAssetFilter(ESpeakerID NPCID, EChapterID ChapterID) const;
	void InitializeDialogueData();
	bool IsCandidateDialogueGraphAsset(const FAssetData& AssetData) const;
	void OnDialogueLoaded();

	FGameplayTagContainer GetPlayerOwnedTags() const;
	int32 GetPlayerLevel() const;
	EChapterID GetCurrentChapter() const;

	void PreloadPortraits();

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|Portrait")
    UTexture2D* GetPortrait(const ESpeakerID NPCID, const EEmoteType EmoteType) const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSubsystem|Portrait")
    const FPortraitData GetPortraitData() const;

	template<typename TEnum>
	FString GetEnumNameString(TEnum EnumValue) const
	{
		static_assert(TIsEnum<TEnum>::Value, "UDialogueSubsystem::GetEnumNameString: TEnum must be an enum type");

		const UEnum* Enum = StaticEnum<TEnum>();
		if (Enum)
		{
			return Enum->GetNameStringByValue(static_cast<int64>(EnumValue));
		}

		UE_LOG(LogTemp, Error, TEXT("UDialogueSubsystem::GetEnumNameString : Enum is nullptr"));
		
		return FString();
	}

	void SaveRelativeDatas() const;
	bool LoadDialogueSaveData();
	void SaveDialogueSaveData() const;
	
	UPROPERTY()
	UDialogueGraph* CurrentDialogueGraph;

	UPROPERTY()
	TArray<UDialogueNodeInfo*> DialogueHistory;	// Dialogue Recall을 위한 캐싱

	UPROPERTY()
	FGuidList ShownDialogueGuids;
	
	UPROPERTY()
	UDialogueNodeInfo* CurrentOngoingDialogueNodeInfo;

	UPROPERTY()
	UDialogueNodeInfo* LocalizedCurrentOngoingDialogueNodeInfo = nullptr;
	
	UPROPERTY()
	FGuid CurrentOngoingNodeGuid;
	
	UPROPERTY()
	TMap<FGuid, UDialogueRuntimeNode*> IdToNodeMap;

	TSharedPtr<FStreamableHandle> CurrentHandle;

	FOnDialogueReady OnDialogueReady;
	
	FOnCurrentDialogueNodeChange OnCurrentDialogueChanged;

    UPROPERTY(BlueprintAssignable, Category = "DialogueKit|DialogueSubsystem|Events")
    FOnDialogueNodeInfoChanged OnDialogueNodeInfoChanged;

    UPROPERTY(BlueprintAssignable, Category = "DialogueKit|DialogueSubsystem|Events")
    FOnDialogueEnd OnDialogueEnded;

    UPROPERTY(BlueprintAssignable, Category = "DialogueKit|DialogueSubsystem|Events")
    FOnStopSkip OnStopSkip;
	
	UPROPERTY()
	FTimerHandle OnShownDialogueSkipTimerHandle;
	
	UPROPERTY()
	TArray<UDialogueGraph*> PossibleDialogueGraphs;

	UPROPERTY()
	TMap<ESpeakerID, EEmoteType> CurrentNPCEmote;

	UPROPERTY()
	TMap<ESpeakerID, FPortraitEmoteIDPair> CachedPortraitEmotePairMap;

    UPROPERTY(BlueprintReadWrite, Category = "DialogueKit|DialogueSubsystem|Portrait", meta = (AllowPrivateAccess = true))
    TMap<ESpeakerID, EPortraitSide> CachedPortraitSideMap;
	
	TSharedPtr<FStreamableHandle> PortraitPreLoadHandle;
	
	UPROPERTY()
	TSoftClassPtr<UUserWidget> DialogueWidgetClass;

	UPROPERTY()
	UUserWidget* DialogueWidget = nullptr;

	UPROPERTY()
	UUserWidget* RecallWidget = nullptr;

	const FString ShownDialogueSaveSlot = TEXT("ShownDialogueSaveSlot");
	const int32 DialogueHistorySaveIndex = 0;

	UPROPERTY()
	class UShownDialogueSaveData* DialogueHistorySaveData = nullptr;
};
