#include "DialogueBlockMetadata.h"
#include "DialogueConditionLibrary.h"
#include "Misc/OutputDevice.h"

// 소속 없음은 유효한 저작 상태지만 신규 후보 평가에서는 제외된다.
bool FDialogueBlockMetadata::IsValid() const
{
	if (!HasKnownGraphType()) return false;
	return MetadataVersion == 3 && bRetiredFilterAsset == false && Block.IsValid() && TargetTag.IsValid() &&
		(Availability == EDialogueBlockAvailability::Active || Availability == EDialogueBlockAvailability::RestoreOnly) &&
		uint8(StoryScope.Mode) <= uint8(EDialogueStoryScope::Scene) && (bDefault && bTopPriority) == false;
}

// int64 Count를 부동소수점 JSON으로 변환하지 않고 UE 구조체 텍스트로 보존한다.
FString FDialogueBlockMetadata::ToRegistryValue() const
{
	FString Value;
	StaticStruct()->ExportText(Value, this, nullptr, nullptr, PPF_None, nullptr);
	return Value;
}

// 파싱 실패 시 출력값을 변경하지 않으며 에셋 로드를 호출하지 않는다.
bool FDialogueBlockMetadata::FromRegistryValue(const FString& Value, FDialogueBlockMetadata& OutMetadata)
{
	if (Value.IsEmpty())
	{
		return false;
	}
	FDialogueBlockMetadata Parsed;
	Parsed.MetadataVersion = 0;
	FStringOutputDevice Errors;
	const TCHAR* End = StaticStruct()->ImportText(*Value, &Parsed, nullptr, PPF_None, &Errors, TEXT("DialogueBlockMetadata"));
	if (End == nullptr || *End != 0 || Errors.IsEmpty() == false || Parsed.IsValid() == false)
	{
		return false;
	}
	OutMetadata = MoveTemp(Parsed);
	return true;
}

// Registry에 기록한 값만 읽는다. GetAsset/TryLoad를 사용하지 않는다.
bool FDialogueBlockMetadata::ReadAsset(const FAssetData& Asset, FDialogueBlockMetadata& OutMetadata)
{
	FString Value;
	if (!Asset.GetTagValue(RegistryKey(), Value) || !FromRegistryValue(Value, OutMetadata)) return false;
	return true;
}

// 색인 키 이름은 유지하며 payload 버전 및 저장 게임 스키마 버전과 구분한다.
FName FDialogueBlockMetadata::RegistryKey()
{
	return TEXT("DialogueKit.MetadataV1");
}

// AssetRegistry의 정확한 태그 값 검색에 사용하는 경량 색인이다.
FName FDialogueBlockMetadata::TargetRegistryKey()
{
	return TEXT("DialogueKit.Target");
}

// 현재 값은 게임 제공자가 소유하며 Registry에는 조건 정의만 들어 있다.
bool UDialogueMetadataLibrary::IsEligible(const FDialogueBlockMetadata& Metadata, FGameplayTag Target,
	const UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress,
	UObject* StateProvider, UDialogueDiagnosticRouter* Router)
{
	if (Metadata.IsValid() == false || Target.IsValid() == false || Metadata.TargetTag != Target || Metadata.Availability != EDialogueBlockAvailability::Active)
	{
		return false;
	}
	const FDialogueStoryScopeResult Scope = UDialogueStoryLibrary::EvaluateScope(Story, Metadata.StoryScope, Progress);
	if (Scope.bValid == false)
	{
		FDialogueDiagnostic Diagnostic;
		Diagnostic.Code = Scope.ErrorCode;
		Diagnostic.Context.Block = Metadata.Block;
		Diagnostic.Message = TEXT("Block 소속 조건을 평가하지 못했습니다.");
		Diagnostic.Handling = TEXT("ExcludeFromNewCandidates");
		UDialogueDiagnosticRouter::Dispatch(Diagnostic, Router);
	}
	if (Scope.bValid == false || Scope.bMatches == false)
	{
		return false;
	}
	FDialogueDiagnosticContext Context;
	Context.Block = Metadata.Block;
	return UDialogueConditionLibrary::EvaluateConditions(Metadata.EntryConditions, StateProvider, false, Context, Router).bPassed;
}

// 출시된 복원 전용/소속 없는 콘텐츠도 식별하며 신규 자격은 평가하지 않는다.
bool FDialogueBlockMetadata::ReadIdentity(const FAssetData& Asset, FDialogueBlockReference& OutReference)
{
	FString Id;
	FString Version;
	const bool bHasId = Asset.GetTagValue(FName(TEXT("DialogueKit.BlockId")), Id);
	const bool bHasVersion = Asset.GetTagValue(FName(TEXT("DialogueKit.ContentVersion")), Version);
	if (bHasId || bHasVersion)
	{
		FDialogueBlockReference Reference;
		if (bHasId == false || bHasVersion == false || FGuid::ParseExact(Id, EGuidFormats::Digits, Reference.BlockId) == false ||
			LexTryParseString(Reference.ContentVersion, *Version) == false || Reference.IsValid() == false) return false;
		OutReference = Reference;
		return true;
	}
	// 독립 식별자 태그가 없는 경우 현재 통합 형식의 식별자를 읽는다.
	FString Value;
	if (Asset.GetTagValue(RegistryKey(), Value) == false || Value.IsEmpty()) return false;
	FDialogueBlockMetadata Parsed;
	Parsed.MetadataVersion = 0;
	FStringOutputDevice Errors;
	const TCHAR* End = StaticStruct()->ImportText(*Value, &Parsed, nullptr, PPF_None, &Errors, TEXT("DialogueRestoreIdentity"));
	if (End == nullptr || *End != 0 || Errors.IsEmpty() == false || Parsed.MetadataVersion != 3 || Parsed.Block.IsValid() == false) return false;
	OutReference = Parsed.Block;
	return true;
}

// 손상된 분류는 정상 Quest로 표시하지 않는다.
bool FDialogueBlockMetadata::HasKnownGraphType() const
{
	return DialogueGraphType == EDialogueGraphType::Quest || DialogueGraphType == EDialogueGraphType::Story || DialogueGraphType == EDialogueGraphType::DailyChat;
}
