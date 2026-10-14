#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DialogueConditionTypes.h"
#include "DialogueStateProvider.generated.h"

UINTERFACE(BlueprintType)
class DIALOGUEKIT_API UDialogueDefinitionProvider : public UInterface
{
	GENERATED_BODY()
};

class DIALOGUEKIT_API IDialogueDefinitionProvider
{
	GENERATED_BODY()
public:
	// 목록은 정의일 뿐 현재 수량/능력치가 아니다. 게임 DataTable을 공통 행으로 변환한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DialogueKit|Provider")
	bool GetDialogueDefinitions(TArray<FDialogueStateDefinition>& OutDefinitions) const;
	virtual bool GetDialogueDefinitions_Implementation(TArray<FDialogueStateDefinition>& OutDefinitions) const;
};

UINTERFACE(BlueprintType)
class DIALOGUEKIT_API UDialogueStateProvider : public UInterface
{
	GENERATED_BODY()
};

class DIALOGUEKIT_API IDialogueStateProvider
{
	GENERATED_BODY()
public:
	// 반환 false는 조회 오류, 반환 true와 OutValue=false는 정상적인 태그 부재다.
	// 게임은 효과·보상·저장·열람·커서를 변경하지 않는 조회만 구현한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DialogueKit|Provider")
	bool QueryDialogueTag(FName TargetId, FGameplayTag Tag, bool& OutValue) const;
	virtual bool QueryDialogueTag_Implementation(FName TargetId, FGameplayTag Tag, bool& OutValue) const;

	// 현재 레벨/능력치/진행 수치 등을 조회한다. 알 수 없는 키는 false다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DialogueKit|Provider")
	bool QueryDialogueValue(FName TargetId, FName ItemId, double& OutValue) const;
	virtual bool QueryDialogueValue_Implementation(FName TargetId, FName ItemId, double& OutValue) const;

	// 알려진 아이템의 미보유는 true/0이다. 미등록 키/조회 실패는 false로 구분한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DialogueKit|Provider")
	bool QueryDialogueCount(FName TargetId, FName ItemId, int64& OutValue) const;
	virtual bool QueryDialogueCount_Implementation(FName TargetId, FName ItemId, int64& OutValue) const;
};
