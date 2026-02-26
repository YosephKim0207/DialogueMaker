#include "DialogueLocalizationSubsystem.h"
#include "DialogueLocalizationUtility.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

// 서브시스템 초기화 시 엔진의 현재 언어 코드를 읽어 기본 문화권 값을 설정한다.
void UDialogueLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const FCulturePtr CurrentLanguage = FInternationalization::Get().GetCurrentLanguage())
	{
		CurrentCultureCode = CurrentLanguage->GetName();
	}

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::Initialize - %s"), *CurrentCultureCode);
}

// enum 기반 언어를 문화권 코드로 변환해 실제 언어 변경 함수로 위임한다.
bool UDialogueLocalizationSubsystem::SetCurrentLanguage(ELanguage NewLanguage)
{
	return SetCurrentCultureCode(FDialogueLocalizationUtility::ToCultureCode(NewLanguage));
}

// 엔진 국제화 시스템에 문화권 코드를 적용하고 로컬라이제이션 캐시를 초기화한다.
bool UDialogueLocalizationSubsystem::SetCurrentCultureCode(const FString& NewCultureCode)
{
	if (NewCultureCode.IsEmpty())
	{
		return false;
	}

	const bool bSuccess = FInternationalization::Get().SetCurrentLanguageAndLocale(NewCultureCode);
	if (!bSuccess)
	{
		return false;
	}

	CurrentCultureCode = NewCultureCode;
	ClearLocalizationCache();

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::SetCurrentCultureCode - %s"), *CurrentCultureCode);

	return true;
}

// 현재 로컬라이제이션 조회에 사용하는 문화권 코드를 반환한다.
FString UDialogueLocalizationSubsystem::GetCurrentCultureCode() const
{
	return CurrentCultureCode;
}

// 현재 로드된 그래프별 로컬라이제이션 캐시를 모두 비운다.
void UDialogueLocalizationSubsystem::ClearLocalizationCache()
{
	LocalizedCacheByGraph.Reset();
	AccessTickCounter = 0;
}

// 노드 대사 텍스트를 로컬라이즈된 값으로 조회하고, 없으면 원본 텍스트를 반환한다.
FText UDialogueLocalizationSubsystem::ResolveDialogueText(const FPrimaryAssetId& DialogueGraphPrimaryAssetId, const FGuid& NodeGuid, const FText& FallbackText)
{
	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::ResolveDialogueText : Enter"));

	if (!EnsureLocalizationLoaded(DialogueGraphPrimaryAssetId))
	{
		return FallbackText;
	}

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::ResolveDialogueText : Pass EnsureLocalizationLoaded"));

	FDialogueLocalizationCacheEntry* CacheEntry = LocalizedCacheByGraph.Find(DialogueGraphPrimaryAssetId);
	if (CacheEntry == nullptr || CacheEntry->bMissing)
	{
		return FallbackText;
	}

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::ResolveDialogueText : Pass CacheEntry Check"));

	FDialogueLocalizationLookupKey LookupKey;
	LookupKey.NodeGuid = NodeGuid;
	LookupKey.Key = TEXT("DialogueText");

	TouchCacheEntry(*CacheEntry);
	if (const FText* LocalizedText = CacheEntry->LocalizedTexts.Find(LookupKey))
	{
		return *LocalizedText;
	}

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::ResolveDialogueText : Get LocalizedText Fail"));

	return FallbackText;
}

// 노드/핀 조합으로 선택지 텍스트를 로컬라이즈된 값으로 조회하고, 없으면 원본 텍스트를 반환한다.
FText UDialogueLocalizationSubsystem::ResolveResponseText(const FPrimaryAssetId& DialogueGraphPrimaryAssetId, const FGuid& NodeGuid, const FGuid& PinId, const FText& FallbackText)
{
	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::ResolveResponseText : Enter"));

	if (!EnsureLocalizationLoaded(DialogueGraphPrimaryAssetId))
	{
		return FallbackText;
	}

	FDialogueLocalizationCacheEntry* CacheEntry = LocalizedCacheByGraph.Find(DialogueGraphPrimaryAssetId);
	if (CacheEntry == nullptr || CacheEntry->bMissing)
	{
		return FallbackText;
	}

	FDialogueLocalizationLookupKey LookupKey;
	LookupKey.NodeGuid = NodeGuid;
	LookupKey.PinId = PinId;
	LookupKey.Key = TEXT("ResponseText");

	TouchCacheEntry(*CacheEntry);
	if (const FText* LocalizedText = CacheEntry->LocalizedTexts.Find(LookupKey))
	{
		return *LocalizedText;
	}

	return FallbackText;
}

// 임의 문자열을 에셋/패키지 이름으로 안전하게 사용할 수 있는 형태로 정규화한다.
FString UDialogueLocalizationSubsystem::SanitizeForObjectName(const FString& Input)
{
	FString Result;
	Result.Reserve(Input.Len());
	for (const TCHAR Character : Input)
	{
		if (FChar::IsAlnum(Character) || Character == TEXT('_'))
		{
			Result.AppendChar(Character);
		}
		else
		{
			Result.AppendChar(TEXT('_'));
		}
	}

	return Result.IsEmpty() ? TEXT("Unknown") : Result;
}

