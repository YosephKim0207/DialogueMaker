#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueGraph.h"
#include "DialogueCheckpointContract.h"
#include "DialogueEffectContract.h"
#include "DialogueReplacement.generated.h"

UENUM(BlueprintType)
enum class EDialogueCorrectionMode : uint8 { None, GameCorrection, SelectiveRollback };

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueReplacementRule
{
	GENERATED_BODY()
	// 출시 후 의미를 바꾸면 새 RuleId를 지정한다. 같은 ID의 다른 payload는 충돌이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") FGuid RuleId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") FGuid SourceBlockId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement", meta = (ClampMin = "1")) int32 FirstVersion = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement", meta = (ClampMin = "1")) int32 LastVersion = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") TSoftObjectPtr<UDialogueGraph> Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") FDialogueBlockReference TargetReference;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") EDialogueCorrectionMode Mode = EDialogueCorrectionMode::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") FName GameRule;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") FGuid RequiredPassedLine;
	// 이 노드의 모든 성공 occurrence를 원래 발생 순서로 게임에 전달한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") TArray<FGuid> EffectNodeIds;
	bool Matches(FDialogueBlockReference Reference) const;
	bool Validate(FName& OutError) const;
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueReplacementMap : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replacement") TArray<FDialogueReplacementRule> Rules;
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Restore") bool Validate(FName& OutError) const;
	// 유효한 map에 일치 없음은 정상 exact 복원이며 오류와 구별한다.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
	bool Resolve(FDialogueBlockReference Source, FDialogueReplacementRule& OutRule, bool& bFound, FName& OutError) const;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCorrectionRequest
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Correction") FGuid CorrectionId;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Correction") FString RuleFingerprint;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Correction") FName GameRule;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Correction") EDialogueCorrectionMode Mode = EDialogueCorrectionMode::None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Correction") FDialogueCheckpointSnapshot Source;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Correction") TArray<FDialogueEffectEvidence> Effects;
};

UINTERFACE(BlueprintType)
class DIALOGUEKIT_API UDialogueCorrectionProvider : public UInterface { GENERATED_BODY() };
class DIALOGUEKIT_API IDialogueCorrectionProvider
{
	GENERATED_BODY()
public:
	// 동기 메모리 transaction: 전량 성공/실패 무변경. 실제 게임 원장과 출처를 대조한다.
	// 같은 ID+같은 내용은 재전달, 다른 내용은 충돌. 다른 경로의 변경을 보존한다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DialogueKit|Restore")
	FDialogueEffectResult CorrectDialogueState(const FDialogueCorrectionRequest& Request);
	virtual FDialogueEffectResult CorrectDialogueState_Implementation(const FDialogueCorrectionRequest& Request);
};

class DIALOGUEKIT_API FDialogueReplacement
{
public:
	// 선택적 보정은 보존한 정확 원본 Graph의 노드 ID와 완전한 현재 실행 증거를 요구한다.
	static bool BuildCorrection(const FDialogueCheckpointSnapshot& Source, const FDialogueReplacementRule& Rule, const UDialogueGraph* SourceGraph, FDialogueCorrectionRequest& OutRequest, bool& bNeedsCorrection, FName& OutError);
};
