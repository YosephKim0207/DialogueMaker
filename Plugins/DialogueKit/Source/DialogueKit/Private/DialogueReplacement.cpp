#include "DialogueReplacement.h"

// 원본 콘텐츠 버전의 양 끝을 포함한다.
bool FDialogueReplacementRule::Matches(FDialogueBlockReference Reference) const
{
	return Reference.BlockId == SourceBlockId && Reference.ContentVersion >= FirstVersion && Reference.ContentVersion <= LastVersion;
}

// 잘못된 범위/ID와 보정 종류를 실행 전에 거부한다.
bool FDialogueReplacementRule::Validate(FName& OutError) const
{
	OutError = TEXT("InvalidReplacementRule");
	if (RuleId.IsValid() == false || SourceBlockId.IsValid() == false || FirstVersion < 1 || LastVersion < FirstVersion || Target.IsNull() ||
		TargetReference.IsValid() == false || Matches(TargetReference) || uint8(Mode) > uint8(EDialogueCorrectionMode::SelectiveRollback)) return false;
	if (Mode != EDialogueCorrectionMode::None && GameRule.IsNone()) return false;
	if (Mode != EDialogueCorrectionMode::SelectiveRollback && (RequiredPassedLine.IsValid() || EffectNodeIds.IsEmpty() == false)) return false;
	TSet<FGuid> Unique;
	for (FGuid Id : EffectNodeIds)
	{
		if (Id.IsValid() == false || Unique.Contains(Id)) return false;
		Unique.Add(Id);
	}
	if (Mode == EDialogueCorrectionMode::SelectiveRollback && Unique.IsEmpty()) return false;
	OutError = NAME_None;
	return true;
}

// 범위 중첩과 교체 순환은 마지막 저장이 반복 교체되는 원인이므로 거부한다.
bool UDialogueReplacementMap::Validate(FName& OutError) const
{
	for (int32 A = 0; A < Rules.Num(); ++A)
	{
		if (Rules[A].Validate(OutError) == false) return false;
		for (int32 B = 0; B < A; ++B)
		{
			if (Rules[A].RuleId == Rules[B].RuleId || (Rules[A].SourceBlockId == Rules[B].SourceBlockId &&
				Rules[A].FirstVersion <= Rules[B].LastVersion && Rules[B].FirstVersion <= Rules[A].LastVersion))
			{
				OutError = TEXT("AmbiguousReplacementRules");
				return false;
			}
		}
		TSet<int32> Visited;
		int32 Current = A;
		while (Current != INDEX_NONE)
		{
			if (Visited.Contains(Current))
			{
				OutError = TEXT("ReplacementCycle");
				return false;
			}
			Visited.Add(Current);
			const FDialogueBlockReference Next = Rules[Current].TargetReference;
			Current = Rules.IndexOfByPredicate([&](const FDialogueReplacementRule& Rule) { return Rule.Matches(Next); });
		}
	}
	OutError = NAME_None;
	return true;
}

// 일치한 명시적 규칙 하나만 적용한다. 대상의 다른 규칙을 같은 호출에서 재귀 실행하지 않는다.
bool UDialogueReplacementMap::Resolve(FDialogueBlockReference Source, FDialogueReplacementRule& OutRule, bool& bFound, FName& OutError) const
{
	bFound = false;
	if (Validate(OutError) == false) return false;
	if (const FDialogueReplacementRule* Rule = Rules.FindByPredicate([&](const FDialogueReplacementRule& Item) { return Item.Matches(Source); }))
	{
		OutRule = *Rule;
		bFound = true;
	}
	return true;
}

// 미구현 게임 보정은 오류이며 성공으로 간주하지 않는다.
FDialogueEffectResult IDialogueCorrectionProvider::CorrectDialogueState_Implementation(const FDialogueCorrectionRequest& Request)
{
	FDialogueEffectResult Result;
	Result.ErrorCode = TEXT("CorrectionProviderNotImplemented");
	return Result;
}

