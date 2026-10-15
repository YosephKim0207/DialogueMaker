#include "DialogueBlockMetadata.h"
#include "DialogueConditionLibrary.h"
#include "Misc/OutputDevice.h"

// 소속 없음은 유효한 저작 상태지만 신규 후보 평가에서는 제외된다.
bool FDialogueBlockMetadata::IsValid() const
{
	return MetadataVersion == 1 && bRequiresManualRewrite == false && Block.IsValid() && TargetTag.IsValid() &&
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
	return Asset.GetTagValue(RegistryKey(), Value) && FromRegistryValue(Value, OutMetadata);
}

// 메타데이터 버전과 저장 게임 스키마 버전은 서로 독립적이다.
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
