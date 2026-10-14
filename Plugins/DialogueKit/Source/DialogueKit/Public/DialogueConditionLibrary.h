#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialogueConditionTypes.h"
#include "DialogueConditionLibrary.generated.h"

UCLASS()
class DIALOGUEKIT_API UDialogueConditionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// 오류 항목만 True로 대체한 뒤 나머지와 AND/OR 집계한다. 효과 실행 실패에는 적용하지 않는다.
	// bBranchCondition은 빈 Branch 경고에만 사용하며 일반 빈 조건은 정상 True다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Conditions")
	static FDialogueConditionResult EvaluateConditions(const FDialogueConditionSet& ConditionSet, UObject* Provider,
		bool bBranchCondition, const FDialogueDiagnosticContext& Context, UDialogueDiagnosticRouter* Router);

	// 공통 행 DataTable을 검증하여 정의 목록으로 읽는다. 잘못된 표/중복이면 false이며 출력은 빈 배열이다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Conditions")
	static bool ReadDefinitionTable(UDataTable* Table, TArray<FDialogueStateDefinition>& OutDefinitions);
};
