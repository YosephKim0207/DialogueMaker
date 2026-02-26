#pragma once

#include "CoreMinimal.h"
#include "DialogueLocalizationDataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DialogueLocalizationSubsystem.generated.h"

UENUM(BlueprintType)
enum class ELanguage : uint8
{
	English UMETA(DisplayName = "English"),
	ChineseSimplified UMETA(DisplayName = "Chinese (Simplified)"),
	German UMETA(DisplayName = "German"),
	Spanish UMETA(DisplayName = "Spanish"),
	BrazilianPortuguese UMETA(DisplayName = "Brazilian Portuguese"),
	Russian UMETA(DisplayName = "Russian"),
	Korean UMETA(DisplayName = "Korean"),
	French UMETA(DisplayName = "French"),
	Italian UMETA(DisplayName = "Italian"),
	Turkish UMETA(DisplayName = "Turkish"),
	Polish UMETA(DisplayName = "Polish")
};

USTRUCT()
struct FDialogueLocalizationLookupKey
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid NodeGuid;

	UPROPERTY()
	FGuid PinId;

	UPROPERTY()
	FName Key;

	bool operator==(const FDialogueLocalizationLookupKey& Other) const
	{
		return NodeGuid == Other.NodeGuid && PinId == Other.PinId && Key == Other.Key;
	}
};

FORCEINLINE uint32 GetTypeHash(const FDialogueLocalizationLookupKey& Value)
{
	return HashCombine(HashCombine(GetTypeHash(Value.NodeGuid), GetTypeHash(Value.PinId)), GetTypeHash(Value.Key));
}

UCLASS()
class DIALOGUEKIT_API UDialogueLocalizationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Localization")
	bool SetCurrentLanguage(ELanguage NewLanguage);

	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Localization")
	bool SetCurrentCultureCode(const FString& NewCultureCode);

	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Localization")
	FString GetCurrentCultureCode() const;

	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Localization")
	void ClearLocalizationCache();

	FText ResolveDialogueText(const FPrimaryAssetId& DialogueGraphPrimaryAssetId, const FGuid& NodeGuid, const FText& FallbackText);
	FText ResolveResponseText(const FPrimaryAssetId& DialogueGraphPrimaryAssetId, const FGuid& NodeGuid, const FGuid& PinId, const FText& FallbackText);

	// 언어 enum을 문화권 코드 문자열로 변환한다.
	static FString ToCultureCode(ELanguage Language);

private:
	struct FDialogueLocalizationCacheEntry
	{
		bool bMissing = false;
		uint64 LastAccessTick = 0;
		TMap<FDialogueLocalizationLookupKey, FText> LocalizedTexts;
	};

	static FString SanitizeForObjectName(const FString& Input);

	bool EnsureLocalizationLoaded(const FPrimaryAssetId& DialogueGraphPrimaryAssetId);
	void TouchCacheEntry(FDialogueLocalizationCacheEntry& CacheEntry);
	void EvictIfNeeded();
	FString BuildLocalizationObjectPath(const FPrimaryAssetId& DialogueGraphPrimaryAssetId) const;

private:
	UPROPERTY()
	FString CurrentCultureCode = TEXT("en-US");

	UPROPERTY()
	int32 MaxCachedGraphs = 64;

	uint64 AccessTickCounter = 0;
	TMap<FPrimaryAssetId, FDialogueLocalizationCacheEntry> LocalizedCacheByGraph;
};
