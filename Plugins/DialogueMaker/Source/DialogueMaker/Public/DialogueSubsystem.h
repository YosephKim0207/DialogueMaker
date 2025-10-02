// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.h"
#include "DialogueNodeInfo.h"
#include "ShownDialogueSaveData.h"
#include "DialogueConditionEvalCriteria.h"
#include "Engine/StreamableManager.h"
#include "Portrait.h"
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
class DIALOGUEMAKER_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UDialogueSubsystem* Get(const UObject* WorldContextObject);
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void BeginDialogue(ESpeakerID SpeakerID);
	
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	class UDialogueNodeInfo* ProgressNextDialogue(const int32 SelectedChoiceIndex = 0, const bool bIsFirstDialogue = false);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	const UDialogueNodeInfo* GetCurrentDialogueNodeInfo() const;
	
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool HasChoicesInCurrentDialogue(UDialogueNodeInfo* DialogueNodeInfo) const;
	
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void GetSelectableChoiceTexts(UDialogueNodeInfo* DialogueNodeInfo, TArray<FText>& OutSelectableChoiceTexts, TArray<int32>& OutSelectableChoiceOriginalIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Shown Dialogue")
	void MakeCurrentDialogueNodeToShown();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool IsAlreadyShownDialogue(UDialogueNodeInfo* DialogueNodeInfo) const;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	FTimerHandle& GetSkipHandler();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetSkipHandler(const FTimerHandle& Handle);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	const TArray<UDialogueNodeInfo*>& GetDialogueHistory();

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetDialogueRecallWidget(UUserWidget* UserWidget);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	UUserWidget* GetDialogueRecallWidget() const;
	
	FPlayerCondition GetPlayerEvalCondition() const;

	UFUNCTION(BlueprintCallable, Category = "Portrait")
	const TArray<FPortraitInitData> GetInitPortraitDatas() const;

	UFUNCTION(BlueprintCallable, Category = "Portrait")
	const TArray<FPortraitActionData> GetPortraitActionDatas() const;

	#if !UE_BUILD_SHIPPING
	void PlayDialogueGraph(UDialogueGraph* DialogueGraph);
	#endif
	
private:
	void CheckDelegates();
	
	void GetDialogueGraph(ESpeakerID NPCID);
	void StartDialogue(UDialogueGraph* DialogueGraph);
	void CreateDialogueUI();
	UDialogueRuntimeNode* GetFirstNode();
	void UpdateCurrentDialogueNode(FGuid NewDialogueNodeGuid);
	void SetCurrentDialogueInfo();
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

	UFUNCTION(BlueprintCallable, Category = "Portrait")
	UTexture2D* GetPortrait(const ESpeakerID NPCID, const EEmoteType EmoteType) const;

	UFUNCTION(BlueprintCallable, Category = "Portrait")
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
	FGuid CurrentOngoingNodeGuid;
	
	UPROPERTY()
	TMap<FGuid, UDialogueRuntimeNode*> IdToNodeMap;

	TSharedPtr<FStreamableHandle> CurrentHandle;

	FOnDialogueReady OnDialogueReady;
	
	FOnCurrentDialogueNodeChange OnCurrentDialogueChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueNodeInfoChanged OnDialogueNodeInfoChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueEnd OnDialogueEnded;

	UPROPERTY(BlueprintAssignable)
	FOnStopSkip OnStopSkip;
	
	UPROPERTY()
	FTimerHandle OnShownDialogueSkipTimerHandle;
	
	UPROPERTY()
	TArray<UDialogueGraph*> PossibleDialogueGraphs;

	UPROPERTY()
	TMap<ESpeakerID, EEmoteType> CurrentNPCEmote;

	UPROPERTY()
	TMap<ESpeakerID, FPortraitEmoteIDPair> CachedPortraitEmotePairMap;

	UPROPERTY(BlueprintReadWrite, Category = "Portrait", meta = (AllowPrivateAccess = true))
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
