#pragma once

#include "CoreMinimal.h"
#include "DialogueConditionTypes.h"
#include "DialogueStoryDefinition.h"
#include "AssetRegistry/AssetData.h"
#include "DialogueBlockMetadata.generated.h"

UENUM(BlueprintType)
enum class EDialogueBlockAvailability : uint8
{
	Active,
	RestoreOnly
};

// Graph/이미지/음향을 로드하지 않고 읽는 Registry 값이다. 원본은 Block 에셋이다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueBlockMetadata
{
	GENERATED_BODY()
	UPROPERTY()
	int32 MetadataVersion = 1;
	UPROPERTY()
	bool bRequiresManualRewrite = false;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FDialogueBlockReference Block;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FGameplayTag TargetTag;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FDialogueStoryScope StoryScope;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FDialogueConditionSet EntryConditions;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	EDialogueBlockAvailability Availability = EDialogueBlockAvailability::Active;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	bool bDefault = false;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	bool bTopPriority = false;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	int32 Priority = 0;

	bool IsValid() const;
	FString ToRegistryValue() const;
	static bool FromRegistryValue(const FString& Value, FDialogueBlockMetadata& OutMetadata);
	// 복원은 신규 대상/소속 조건과 무관하게 정확 콘텐츠 식별자만 읽는다.
	static bool ReadIdentity(const FAssetData& Asset, FDialogueBlockReference& OutReference);
	static bool ReadAsset(const FAssetData& Asset, FDialogueBlockMetadata& OutMetadata);
	static FName RegistryKey();
	static FName TargetRegistryKey();
};

UCLASS()
class DIALOGUEKIT_API UDialogueMetadataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// 신규 시작/Refresh 시점에만 평가한다. 복원 경로는 이 필터를 호출하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Selection")
	static bool IsEligible(const FDialogueBlockMetadata& Metadata, FGameplayTag Target,
		const UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress,
		UObject* StateProvider, UDialogueDiagnosticRouter* Router);
};
