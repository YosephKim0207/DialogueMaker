#include "DialogueConditionLibrary.h"
#include "DialogueStateProvider.h"

namespace
{
	// Value와 Count를 변환하지 않고 원래 수치형으로 비교한다.
	template<typename T>
	bool CompareValue(T Current, T Operand, EDialogueComparison Comparison)
	{
		switch (Comparison)
		{
		case EDialogueComparison::Equal: return Current == Operand;
		case EDialogueComparison::NotEqual: return Current != Operand;
		case EDialogueComparison::Less: return Current < Operand;
		case EDialogueComparison::LessOrEqual: return Current <= Operand;
		case EDialogueComparison::Greater: return Current > Operand;
		case EDialogueComparison::GreaterOrEqual: return Current >= Operand;
		default: return false;
		}
	}

	// 조회 성공과 조건 통과를 분리하고 요청한 종류만 조회한다.
	bool EvaluateOne(const FDialogueCondition& Condition, UObject* Provider, bool& OutPassed)
	{
		if (Condition.Key.IsValid() == false || IsValid(Provider) == false ||
			Provider->GetClass()->ImplementsInterface(UDialogueStateProvider::StaticClass()) == false ||
			static_cast<uint8>(Condition.Comparison) > static_cast<uint8>(EDialogueComparison::GreaterOrEqual))
		{
			return false;
		}
		switch (Condition.Key.Kind)
		{
		case EDialogueStateKind::Tag:
		{
			bool Current = false;
			if ((Condition.Comparison != EDialogueComparison::Equal && Condition.Comparison != EDialogueComparison::NotEqual) ||
				IDialogueStateProvider::Execute_QueryDialogueTag(Provider, Condition.Key.TargetId, Condition.Key.Tag, Current) == false)
			{
				return false;
			}
			OutPassed = CompareValue(Current, Condition.bTagOperand, Condition.Comparison);
			return true;
		}
		case EDialogueStateKind::Value:
		{
			double Current = 0.0;
			if (FMath::IsFinite(Condition.ValueOperand) == false ||
				IDialogueStateProvider::Execute_QueryDialogueValue(Provider, Condition.Key.TargetId, Condition.Key.ItemId, Current) == false || FMath::IsFinite(Current) == false)
			{
				return false;
			}
			OutPassed = CompareValue(Current, Condition.ValueOperand, Condition.Comparison);
			return true;
		}
		case EDialogueStateKind::Count:
		{
			int64 Current = 0;
			if (IDialogueStateProvider::Execute_QueryDialogueCount(Provider, Condition.Key.TargetId, Condition.Key.ItemId, Current) == false || Current < 0)
			{
				return false;
			}
			OutPassed = CompareValue(Current, Condition.CountOperand, Condition.Comparison);
			return true;
		}
		default: return false;
		}
	}
}

// 공통 평가 규칙에 따라 순수 조회 결과를 종합하고 오류는 별도 진단한다.
FDialogueConditionResult UDialogueConditionLibrary::EvaluateConditions(const FDialogueConditionSet& ConditionSet,
	UObject* Provider, bool bBranchCondition, const FDialogueDiagnosticContext& Context, UDialogueDiagnosticRouter* Router)
{
	FDialogueConditionResult Result;
	auto Report = [&](FName Code, const FString& Message, bool bBlocked = false)
	{
		FDialogueDiagnostic Diagnostic;
		Diagnostic.Code = Code;
		Diagnostic.Message = Message;
		Diagnostic.Handling = bBlocked ? TEXT("ContinueWithoutBlock") : TEXT("ContinueWithTrue");
		Diagnostic.Context = Context;
		Result.Diagnostics.Add(UDialogueDiagnosticRouter::Dispatch(MoveTemp(Diagnostic), Router));
		Result.bHadErrors = true;
	};
	// 전 항목을 평가하여 앞선 불충족 때문에 뒤의 오류 진단이 사라지지 않게 한다.
	auto EvaluateGroup = [&](const TArray<FDialogueCondition>& Items, bool bAll, bool bBlocked, const TCHAR* Name)
	{
		if (Items.IsEmpty()) return true;
		bool bCombined = bAll;
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			bool bPassed = false;
			if (EvaluateOne(Items[Index], Provider, bPassed) == false)
			{
				Report(TEXT("ConditionLookupFailed"), FString::Printf(TEXT("%s condition %d is invalid or its state query failed."), Name, Index), bBlocked);
				bPassed = !bBlocked;
			}
			bCombined = bAll ? bCombined && bPassed : bCombined || bPassed;
		}
		return bBlocked ? !bCombined : bCombined;
	};
	bool bLegacy = true;
	if (ConditionSet.Combine != EDialogueConditionCombine::All && ConditionSet.Combine != EDialogueConditionCombine::Any)
	{
		Report(TEXT("InvalidConditionCombine"), TEXT("Unknown legacy condition combine mode."));
	}
	else
	{
		bLegacy = EvaluateGroup(ConditionSet.Conditions, ConditionSet.Combine == EDialogueConditionCombine::All, false, TEXT("Legacy"));
	}
	if (ConditionSet.IsEmpty() && bBranchCondition)
	{
		Report(TEXT("EmptyBranchCondition"), TEXT("Branch has no conditions in any group."));
	}
	const bool bRequired = EvaluateGroup(ConditionSet.RequiredConditions, true, false, TEXT("Required"));
	const bool bAny = EvaluateGroup(ConditionSet.AnyConditions, false, false, TEXT("Any"));
	const bool bNotBlocked = EvaluateGroup(ConditionSet.BlockedConditions, false, true, TEXT("Blocked"));
	Result.bPassed = bLegacy && bRequired && bAny && bNotBlocked;
	return Result;
}

// 별도 게임별 표는 정의 제공자가 변환하며 이 함수는 공통 행 표만 읽는다.
bool UDialogueConditionLibrary::ReadDefinitionTable(UDataTable* Table, TArray<FDialogueStateDefinition>& OutDefinitions)
{
	OutDefinitions.Reset();
	if (IsValid(Table) == false || Table->GetRowStruct() != FDialogueStateDefinition::StaticStruct())
	{
		return false;
	}
	TSet<FDialogueStateKey> Keys;
	TArray<FDialogueStateDefinition> Definitions;
	for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
	{
		const FDialogueStateDefinition* Definition = reinterpret_cast<const FDialogueStateDefinition*>(Row.Value);
		if (Definition == nullptr || Definition->Key.IsValid() == false || Keys.Contains(Definition->Key))
		{
			return false;
		}
		Keys.Add(Definition->Key);
		Definitions.Add(*Definition);
	}
	OutDefinitions = MoveTemp(Definitions);
	return true;
}
