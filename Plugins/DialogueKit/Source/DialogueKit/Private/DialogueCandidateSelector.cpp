#include "DialogueCandidateSelector.h"

// 같은 우선순위 후보는 FRandomStream의 균등 정수 추첨을 사용한다.
UDialogueCandidateSelector::UDialogueCandidateSelector()
{
	Random.GenerateNewSeed();
}

// 게임이 새 대상의 세션을 시작할 때 이전 임시 소진 자료를 버린다.
bool UDialogueCandidateSelector::BeginSession(FGameplayTag Target)
{
	if (Target.IsValid() == false)
	{
		return false;
	}
	if (SessionTarget != Target)
	{
		EndSession();
		SessionTarget = Target;
	}
	return true;
}

// 세션 종료는 Block 정상 종료와 다르다. 저장 이력은 이 클래스에 없다.
void UDialogueCandidateSelector::EndSession()
{
	SessionTarget = FGameplayTag();
	Selected = FDialogueBlockCandidate();
	CompletedOrdinary.Reset();
	CompletedDefaults.Reset();
	PreviousBlock.Invalidate();
	bHasSelection = false;
}

// 난수 소스만 교체하며 세션 진행을 변경하지 않는다.
void UDialogueCandidateSelector::SetRandomSeed(int32 Seed)
{
	Random.Initialize(Seed);
}

// 요청 순간의 조건으로 후보를 고정한다. 비동기 로드 완료 때 다시 평가하지 않는다.
FDialogueSelectionResult UDialogueCandidateSelector::Select(const TArray<FDialogueBlockCandidate>& Candidates,
	const UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress, UObject* Provider, UDialogueDiagnosticRouter* Router)
{
	FDialogueSelectionResult Result;
	bHasSelection = false;
	if (SessionTarget.IsValid() == false)
	{
		Result.ErrorCode = TEXT("MissingDialogueSessionTarget");
		return Result;
	}
	TArray<const FDialogueBlockCandidate*> Top;
	TArray<const FDialogueBlockCandidate*> Ordinary;
	TArray<const FDialogueBlockCandidate*> Defaults;
	TSet<FDialogueBlockReference> References;
	TSet<FGuid> ActiveIds;
	for (const FDialogueBlockCandidate& Candidate : Candidates)
	{
		const FDialogueBlockMetadata& Metadata = Candidate.Metadata;
		if (Metadata.TargetTag != SessionTarget || Metadata.Availability != EDialogueBlockAvailability::Active)
		{
			continue;
		}
		if (Candidate.AssetPath.IsValid() == false || Metadata.IsValid() == false || References.Contains(Metadata.Block) || ActiveIds.Contains(Metadata.Block.BlockId))
		{
			Result.ErrorCode = TEXT("InvalidOrAmbiguousBlockCatalog");
			return Result;
		}
		References.Add(Metadata.Block);
		ActiveIds.Add(Metadata.Block.BlockId);
		if (UDialogueMetadataLibrary::IsEligible(Metadata, SessionTarget, Story, Progress, Provider, Router) == false)
		{
			continue;
		}
		if (Metadata.bTopPriority)
		{
			Top.Add(&Candidate);
		}
		else if (Metadata.bDefault)
		{
			Defaults.Add(&Candidate);
		}
		else if (CompletedOrdinary.Contains(Metadata.Block.BlockId) == false && Metadata.Block.BlockId != PreviousBlock)
		{
			Ordinary.Add(&Candidate);
		}
	}
	TArray<const FDialogueBlockCandidate*> Pool = Top.IsEmpty() ? Ordinary : Top;
	if (Pool.IsEmpty() == false)
	{
		int32 Highest = MIN_int32;
		for (const FDialogueBlockCandidate* Candidate : Pool)
		{
			Highest = FMath::Max(Highest, Candidate->Metadata.Priority);
		}
		Pool.RemoveAll([&](const FDialogueBlockCandidate* Candidate) { return Candidate->Metadata.Priority != Highest; });
	}
	else
	{
		Pool = Defaults;
		Pool.RemoveAll([&](const FDialogueBlockCandidate* Candidate) { return CompletedDefaults.Contains(Candidate->Metadata.Block.BlockId); });
		if (Pool.IsEmpty() && Defaults.IsEmpty() == false)
		{
			CompletedDefaults.Reset();
			Pool = Defaults;
		}
	}
	if (Top.IsEmpty() && Pool.Num() > 1)
	{
		Pool.RemoveAll([&](const FDialogueBlockCandidate* Candidate) { return Candidate->Metadata.Block.BlockId == PreviousBlock; });
	}
	if (Pool.IsEmpty())
	{
		Result.ErrorCode = TEXT("NoEligibleDialogueBlock");
		return Result;
	}
	Selected = *Pool[Random.RandRange(0, Pool.Num() - 1)];
	bHasSelection = true;
	Result.bSelected = true;
	Result.Candidate = Selected;
	return Result;
}

// 최우선은 조건이 남으면 다시 노출할 수 있다. 일반/기본은 세션 안에서 정상 완료를 기록한다.
bool UDialogueCandidateSelector::RecordCompleted(FDialogueBlockReference Block)
{
	if (bHasSelection == false || (Block == Selected.Metadata.Block) == false)
	{
		return false;
	}
	PreviousBlock = Block.BlockId;
	if (Selected.Metadata.bDefault)
	{
		CompletedDefaults.Add(Block.BlockId);
	}
	else if (Selected.Metadata.bTopPriority == false)
	{
		CompletedOrdinary.Add(Block.BlockId);
	}
	bHasSelection = false;
	return true;
}

// 노출 기록과 정상 완료 소진을 분리하여 중단된 일반 대사도 직전 후보에서는 제외한다.
bool UDialogueCandidateSelector::RecordExposed(FDialogueBlockReference Block)
{
	if (bHasSelection == false || (Block == Selected.Metadata.Block) == false)
	{
		return false;
	}
	PreviousBlock = Block.BlockId;
	return true;
}