// 지정한 DialogueGraph의 로컬라이제이션 DataAsset을 로드하고 캐시에 보장한다.
bool UDialogueLocalizationSubsystem::EnsureLocalizationLoaded(const FPrimaryAssetId& DialogueGraphPrimaryAssetId)
{
	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::EnsureLocalizationLoaded : Enter"));

	if (!DialogueGraphPrimaryAssetId.IsValid())
	{
		UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::EnsureLocalizationLoaded : %s"), *DialogueGraphPrimaryAssetId.ToString());
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::EnsureLocalizationLoaded : Pass Id Check"));

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::EnsureLocalizationLoaded : LocalizedCacheByGraph Size %d"), LocalizedCacheByGraph.Num());

	if (FDialogueLocalizationCacheEntry* ExistingEntry = LocalizedCacheByGraph.Find(DialogueGraphPrimaryAssetId))
	{
		// 캐시 히트 시 접근 시각을 갱신해 LRU 우선순위를 유지한다.
		TouchCacheEntry(*ExistingEntry);
		return !ExistingEntry->bMissing;
	}

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::EnsureLocalizationLoaded : Pass Cache Check"));

	FDialogueLocalizationCacheEntry NewEntry;
	const FString ObjectPath = BuildLocalizationObjectPath(DialogueGraphPrimaryAssetId);
	UDialogueLocalizationDataAsset* LocalizationAsset = LoadObject<UDialogueLocalizationDataAsset>(nullptr, *ObjectPath);
	if (LocalizationAsset == nullptr || LocalizationAsset->TargetDialoguePrimaryAssetId != DialogueGraphPrimaryAssetId)
	{
		// 에셋 누락/불일치 케이스를 음수 캐시로 저장해 반복 로드를 방지한다.
		NewEntry.bMissing = true;
		TouchCacheEntry(NewEntry);
		LocalizedCacheByGraph.Add(DialogueGraphPrimaryAssetId, MoveTemp(NewEntry));
		EvictIfNeeded();
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("DialogueLocalizationSubsystem::EnsureLocalizationLoaded : Pass LocalizationAsset Check"));

	for (const FDialogueLocalizationEntry& Entry : LocalizationAsset->Entries)
	{
		// (NodeGuid, PinId, Key) 조합 키 기반 조회 테이블을 구성한다.
		FDialogueLocalizationLookupKey LookupKey;
		LookupKey.NodeGuid = Entry.NodeGuid;
		LookupKey.PinId = Entry.PinId;
		LookupKey.Key = Entry.Key;
		NewEntry.LocalizedTexts.FindOrAdd(LookupKey) = Entry.Value;
	}

	TouchCacheEntry(NewEntry);
	LocalizedCacheByGraph.Add(DialogueGraphPrimaryAssetId, MoveTemp(NewEntry));
	EvictIfNeeded();
	return true;
}

// 캐시 엔트리 접근 시각을 증가시켜 LRU 제거 기준으로 사용한다.
void UDialogueLocalizationSubsystem::TouchCacheEntry(FDialogueLocalizationCacheEntry& CacheEntry)
{
	CacheEntry.LastAccessTick = ++AccessTickCounter;
}

// 캐시 최대 개수를 초과하면 가장 오래 사용되지 않은 항목부터 제거한다.
void UDialogueLocalizationSubsystem::EvictIfNeeded()
{
	while (LocalizedCacheByGraph.Num() > MaxCachedGraphs)
	{
		uint64 OldestTick = MAX_uint64;
		FPrimaryAssetId OldestAssetId;
		bool bFoundOldest = false;

		for (const TPair<FPrimaryAssetId, FDialogueLocalizationCacheEntry>& Pair : LocalizedCacheByGraph)
		{
			if (Pair.Value.LastAccessTick < OldestTick)
			{
				OldestTick = Pair.Value.LastAccessTick;
				OldestAssetId = Pair.Key;
				bFoundOldest = true;
			}
		}

		if (!bFoundOldest)
		{
			break;
		}

		LocalizedCacheByGraph.Remove(OldestAssetId);
	}
}

// 그래프 PrimaryAssetId와 현재 문화권을 기준으로 로컬라이제이션 에셋 경로를 구성한다.
FString UDialogueLocalizationSubsystem::BuildLocalizationObjectPath(const FPrimaryAssetId& DialogueGraphPrimaryAssetId) const
{
	const FString GraphAssetName = SanitizeForObjectName(DialogueGraphPrimaryAssetId.PrimaryAssetName.ToString());
	const FString CultureSegment = SanitizeForObjectName(CurrentCultureCode);
	const FString AssetName = FString::Printf(TEXT("DL_%s_%s"), *GraphAssetName, *CultureSegment);
	const FString PackagePath = FString::Printf(TEXT("/Game/DialogueLocalization/%s/%s"), *CultureSegment, *AssetName);
	return PackagePath + TEXT(".") + AssetName;
}