// 누적 열람을 현재 실행의 효과 근거로 사용하지 않는다.
bool FDialogueReplacement::BuildCorrection(const FDialogueCheckpointSnapshot& Source, const FDialogueReplacementRule& Rule, const UDialogueGraph* SourceGraph, FDialogueCorrectionRequest& OutRequest, bool& bNeedsCorrection, FName& OutError)
{
	bNeedsCorrection = false;
	if (Source.Validate(OutError) == false || Rule.Validate(OutError) == false) return false;
	OutError = TEXT("ReplacementDoesNotMatchActiveSave");
	if (Rule.Matches(Source.Resume.Block) == false || (Source.Resume.Kind != EDialogueResumeKind::BeforeNode && Source.Resume.Kind != EDialogueResumeKind::BlockStart)) return false;
	FDialogueCorrectionRequest Request;
	Request.Source = Source;
	Request.Mode = Rule.Mode;
	Request.GameRule = Rule.GameRule;
	FString Payload;
	FDialogueReplacementRule::StaticStruct()->ExportText(Payload, &Rule, nullptr, nullptr, PPF_None, nullptr);
	Request.RuleFingerprint = FGuid::NewDeterministicGuid(Payload).ToString(EGuidFormats::Digits);
	Request.CorrectionId = FGuid::NewDeterministicGuid(Source.CheckpointId.ToString() + Rule.RuleId.ToString());
	bNeedsCorrection = Rule.Mode == EDialogueCorrectionMode::GameCorrection;
	if (Rule.Mode == EDialogueCorrectionMode::SelectiveRollback)
	{
		OutError = TEXT("IncompleteRollbackEvidence");
		if (Source.SchemaVersion != 2 || Source.RunEvidence.bComplete == false) return false;
		OutError = TEXT("RollbackSourceGraphUnavailable");
		if (IsValid(SourceGraph) == false || IsValid(SourceGraph->Graph) == false || (SourceGraph->GetBlockReference() == Source.Resume.Block) == false) return false;
		TArray<FString> Issues;
		if (SourceGraph->Graph->ValidateIdentifiers(Issues) == false) return false;
		OutError = TEXT("InvalidRollbackNode");
		auto FindNode = [&](FGuid Id) -> const UDialogueRuntimeNode*
		{
			const auto* Found = SourceGraph->Graph->Nodes.FindByPredicate([&](const UDialogueRuntimeNode* Node) { return Node && Node->NodeGuid == Id; });
			return Found ? *Found : nullptr;
		};
		if (Rule.RequiredPassedLine.IsValid())
		{
			const UDialogueRuntimeNode* Line = FindNode(Rule.RequiredPassedLine);
			if (Line == nullptr || Line->DialogueNodeType != EDialogueType::LineNode) return false;
		}
		for (FGuid Id : Rule.EffectNodeIds)
		{
			const UDialogueRuntimeNode* Node = FindNode(Id);
			if (Node == nullptr || (Node->DialogueNodeType != EDialogueType::ChoiceNode && Node->DialogueNodeType != EDialogueType::StateChangeNode)) return false;
		}
		if (Rule.RequiredPassedLine.IsValid() == false || Source.RunEvidence.PassedLines.Contains(Rule.RequiredPassedLine))
		{
			for (const FDialogueEffectEvidence& Effect : Source.RunEvidence.Effects)
			{
				if (Rule.EffectNodeIds.Contains(Effect.NodeGuid)) Request.Effects.Add(Effect);
			}
			bNeedsCorrection = Request.Effects.IsEmpty() == false;
		}
	}
	OutRequest = MoveTemp(Request);
	OutError = NAME_None;
	return true;
}

#if WITH_EDITOR
// Details에서 신규 항목을 만들 때만 누락 ID를 부여한다. 로드/저장 시 기존 규칙을 바꾸지 않는다.
void UDialogueReplacementMap::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	for (FDialogueReplacementRule& Rule : Rules)
	{
		if (Rule.RuleId.IsValid() == false) Rule.RuleId = FGuid::NewGuid();
	}
}
#endif
