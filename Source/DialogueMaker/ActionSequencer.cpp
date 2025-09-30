// Fill out your copyright notice in the Description page of Project Settings.

#include "ActionSequencer.h"

#include "GPUSkinVertexFactory.h"
#include "PortraitSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"

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

// 스크린 밖에서 진입하는 연출을 위해 Widget의 크기를 기준으로 스크린 밖의 좌표 반환
bool UActionSequencer::TryGetOutOfScreenPosition(const UCanvasPanelSlot* CanvasPanelSlot,
	const EPortraitSide PortraitSide, FVector2D& OutPosition) const
{
	const UPanelWidget* Parent = CanvasPanelSlot->Parent;
	const UWidget* Widget = CanvasPanelSlot->Content;
	if (Parent == nullptr || Widget == nullptr)
	{
		UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::GetOutOfScreenPosition : Parent or Widget is nullptr"));

		return false;
	}

	// 직전에 생성된 위젯이 아직 뷰포트에 올라가지 않은 경우
	if (!Parent->GetCachedWidget().IsValid() || !Widget->GetCachedWidget().IsValid())
	{
		UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::GetOutOfScreenPosition : Cached Parent or Cached Widget is nullptr"));
		
		return false;
	}

	const FVector2D ParentSize = Parent->GetCachedGeometry().GetLocalSize();
	const FVector2D WidgetSize = Widget->GetCachedGeometry().GetLocalSize();
	const FAnchors CurrentAnchors = CanvasPanelSlot->GetAnchors();
	const FVector2D CurrentAlignment = CanvasPanelSlot->GetAlignment();

	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::GetOutOfScreenPosition : ParentSize %s, WidgetSize %s, CurrentAnchors %s, CurrentAlignment %s")
		, *ParentSize.ToString(), *WidgetSize.ToString(), *CurrentAnchors.Minimum.ToString(), *CurrentAlignment.ToString())

	const FVector2D AnchorPoint = CurrentAnchors.Minimum * ParentSize;
	const FVector2D TopLeft = AnchorPoint - (CurrentAlignment * WidgetSize) + Widget->GetRenderTransform().Translation;

	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::GetOutOfScreenPosition : AnchorPoint %s, TopLeft %s")
		, *AnchorPoint.ToString(), *TopLeft.ToString());
	
	switch (PortraitSide)
	{
	case EPortraitSide::Left:
		UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::GetOutOfScreenPosition : return value = (%f, 0.0f)"), -(TopLeft.X + WidgetSize.X) - 1.0f);

		OutPosition = FVector2D(-(TopLeft.X + WidgetSize.X) - 1.0f, 0.0f);
		return true;
		
	case EPortraitSide::Right:
	default:
		UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::GetOutOfScreenPosition : return value = (%f, 0.0f)"), ParentSize.X - WidgetSize.X + 1.0f);

		OutPosition = FVector2D(ParentSize.X - WidgetSize.X + 1.0f, 0.0f);
		return true;
	}
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
		CachedOriginalRenderTranslation = CurrentActionTargetWidget->GetRenderTransform().Translation;

		UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::PlayCurrentMoveAction : CachedCurrentRenderTranslation %s"), *CachedOriginalRenderTranslation.ToString())
		
		TimerManager.SetTimer(TickTimerHandle, this, &UActionSequencer::TickCurrentMoveAction, SequenceTickInterval, true, 0.0f);
	}
}

