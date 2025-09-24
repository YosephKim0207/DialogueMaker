// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSequencer.h"

DEFINE_LOG_CATEGORY_STATIC(ActionSequencerLog, Log, All);

// TimeManager 사용을 위한 WorldContextObject 설정
void UActionSequencer::Initialize(UObject* NewInWorldContextObject)
{
	InWorldContextObject = NewInWorldContextObject;

	if (InWorldContextObject == nullptr)
	{
		UE_LOG(ActionSequencerLog, Error, TEXT("UActionSequencer::Initialize : Action Sequencer object is null"));
	}
}

// Action 작동시 사용할 PortraitWidget을 캐싱
void UActionSequencer::AddPortraitWidget(const ESpeakerID PortraitOwner, UWidget* Widget)
{
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::AddPortraitWidget : Enter"));

	if (CachedWidgetMap.Contains(PortraitOwner))
	{
		const UEnum* Enum = StaticEnum<ESpeakerID>();
		FString PortraitOwnerString =  Enum->GetNameStringByValue(static_cast<int64>(PortraitOwner));
		UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::AddPortraitWidget : %s Already Exist")
			, *PortraitOwnerString);
		return;
	}
	
	if (UPortraitItemWidget* PortraitItemWidget = Cast<UPortraitItemWidget>(Widget))
	{
		CachedWidgetMap.Add(PortraitOwner, PortraitItemWidget);
		return;
	}

	const FString NullTargetString = Widget->IsVisible() ? Widget->GetName() : TEXT("Empty Widget");
	UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::AddPortraitWidget : %s cast fail"), *NullTargetString);
}

// 현재 대화에 필요한 Action에 대한 모든 연출 순차 실행
void UActionSequencer::StartSequence(const TArray<FPortraitActionData>& ActionDatas, const bool bIsSkip)
{
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::StartSequence : Enter"));
	
	// TODO DialogueSusystem에서 Skip시 최종 End 노드에서 StartSquence가 호출될 수 있는 상황 여부 판단 후 Skip관련 로직 최종 결정 필요
	// StopSequence();
	
	if (ActionDatas.Num() == 0)
	{
		UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::StartSequence : ActionDatas is Empty"));
		return;
	}

	CachedActionDatas = ActionDatas;
	
	if (bIsSkip)
	{
		StopSequence();
		return;
	}

	ResetState();

	AdvanceToNextAction();
}

/*
 * 현재 대화의 전체 연출 중단
 * 이때 ActionDatas를 순회하면 연출의 최종 Position에 Portrait_Item들을 배치한 후 다음 ActionData로 넘어간다
 * ActionDatas에 대한 전체 순회가 종료되면 종료
 */
void UActionSequencer::StopSequence()
{
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::StopSequence : Enter"));

	ClearAllTimer();
}

UWorld* UActionSequencer::GetWorld() const
{
	return InWorldContextObject.IsValid() ? InWorldContextObject->GetWorld() : nullptr;
}

// 다음 연출에 대한 작업을 시작
void UActionSequencer::AdvanceToNextAction()
{
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::AdvanceToNextAction : Enter"));

	++CurrentActionIndex;

	if (CachedActionDatas.IsValidIndex(CurrentActionIndex) == false)
	{
		return;
	}
	
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::AdvanceToNextAction : %d"), CurrentActionIndex);

	const FPortraitActionData ActionData = CachedActionDatas[CurrentActionIndex];
	switch (ActionData.ActionType)
	{
	case EPortraitActionType::Move:
		if (ActionData.Delay <= 0.0f)
		{
			PlayCurrentMoveAction();
			break;
		}
		
		if (UWorld* World = GetWorld())
		{
			FTimerManager& TimerManager = World->GetTimerManager();
			TimerManager.SetTimer(DelayTimerHandle, this, &UActionSequencer::PlayCurrentMoveAction
				, ActionData.Delay, false);
		}
		break;
	case EPortraitActionType::Emote:
		break;
	case EPortraitActionType::None:
		break;
	}
}

void UActionSequencer::PlayCurrentMoveAction()
{
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::PlayCurrentMoveAction : Enter"));

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(DelayTimerHandle);
	}

	const FPortraitActionData& ActionData = CachedActionDatas[CurrentActionIndex];
	CurrentActionTargetWidget = CachedWidgetMap.FindRef(ActionData.ActionTargetSpeakerID);
	if (CurrentActionTargetWidget.IsValid() == false)
	{
		UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::PlayCurrentMoveAction : CurrentActionTargetWidget is null"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.SetTimer(TickTimerHandle, this, &UActionSequencer::TickCurrentMoveAction, SequenceTickInterval, true, 0.0f);
	}
}

void UActionSequencer::TickCurrentMoveAction()
{
	const FPortraitActionData& ActionData = CachedActionDatas[CurrentActionIndex];
	const float ActionDuration = FMath::Max(ActionData.Duration, 0.0f);
	CurrentActionElapsedTime += SequenceTickInterval;
	const float Alpha = FMath::Clamp(CurrentActionElapsedTime / ActionDuration, 0.0f, 1.0f);
	const bool bIsComplete = CurrentActionElapsedTime >= ActionData.Duration;
	const FVector2D NewTranslation = bIsComplete
	? ActionData.ToTranslation
	: FMath::Lerp(ActionData.FromTranslation, ActionData.ToTranslation, Alpha);
	
	CurrentActionTargetWidget->SetBaseTranslation(NewTranslation);

	if (bIsComplete)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerManager& TimerManager = World->GetTimerManager();
			TimerManager.ClearTimer(TickTimerHandle);
		}

		CurrentActionElapsedTime = 0.0f;
		AdvanceToNextAction();
	}
}

void UActionSequencer::SkipSequence(const FPortraitActionData& ActionData)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
	}
}

void UActionSequencer::ResetState()
{
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::ResetState : Enter"));

	CurrentActionIndex = INDEX_NONE;
	CurrentActionElapsedTime = 0.0f;
	ClearAllTimer();
}

void UActionSequencer::ClearAllTimer()
{
	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::ClearAllTimer : Enter"));

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(DelayTimerHandle);
		TimerManager.ClearTimer(TickTimerHandle);
	}
}