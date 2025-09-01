// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.h"
#include "Engine/StreamableManager.h"
#include "Subsystems/Subsystem.h"
#include "DialogueSubsystem.generated.h"

DECLARE_DELEGATE_OneParam(FOnDialogueReady, UDialogueGraph*);
DECLARE_DELEGATE_OneParam(FOnCurrentDialogueNodeChange, FGuid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnd);

UCLASS()
class DIALOGUEMAKER_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void ShowDialogue(ENPCID NPCID);
	
	UFUNCTION(BlueprintCallable)
	class UDialogueNodeInfo* GetNextDialogueNodeInfo(const int32 SelectedChoiceIndex = 0, const bool bIsFirstDialogue = false);

	UFUNCTION(BlueprintCallable)
	const UDialogueNodeInfo* GetCurrentDialogueNodeInfo() const;
	
	UFUNCTION(BlueprintCallable, Category = "Choice")
	bool HasChoicesInCurrentDialogue(UDialogueNodeInfo* DialogueNodeInfo) const;
	
	UFUNCTION(BlueprintCallable, Category = "Choice")
	TArray<FText> GetSelectableChoicesText(UDialogueNodeInfo* DialogueNodeInfo) const;

private:
	void GetDialogueGraph(ENPCID NPCID);
	void StartDialogue(UDialogueGraph* DialogueGraph);
	void ShowDialogueUI();
	UDialogueRuntimeNode* GetFirstNode();
	void RefreshCurrentDialogueNode(FGuid NewDialogueNodeGuid);
	void SetCurrentDialogueInfo();
	void SetInputSettings(bool bIsShowUI) const;
	void EndDialogue();

	UDialogueNodeInfo* GetDialogueNodeInfo(FGuid DialogueNodeGuid);
	UDialogueRuntimeNode* GetNextNode(const int32 SelectedChoiceIndex);
	TArray<FGuid> GetSelectableChoicesLinkedGuid(UDialogueRuntimeNode* DialogueRuntimeNode) const;
	bool IsPossibleToShowTrueCondition(UDialogueRuntimeNode* BranchNode) ;
	// void SaveDialogueProgress(FGuid CurrentDialogueGuid);
	UDialogueRuntimeNode* GetDialogueNode(FGuid DialogueNodeGuid);

	FARFilter GetDialogueGraphAssetFilter(ENPCID NPCID, EChapterID ChapterID) const;
	bool IsPossibleToLoadDialogueProgressData();
	void InitializeDialogueData();
	bool IsCandidateDialogueGraphAsset(const FAssetData& AssetData) const;
	void OnDialogueLoaded();
	
	const FString DialogueProgressSaveSlot = TEXT("DialogueProgressSaveSlot");
	const int32 SaveIndex = 0;

	UPROPERTY()
	UDialogueGraph* CurrentDialogueGraph;

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
	FOnDialogueEnd OnDialogueEnded;
	
	UPROPERTY()
	TArray<UDialogueGraph*> PossibleDialogueGraphs;

	UPROPERTY()
	TSoftClassPtr<UUserWidget> DialogueWidgetClass;

	UPROPERTY()
	UUserWidget* DialogueWidget = nullptr;
	
	UPROPERTY()
	class UDialogueProgressSaveData* CachedDialogueProgressSaveData;
};