void UActionSequencer::TickCurrentMoveAction()
{
	CurrentActionElapsedTime += SequenceTickInterval;
	
	const FPortraitActionData& ActionData = CachedActionDatas[CurrentActionIndex];
	const float ActionDuration = FMath::Max(ActionData.Duration, 0.0f);
	const float Alpha = ActionDuration <= 0.0f ? 1.0f : FMath::Clamp(CurrentActionElapsedTime / ActionDuration, 0.0f, 1.0f);
	const bool bIsComplete = CurrentActionElapsedTime >= ActionData.Duration;
	
	const FVector2D ResolvedFromTranslation = ActionData.FromTranslation.IsNearlyZero()
	? CachedOriginalRenderTranslation
	: ActionData.FromTranslation;
	
	const FVector2D ResolvedToTranslation = ResolveTargetTranslation(ActionData);
	
	const FVector2D NewTranslation = bIsComplete
	? ResolvedToTranslation
	: FMath::Lerp(ResolvedFromTranslation, ResolvedToTranslation, Alpha);
	
	CurrentActionTargetWidget->SetBaseTranslation(NewTranslation);

	// 연출 시간 종료시 TickCurrentMoveAction 반복 종료 및 다음 ActionData 실행으로 이동
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

// Widget이 TargetSide, TargetOffset 위치로 간다면 현재 위치를 기준으로 어떻게 이동해야하나 연산
FVector2D UActionSequencer::ResolveTargetTranslation(const FPortraitActionData& ActionData) const
{
	UCanvasPanelSlot* CanvasPanelSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(CurrentActionTargetWidget.Get());
	if (CanvasPanelSlot == nullptr)
	{
		UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::ResolveTargetTranslation : CanvasPanelSlot is nullptr"));

		return FVector2D::ZeroVector;
	}

	UPortraitSubsystem* PortraitSubsystem = UPortraitSubsystem::Get(InWorldContextObject.Get());
	if (PortraitSubsystem == nullptr)
	{
		UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::ResolveTargetTranslation : PortraitSubsystem is nullptr"));

		return FVector2D::ZeroVector;
	}

	// Current와 Target을 기준으로 AnchorPositionDelta - AlignmentDelta * WidgetSize + CurrentRenderTranslation(위젯의 실제 위치) 구하기
	const FAnchors CurrentAnchors = CanvasPanelSlot->GetAnchors();
	const FVector2D CurrentAlignment = CanvasPanelSlot->GetAlignment();
	const FAnchors TargetAnchors = PortraitSubsystem->GetPortraitAnchors(ActionData.TargetSide);
	const FVector2D TargetAlignment = PortraitSubsystem->GetPortraitAlignment(ActionData.TargetSide);

	UPanelWidget* Parent = CanvasPanelSlot->Parent;
	UWidget* Widget = CanvasPanelSlot->Content;
	if (Parent == nullptr || Widget == nullptr)
	{
		UE_LOG(ActionSequencerLog, Warning, TEXT("UActionSequencer::ResolveTargetTranslation : Parent or Widget is nullptr"));

		return FVector2D::ZeroVector;
	}

	FVector2D ParentSize = Parent->GetCachedGeometry().GetLocalSize();
	FVector2D WidgetSize = Widget->GetCachedGeometry().GetLocalSize();

	UE_LOG(ActionSequencerLog, Display, TEXT("UActionSequencer::ResolveTargetTranslation : ParentSize %s, WidgetSize %s, CurrentAnchors %s, CurrentAlignment %s")
		, *ParentSize.ToString(), *WidgetSize.ToString(), *CurrentAnchors.Minimum.ToString(), *CurrentAlignment.ToString())

	auto ResolveAnchors = [&ParentSize](const FAnchors& Anchors, const FVector2D& Alignment)
	{
		const FVector2D AnchorMin = Anchors.Minimum * ParentSize;
		const FVector2D AnchorMax = Anchors.Maximum * ParentSize;

		return FVector2D(
			FMath::Lerp(AnchorMin.X, AnchorMax.X, Alignment.X),
			FMath::Lerp(AnchorMin.Y, AnchorMax.Y, Alignment.Y));
	};

	const FVector2D CurrentAnchorPosition = ResolveAnchors(CurrentAnchors, CurrentAlignment);
	const FVector2D TargetAnchorPosition = ResolveAnchors(TargetAnchors, TargetAlignment);
	const FVector2D AnchorPositionDelta = TargetAnchorPosition - CurrentAnchorPosition;
	const FVector2D AlignmentDelta = TargetAlignment - CurrentAlignment;
	const FVector2D CalculatedTargetTranslation = AnchorPositionDelta - (AlignmentDelta * WidgetSize) + ActionData.TargetSideOffset;

	return CalculatedTargetTranslation;
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