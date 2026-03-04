#pragma once

#include "CoreMinimal.h"
#include "DialogueLocalizationDataAsset.generated.h"

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueLocalizationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueLocalization")
	FGuid NodeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueLocalization")
	FGuid PinId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueLocalization")
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueLocalization")
	FText Value;
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueLocalizationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueLocalization")
	FPrimaryAssetId TargetDialoguePrimaryAssetId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueLocalization")
	FString CultureName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialogueLocalization")
	TArray<FDialogueLocalizationEntry> Entries;
};
