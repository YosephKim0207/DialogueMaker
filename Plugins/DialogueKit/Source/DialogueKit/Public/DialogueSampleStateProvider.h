#pragma once

#include "CoreMinimal.h"
#include "DialogueStateProvider.h"
#include "DialogueSampleStateProvider.generated.h"

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueSampleState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Sample")
	FDialogueStateKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Sample")
	bool bTagValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Sample")
	double Value = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Sample")
	int64 Count = 0;
};

// 플러그인 단독 검증용 메모리 구현이다. 게임 제공자 누락 시 자동 대체하지 않는다.
UCLASS(BlueprintType, Blueprintable)
class DIALOGUEKIT_API UDialogueSampleStateProvider : public UObject, public IDialogueStateProvider, public IDialogueDefinitionProvider
{
	GENERATED_BODY()
public:
	// 정의는 별도로 등록하며 중복/잘못된 키는 false로 거부한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	bool AddDefinition(const FDialogueStateDefinition& Definition);

	// 샘플 상태만 설정한다. 게임 인벤토리/저장에는 접근하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	bool SetSampleState(const FDialogueSampleState& NewState);

	virtual bool GetDialogueDefinitions_Implementation(TArray<FDialogueStateDefinition>& OutDefinitions) const override;
	virtual bool QueryDialogueTag_Implementation(FName TargetId, FGameplayTag Tag, bool& OutValue) const override;
	virtual bool QueryDialogueValue_Implementation(FName TargetId, FName ItemId, double& OutValue) const override;
	virtual bool QueryDialogueCount_Implementation(FName TargetId, FName ItemId, int64& OutValue) const override;

protected:
	// 알려진 항목의 미설정 값은 샘플 기본값 0/false다.
	bool GetSampleState(const FDialogueStateKey& Key, FDialogueSampleState& OutState) const;

	UPROPERTY()
	TArray<FDialogueStateDefinition> Definitions;

	UPROPERTY()
	TArray<FDialogueSampleState> States;
};
